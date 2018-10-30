#!/bin/bash

# p44b: host script for managing LEDE based product builds
shopt -s extglob

# must always be started from buildroot
BUILDROOT="$(pwd -P)"

if [[ $# -ne 3 ]]; then
  echo "Usage: $0 <ethernet-if> <probe_cmd> <probe_result>"
  echo " Note: on Mac, use 'networksetup -listallhardwareports' to find ethernet interface name"
  exit 1
fi
ETH_IF=$1
PROBE_CMD=$2
PROBE_RESULT=$3

PROG_LOG="/tmp/flashed_ipv6"


# calculate path to current image
source ".config" 2>/dev/null # current settings, not saved ones
IMG_DIST_NAME="${CONFIG_VERSION_DIST}"
IMG_DIST_NAME_LC=$(echo "$IMG_DIST_NAME" | awk '{print tolower($0)}')

SRCIMGS=$(ls -1t ${BUILDROOT}/bin/targets/${CONFIG_TARGET_BOARD}/${CONFIG_TARGET_SUBTARGET}/${IMG_DIST_NAME_LC}-${CONFIG_VERSION_NUMBER}*-${CONFIG_TARGET_BOARD}-${CONFIG_TARGET_SUBTARGET}-${CONFIG_TARGET_PROFILE##DEVICE_}-*);
set -- $SRCIMGS
FWIMG_FILE="$1"

echo "image: ${FWIMG_FILE}"

# look for targets
TARGET_IPV6_PREFIX="fe80::42a3:6bff:fec" # Prefix for possible targets

TARGET_MATCH="s/^[0-9]+ +bytes from (${TARGET_IPV6_PREFIX}[0-9A-Fa-f]:[0-9A-Fa-f]{4}).*\$/\1/p"

echo "Searching for targets via ping6 multicast (4 seconds)..."
TARGETS_IPV6=""
# use ping6 multicast to query devices
TARGETS_IPV6=$(ping6 -c 2 -i 4 ff02::1%${ETH_IF} | sed -n -E -e "${TARGET_MATCH}")

# create flash script
FWFLASH_SCRIPT="/tmp/fwflash"
cat >"${FWFLASH_SCRIPT}" <<'ENDOFFILE1'
cd /tmp
sysupgrade p44firmware.bin >/tmp/sysupgrade.out 2>&1 &
# two minutes is enough for flashing
sleep 120
echo "**** Error: should have restarted with new firmware, but apparently has not"
ENDOFFILE1
chmod a+x "${FWFLASH_SCRIPT}"

# process found devices
touch "${PROG_LOG}"
for TARGET_IPV6 in ${TARGETS_IPV6}; do
  echo ""
  echo "Found: ${TARGET_IPV6}"
  # check if this unit was already programmed
  grep -q "${TARGET_IPV6}" "${PROG_LOG}"
  if [[ $? == 0 ]]; then
    # already programmed
    echo "- Device already programmed -> skipping"
  else
    # not yet programmed
    TARGET_LINKLOCAL="${TARGET_IPV6}%${ETH_IF}"
    RES=$(ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no  root@${TARGET_LINKLOCAL} "${PROBE_CMD}" 2>/dev/null)
    echo "${RES}" | grep -q "${PROBE_RESULT}"
    if [[ $? != 0 ]]; then
      # this is a not target
      echo "- Device is not a target (test '${PROBE_CMD}' result does not contain ${PROBE_RESULT}: '${RES}')"
    else
      # this is a target
      echo "- Device is a target, copying update script and image to /tmp"
      scp -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no "${FWFLASH_SCRIPT}" root@[${TARGET_LINKLOCAL}]:/tmp 2>/dev/null
      scp -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no "${FWIMG_FILE}" root@[${TARGET_LINKLOCAL}]:/tmp/p44firmware.bin 2>/dev/null

      echo "- firmware copied, starting fw flash"
      ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no  root@${TARGET_LINKLOCAL} "/tmp/fwflash"
      # 2>/dev/null
    fi
    # remember
    echo "${TARGET_IPV6}" >>"${PROG_LOG}"
  fi
done

exit 0

