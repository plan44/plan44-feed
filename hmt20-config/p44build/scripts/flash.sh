#!/bin/bash

FLASHER=omega2flash

if ! which -s "${FLASHER}"; then
  echo "Missing omega2flash utility"
  exit 1
fi

if [[ $# -lt 2 ]]; then
  echo "Usage: flash <ethernet-if> <auto|wait|omega2_ipv6|list|ping> [IMG [<uboot_env_file>]]"
  exit 1
fi

IF=$1
shift
MODE=$1
shift
IMG=$1
shift
if [[ "${IMG}" == "IMG" ]]; then
  echo "Using image = ${BUILT_IMAGE}"
  IMG="${BUILT_IMAGE}"
fi
echo "### Invoking:" "${FLASHER}" "${IF}" "${MODE}" "${IMG}" "$@"
"${FLASHER}" "${IF}" "${MODE}" "${IMG}" "$@"
