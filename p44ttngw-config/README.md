# How to build the P44-TTNGW firmware image based on OpenWrt

## Preparations

### prerequisites for OpenWrt on Linux

Details see official [OpenWrt](https://wiki.openwrt.org/doc/howto/buildroot.exigence) instructions.

Usually the following is sufficient:

- `sudo apt-get install git-core build-essential libssl-dev libncurses5-dev unzip gawk zlib1g-dev`

Additionally, we need quilt to patch the OpenWrt tree with p44b before running make for the first time (which will build quilt), so we just install it:

- `sudo apt-get install quilt`


### prerequisites for OpenWrt on macOS

1. OpenWrt needs a case sensitive file system, but macOS by default has a case insensitive file system. So OpenWrt needs to be put on a volume with case sensitive file system.

   On macOS before 10.13, you can create a sparse image of ~20GB with Disk Utility.

   With macOS 10.13 and later, the APFS file system allows creating extra volumes without partitioning or reserving any space - this is much better than a disk image (*Disk Utility -> Edit -> Add APFS volume...*)

2. XCode needs to be installed

3. Some utilities are needed from homebrew
   - install brew [as described here](https://brew.sh)
   - `brew tap homebrew/dupes`
   - `brew install coreutils findutils gawk gnu-getopt gnu-tar grep wget quilt xz`
   - `brew ln gnu-getopt --force`

Also see [OpenWrt Quick Image Building docs](https://openwrt.org/docs/guide-developer/quickstart-build-images) for general info about OpenWrt bootstrap.

### Get OpenWrt (formerly LEDE, formerly OpenWrt)

Assuming that you've created and mounted a case sensitive volume named `CaseSens` already (macOS case, for Linux just use any directory of your choice):

    cd /Volumes/CaseSens
    git clone -o openwrt.org -b openwrt-18.06 https://git.openwrt.org/openwrt/openwrt.git openwrt

### Get the p44build script

Note: this is a script that helps managing differently configured OpenWrt targets on top of an unpolluted OpenWrt original tree.

    git clone -o github https://github.com/plan44/p44build.git

## Start working with OpenWrt

Go to the directory where you checked out OpenWrt and p44build (`/Volumes/CaseSens` on macOS according to the steps above). Then:

    cd openwrt
    git checkout -b p44ttngw v18.06.2

### Configure the extra feeds we need

    # do NOT change feeds.conf.default - custom changes belong into feeds.conf!
    cp feeds.conf.default feeds.conf
    echo "src-git plan44 https://github.com/plan44/plan44-feed.git;master" >>feeds.conf
    echo "src-git onion https://github.com/OnionIoT/OpenWRT-Packages.git" >>feeds.conf

    ./scripts/feeds update -a

#### optionally: Unshallow plan44 feed to be able to work with tools like GitX in it

OpenWrt clones only a shallow (no history) copy of the feed repository. This saves space, but limits git operations (and crashes tools like GitX). The following steps convert the feed into a regular repository:

    pushd feeds/plan44
    git fetch --unshallow
    popd

### Initialize p44b(uild) for p44ttngw

#### Direct p44b to the information that will control everything

    TARGET_CFG_PACKAGE="plan44/p44ttngw-config"
    ../p44build/p44b init feeds/${TARGET_CFG_PACKAGE}/p44build

#### Prepare (patch) the OpenWrt tree for p44ttngw

    ./p44b prepare

**Note:** My standard setup disables *any* password based login by default, by providing a modified shadow file in `files/etc/shadow`. If you want the standard OpenWrt default of no initial password, then delete this extra file now:

    rm files/etc/shadow

#### Install needed packages from feeds

only install (= make ready for OpenWrt to potentially build at all)
those packages that were recorded present at last './p44b save':

    ./p44b instpkg

or just install all packages from all feeds:

    ./scripts/feeds install -a

#### Some tweaks apparently needed (for now)

1. If python/python3 package is installed, make will try to host-compile it and fail on macOS. As we don't need python at all, just make sure those packages are not installed:

    - `./scripts/feeds uninstall python`
    - `./scripts/feeds uninstall python3`

#### Configure OpenWrt for the target platform

    # shows all possible targets, currently only Omega2
    ./p44b target

    # select the target
    ./p44b target p44ttngw-omega2

#### optionally: Inspect/change config to add extra features

    make menuconfig

### Build P44-TTNGW image

    make
    # or, if you have multiple CPU cores you want to use (3, here)
    # to speed up things, allow parallelizing jobs:
    make -j 3

**Note:** when doing this for the first time, it takes a looooong time (hours). This is because initial LEDE build involves creating the compiler toolchain, and the complete linux kernel and tools. Subsequent builds will be faster.

### Use the firmware to run a TTN gateway

#### Prepare the hardware

Of course, to actually get a working TTN gateway, you also need to connect a LoRa concentrator to the SPI bus. I use a RAK831, connected as follows:

Function | Omega(2) Dock     | Omega2    | RAK831
-------- | ----------------- | --------- | ------
+5V      | 5V                | 5V        | 1
GND      | GND               | GND       | 3,5
RAK_RST  | 0                 | GPIO0     | 19
SCK      | 7                 | SPI_CK(7) | 18
MISO     | 12 (or 9)         | MISO(9)   | 17
MOSI     | 8                 | MOSI(8)   | 16
CS1      | 6                 | CS1(6)    | 15


#### Flash the firmware image to the Omega2

If everything went well, the LEDE build process will have produced a ready-to-flash firmware image in `bin/targets/ramips/mt76x8`. You can now send this to the Omega2 and flash it.

    # specify the IP address of your Omega2 here
    TARGET_HOST=192.168.11.86

    # copy FW image to the Omega2
    scp -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no bin/targets/ramips/mt7688/p44ttngw-*-ramips-mt7688-omega2-squashfs-sysupgrade.bin root@${TARGET_HOST}:/tmp

    # login to the Omega2
    ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no root@${TARGET_HOST}

    # on the Omega2, flash the new firmware image
    cd /tmp
    sysupgrade -n p44ttngw-*.bin


After that, Omega reboots and is now a TTN gateway :-)

**Note:** The image's default config has WiFi enabled including an access point named *P44TTNGW*. To disable Wifi, or to connect the gateway to a WiFi AP, edit `/etc/config/wireless` accordingly (especially the `option disabled` and `ssid`/`key`).

#### Configure the gateway

For now (first rough version of this package), the gateway LoRa config is completely static except for the gateway EUI (which is calculated from the Omega2 MAC address).

You can edit `/etc/lora/local_conf.json` to add your gateway name/description. But do **not change the `##GATEWAY_EUI##`**, this is a placeholder for the MAC-derived EUI!

#### start, stop, logs, tools

packet_forwarder is under `runit` control, so you can stop and start it (after boot, it starts automatically) and see current status from command line:

    sv stop p44ttngw
    sv start p44ttngw
    sv status p44ttngw

Current log is in /var/log/p44ttngw/current, to have it displayed live:

    tail -F /var/log/p44ttngw/current

The tools to low-level test SPI communication with the concentrator (`util_*`, `test_loragw_*`) are in `/root/lora`. Don't forget to `sv stop p44ttngw` before trying these tools.


### Finally, but optionally: save your build's config, feeds, versions

On the build machine:

    ./p44b save

This records the precise details of this build into `feeds/plan44/p44ttngw-config/p44build`, in particular the LEDE tree's SHA and `.config` as well as the SHAs of the feeds used.
The idea is that this can be committed back into the p44ttngw-config package, as kind of a "head" record for this very p44ttngw firmware build, and allows to go back to this point later, even if the LEDE tree was used to build other firmware images in between (in fact, exactly that is the very purpose of `p44b` - the ability to work and switch between different firmware projects in a single LEDE tree)
