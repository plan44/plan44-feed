#!/bin/bash

# in p44-xx-open context, the flasher should be here:
FLASHER = "${BUILDROOT}/../omega2flash/omega2flash.sh"
if [ ! -x "${FLASHER}" ]; then
  # other context, just hope it is in the path
  FLASHER=omega2flash
  if ! which -s "${FLASHER}"; then
    echo "Missing omega2flash utility"
    exit 1
  fi
fi

if [[ $# -lt 2 ]]; then
  echo "Usage: flash <ethernet-if> <auto|wait|omega2_ipv6|list|ping> [IMG [<uboot_env_file>] [<extra_conf_files_dir]]"
  echo " Note: on Mac, use 'networksetup -listallhardwareports' to find ethernet interface name"
  exit 1
fi

SCRIPTDIR="$(dirname $0)"
cd "${SCRIPTDIR}"

INTF=$1
shift
MODE=$1
shift
IMG=$1
shift
if [[ "${IMG}" == "IMG" ]]; then
  echo "Using image = ${BUILT_IMAGE}"
  IMG="${BUILT_IMAGE}"
fi
echo "### Invoking:" "${FLASHER}" "${INTF}" "${MODE}" "${IMG}" "$@"
"${FLASHER}" "${INTF}" "${MODE}" "${IMG}" "$@"
