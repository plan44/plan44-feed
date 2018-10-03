# How to build the HERMEL firmware image based on OpenWrt (running on LETH led controller hardware)

## Preparations

### prerequisites for OpenWrt on macOS

1. OpenWrt needs a case sensitive file system, but macOS by default has a case insensitive file system. So OpenWrt needs to be put on a volume with case sensitive file system.

   On macOS before 10.13, you can create a case sensitive HFS sparse image of ~20GB with Disk Utility.

   With macOS 10.13 and later, the APFS file system allows creating extra volumes without partitioning or reserving any space - this is much better than a disk image (*Disk Utility -> Edit -> Add APFS volume...*)

2. XCode needs to be installed (to get the basic build tools)
   - Note that command line tools also need to be installed (`/usr/bin/xcode-select --install`).


3. Some utilities are needed from homebrew
   - install brew [as described here](https://brew.sh)
   - `brew tap homebrew/dupes`
   - `brew install coreutils findutils gawk gnu-getopt gnu-tar gnu-time grep wget quilt xz`
   - `brew ln gnu-getopt --force`

Also see [https://openwrt.org/docs/guide-developer/quickstart-build-images](https://openwrt.org/docs/guide-developer/quickstart-build-images) for general info about OpenWrt bootstrap.

### Get OpenWrt (formerly LEDE, formerly OpenWrt)

Assuming that you've created and mounted a **case sensitive volume named `OpenWrt`** already:

    cd /Volumes/OpenWrt
    git clone -o openwrt.org -b openwrt-18.06 https://git.openwrt.org/openwrt/openwrt.git openwrt

### Get the p44build script

Note: this is a script that helps managing differently configured OpenWrt targets on top of an unpolluted OpenWrt original tree.

    git clone -o github https://github.com/plan44/p44build.git

## Start working with OpenWrt

Go to openwrt directory and check out the current stable release

    cd openwrt
    git checkout -b hermel v18.06.1

Note: I'm using the latest official release tagged `v18.06.0`

### Configure the extra feeds we need

    # do NOT change feeds.conf.default - custom changes belong into feeds.conf!
    cp feeds.conf.default feeds.conf
    # plan44.ch feed
    echo "src-git plan44 https://github.com/plan44/plan44-feed.git;master" >>feeds.conf
    # onion.io feed
    echo "src-git onion https://github.com/OnionIoT/OpenWRT-Packages.git" >>feeds.conf

    ./scripts/feeds update -a

#### recommended: Unshallow plan44 feed to be able to work with tools like GitX in it

OpenWrt clones only a shallow (no history) copy of the feed repository. This saves space, but limits git operations (and crashes tools like GitX). The following steps convert the feed into a regular repository:

    pushd feeds/plan44
    git fetch --unshallow
    popd

### Initialize p44b(uild) for HERMEL

#### Direct p44b to the information that will control everything

    TARGET_CFG_PACKAGE="plan44/hermel-config"
    ../p44build/p44b init feeds/${TARGET_CFG_PACKAGE}/p44build

#### Prepare (patch) the OpenWrt tree for HERMEL

    p44b prepare

**Note:** My standard setup disables *any* password based login by default, by providing a modified shadow file in `files/etc/shadow`. If you want the standard OpenWrt default of no initial password, then delete this extra file now:

    rm files/etc/shadow

#### Install needed packages from feeds

you can only install (= make ready for OpenWrt to potentially build at all)
those packages that were recorded present at last 'p44b save':

     p44b instpkg

or just install all packages from all feeds:

    ./scripts/feeds install -a

#### Some tweaks apparently needed (for now)

If python/python3 package is installed, make will try to host-compile it and fail on macOS. As we don't need python at all, just make sure those packages are not installed:

    scripts/feeds uninstall python
    scripts/feeds uninstall python3

#### Configure OpenWrt for the target platform

    # shows all possible targets, currently only leth-omega2
    p44b target

    # select the target
    p44b target leth-omega2

#### optionally: Inspect/change config to add extra features

    make menuconfig

### Build HERMEL image

    make
    # or, if you have multiple CPU cores you want to use (3, here)
    # to speed up things, allow parallelizing jobs:
    make -j 3

**Note:** when doing this for the first time, it takes a looooong time (hours). This is because initial OpenWrt build involves creating the compiler toolchain, and the complete linux kernel and tools. Subsequent builds will be faster.


#### Flash the firmware image to the Omega2

If everything went well, the OpenWrt build process will have produced a ready-to-flash firmware image in `bin/targets/ramips/mt7688`. You can now send this to the Omega2 and flash it.

    # specify the IP address of your Omega2 here
    export TARGET_HOST=192.168.11.86

    # copy FW image to the Omega2
    p44b send

    # login to the Omega2
    p44b login

    # on the Omega2, flash the new firmware image
    cd /tmp
    sysupgrade -n hermel-*.bin


After that, Omega reboots and runs the hermeld daemon.

#### start, stop, logs, tools

`lethd` is under `runit` control, so you can stop and start it (after boot, it starts automatically) and see current status from command line:

    sv stop hermeld
    sv start hermeld
    sv status hermeld

Current log is in /var/log/hermeld/current, to have it displayed live:

    tail -F /var/log/hermeld/current


### Finally, but optionally: save your build's config, feeds, versions

On the build machine:

    p44b save

This records the precise details of this build into `feeds/plan44/hermel-config/p44build`, in particular the OpenWrt tree's SHA and `.config` as well as the SHAs of the feeds used.

The idea is that this can be committed back into the hermel-config package, as kind of a "head" record for this very hermeld firmware build, and allows to go back to this point later, even if the OpenWrt tree was used to build other firmware images in between.

In fact, exactly that is the very purpose of `p44b` - the ability to work and switch between different firmware projects in a single OpenWrt tree.

