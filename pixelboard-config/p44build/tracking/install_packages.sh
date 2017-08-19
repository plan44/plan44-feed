# Run this script to install all packages that were installed at the time of p44b save
./scripts/feeds update onion
./scripts/feeds install -p onion gpio-test
./scripts/feeds install -p onion omega2-ctrl
./scripts/feeds install -p onion wifimanager
./scripts/feeds install -p onion wifisetup
./scripts/feeds update p44i
./scripts/feeds install -p p44i p44-maint-keys
./scripts/feeds update packages
./scripts/feeds install -p packages alsa-lib
./scripts/feeds install -p packages alsa-utils
./scripts/feeds install -p packages avahi
./scripts/feeds install -p packages bash
./scripts/feeds install -p packages boost
./scripts/feeds install -p packages db47
./scripts/feeds install -p packages dbus
./scripts/feeds install -p packages expat
./scripts/feeds install -p packages gdbm
./scripts/feeds install -p packages intltool
./scripts/feeds install -p packages libdaemon
./scripts/feeds install -p packages libffi
./scripts/feeds install -p packages libpam
./scripts/feeds install -p packages libxml2
./scripts/feeds install -p packages openssh
./scripts/feeds install -p packages protobuf-c
./scripts/feeds install -p packages sqlite3
./scripts/feeds install -p packages xz
./scripts/feeds update plan44
./scripts/feeds install -p plan44 i2c-tools
./scripts/feeds install -p plan44 libev
./scripts/feeds install -p plan44 libpng
./scripts/feeds install -p plan44 messagetorch
./scripts/feeds install -p plan44 mg44
./scripts/feeds install -p plan44 kmod-p44-ledchain
./scripts/feeds install -p plan44 p44sbbd
./scripts/feeds install -p plan44 p44wiper-config
./scripts/feeds install -p plan44 p44wiperd
./scripts/feeds install -p plan44 pagekitec
./scripts/feeds install -p plan44 pixelboard-config
./scripts/feeds install -p plan44 pixelboardd
./scripts/feeds install -p plan44 timidity
