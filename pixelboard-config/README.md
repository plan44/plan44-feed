# How to build the Pixelboard FW image based on LEDE

## Preparations

### prerequisites for LEDE on macOS

1. LEDE needs a case sensitive file system, macOS by default has a case insensitive file system (might change with apfs on high sierra, don't know yet). So at least for now, LEDE needs to be put on a volume with case sensitive file system (a sparse image of ~20GB created with Disk Utility works fine).

2. XCode needs to be installed

3. Some utilities are needed from homebrew
   - install brew [as described here](https://brew.sh)
   - `brew tap homebrew/dupes`
   - `brew install coreutils findutils gawk gnu-getopt gnu-tar grep wget quilt xz`
   - `brew ln gnu-getopt --force`

Also see [https://lede-project.org/docs/guide-developer/quickstart-build-images](https://lede-project.org/docs/guide-developer/quickstart-build-images) for general info about LEDE bootstrap.

### Get LEDE

Assuming that you've created and mounted a case sensitive volume named `LEDE` already:

    cd /Volumes/LEDE
    git clone -o lede-project https://git.lede-project.org/source.git lede-master

### Get the p44build script

Note: this is a script that helps managing differently configured LEDE targets on top of an unpolluted LEDE original tree.

    git clone -o github https://github.com/plan44/p44build.git

## Start working with LEDE

Go to the LEDE master checkout directory

    cd lede-master

### Configure the extra feeds we need

    cp feeds.conf.default feeds.conf
    echo "src-git plan44 https://github.com/plan44/plan44-feed.git;for-lede-master" >>feeds.conf
    echo "src-git onion https://github.com/OnionIoT/OpenWRT-Packages.git" >>feeds.conf

    ./scripts/feeds update -a

#### optionally: Unshallow plan44 feed to be able to work with tools like GitX in it

LEDE clones only a shallow (no history) copy of the feed repository. This saves space, but limits git operations (and crashes tools like GitX). The following steps convert the feed into a regular repository:

    pushd feeds/plan44
    git fetch --unshallow
    popd

### Initialize p44b(uild) for pixelboard

#### Direct p44b to the information that will control everything

    TARGET_CFG_PACKAGE="plan44/pixelboard-config"
    ../p44build/p44b init feeds/${TARGET_CFG_PACKAGE}/p44build

#### Prepare (patch) the LEDE tree for pixelboard

    p44b prepare

**Note:** My standard setup disables *any* password based login by default, by providing a modified shadow file in `files/etc/shadow`. If you want the standard LEDE default of no initial password, then delete this extra file now:

    rm files/etc/shadow

#### Install needed packages from feeds

only install (= make ready for LEDE to potentially build at all)
those packages that were recorded present at last 'p44b save':

     p44b instpkg

or just install all packages from all feeds:

    ./scripts/feeds install -a

#### Some tweaks apparently needed (for now)

1. If python/python3 package is installed, make will try to host-compile it and fail on macOS. As we don't need python at all, just make sure those packages are not installed:

    - `./scripts/feeds uninstall python`
    - `./scripts/feeds uninstall python3`

2. For pixelboardd, make sure the plan44 version of i2c-tools is installed, not the standard package. The difference is that it does not include python stuff, but installs the needed enhanced i2c-dev.h header plus a copy as lib-i2c-dev.h:

    - `./scripts/feeds uninstall i2c-tools`
    - `./scripts/feeds install -p plan44 i2c-tools`

#### Configure LEDE for the target platform

    # shows all possible targets, currently only Omega2
    p44b target

    # select the target
    p44b target pixelboard-omega2

#### optionally: Inspect/change config to add extra features

    make menuconfig

### Build pixelboard image

    make
    # or, if you have multiple CPU cores you want to use (3, here)
    # to speed up things, allow parallelizing jobs:
    make -j 3

**Note:** when doing this for the first time, it takes a looooong time (hours). This is because initial LEDE build involves creating the compiler toolchain, and the complete linux kernel and tools. Subsequent builds will be faster.

### Save your build's config, feeds, versions

    p44b save
