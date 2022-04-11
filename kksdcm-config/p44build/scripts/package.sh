#!/bin/bash

# p44b packaging script to package KKS-DCM OpenWrt based firmware for SD card deployment

# Name of upgrade image on SD card (as detected by /etc/service/kksdcmd/run script)
SD_UPDATEIMAGE="kks-dcm.sysupgrade.bin"


if [ $# -ne 1 ]; then
  echo "usage: $0 <target directory for image>"
  exit 1
fi
OUTPUTDIR="$1"
if [ ! -d "${OUTPUTDIR}" ]; then
  echo "output directory '${OUTPUTDIR}' does not exist"
  exit 1
fi

# check info we might have in the environment when called via p44b
# - buildroot
if [ -z ${BUILDROOT+x} ]; then
  BUILDROOT="$(pwd)"
fi
# - scripts dir (which also contains target update scripts)
if [ -z ${P44B_SCRIPTS_DIR+x} ]; then
  SCRIPTDIR=$(dirname $0)
else
  SCRIPTDIR="${P44B_SCRIPTS_DIR}"
fi
# - imagetype
if [ -z ${P44B_IMAGETYPE+x} ]; then
  P44B_IMAGETYPE="squashfs-sysupgrade"
fi

# build system (LEDE no longer supported)
DIST="openwrt"

# import .config:
source ${BUILDROOT}/.config 2>/dev/null

# determine params from .config vars

# e.g:
# CONFIG_TARGET_BOARD="ramips" or "brcm2708"
# CONFIG_TARGET_SUBTARGET="mt76x8"
# CONFIG_TARGET_PROFILE="DEVICE_KKSDCMOmega2"
TARGET="${CONFIG_TARGET_BOARD}"
SUBTARGET="${CONFIG_TARGET_SUBTARGET}"
PROFILE="${CONFIG_TARGET_PROFILE##DEVICE_}"

# CONFIG_VERSION_DIST="KKS-DCM"
IMG_DIST_NAME="${CONFIG_VERSION_DIST}"
IMG_DIST_NAME_LC=$(echo "$IMG_DIST_NAME" | awk '{print tolower($0)}')

# CONFIG_ARCH="mipsel" or "arm"
# CONFIG_CPU_TYPE="24kc" or "arm1176jzf-s_vfp"
# CONFIG_MUSL_VERSION="1.1.16"
# CONFIG_LIBC="musl"
# CONFIG_LIBC_VERSION="1.1.16"
# CONFIG_TARGET_SUFFIX="musl" or "muslgnueabi"
MUSL_SUFFIX=""
if [[ "${CONFIG_TARGET_SUFFIX#muslgnu}" == "eabi" ]]; then
  MUSL_SUFFIX="_eabi"
fi
# target identifying string
TARGETID="target-${CONFIG_ARCH}_${CONFIG_CPU_TYPE}_${CONFIG_LIBC}"
if [ ! -z "${CONFIG_LIBC_VERSION}" ]; then
  TARGETID="${TARGETID}-${CONFIG_LIBC_VERSION}${MUSL_SUFFIX}"
else
  TARGETID="${TARGETID}${MUSL_SUFFIX}"
fi

# CONFIG_VERSION_NUMBER="1.1.0"
VERSION="${CONFIG_VERSION_NUMBER}"

# Now create firmware components

echo "creating firmware image for ${IMG_DIST_NAME}_${VERSION}"

if [ -z ${BUILT_IMAGE+x} ]; then
  # - determine source image
  SRCIMGS=$(ls -1t ${BUILDROOT}/bin/targets/${TARGET}/${SUBTARGET}/${IMG_DIST_NAME_LC}-${VERSION}*-${TARGET}-${SUBTARGET}-${PROFILE}-${P44B_IMAGETYPE}*);
  set -- $SRCIMGS
  SRCIMAGE=$1
else
  SRCIMAGE="${BUILT_IMAGE}"
fi

if [[ ! -e "${SRCIMAGE}" ]]; then
  echo "Source image '${SRCIMAGE}' not found, script must be started from openwrt buildroot, after successful make"
  exit 1
fi
echo "Using source image '${SRCIMAGE}'"

IMGDIR="${OUTPUTDIR}/${IMG_DIST_NAME}_${VERSION}"

# - rename possible previous dir with same name
if [[ -d "${IMGDIR}" ]]; then
  mv "${IMGDIR}" "${IMGDIR}_previous_$(date '+%Y-%m-%d_%H.%M.%S')"
fi
mkdir -v -p "${IMGDIR}"
if [[ ! -d "${IMGDIR}" ]]; then
  echo "Cannot use directory ${IMGDIR}"
  exit 1
fi
# - copy image
cp "${SRCIMAGE}" "${IMGDIR}/${SD_UPDATEIMAGE}"

echo "created SD-ready firmware image in ${IMGDIR}"

exit 0
