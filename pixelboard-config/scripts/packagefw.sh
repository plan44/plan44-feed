#!/bin/bash

# host script to package P44-DSB MIPS based firmware for update server deployment

if [ $# -ne 1 ]; then
  echo "Must specify target profile (onion-omega, carambola2 or LinkIt7688) as parameter"
  exit 1
fi

PROFILE=$1

BUILDROOT="$(pwd)"

# determine build system
grep -q "LEDE Linux distribution" "${BUILDROOT}/README"
if [[ $? == 0 ]]; then
  DIST="lede"
else
  DIST="openwrt"
fi


if [[ "${PROFILE}" == "LinkIt7688" ]]; then
  TARGET=ramips
  SUBTARGET=mt7688
  #STAGINGROOT="${BUILDROOT}/staging_dir/target-mipsel_24kec+dsp_musl-1.1.15/root-ramips/"
  STAGINGROOT="${BUILDROOT}/staging_dir/target-mipsel_24kc_musl-1.1.15/root-ramips/"
elif [[ "${PROFILE}" == "P44DSBOmega2" ]]; then
  TARGET=ramips
  SUBTARGET=mt7688
  STAGINGROOT="${BUILDROOT}/staging_dir/target-mipsel_24kc_musl-1.1.15/root-ramips/"
else
  TARGET=ar71xx
  SUBTARGET=generic
  #STAGINGROOT="${BUILDROOT}/staging_dir/target-mips_34kc_uClibc-0.9.33.2/root-ar71xx/"
  STAGINGROOT="${BUILDROOT}/staging_dir/target-mips_34kc_musl-1.1.14/root-ar71xx/"
fi


UDSCRIPT="${BUILDROOT}/feeds/plan44/p44dsb-config/scripts/udscript.sh"

P44DSBROOT="/Users/luz/Documents/plan44/Projekte/plan44dalibridge"

CKSUM3TOOL="${P44DSBROOT}/open/p44-dsb-e-RPi/tools/cksum3"
IMAGESDIR="${P44DSBROOT}/open/p44-${DIST}/p44-ow-images"

if [[ ! -e "${STAGINGROOT}/etc/p44version" ]]; then
  echo "Staging must contain recent p44 build"
  exit 1
fi

# - prepare vars
VERSION=$(cat "${STAGINGROOT}/etc/p44version")
FEED=$(cat "${STAGINGROOT}/etc/p44feed")
echo "creating firmware components for ${VERSION}_${FEED}"
# - get product defs %%% a bit hacky
if [[ ! -f "${STAGINGROOT}/etc/p44product.defs" ]]; then
  echo "p44product.defs not found"
  exit 1
fi
source "${STAGINGROOT}/etc/p44product.defs"
FWPREFIX="${PRODUCT_IDENTIFIER}"


SRCIMAGE="${BUILDROOT}/bin/targets/${TARGET}/${SUBTARGET}/${DIST}-${VERSION}-${TARGET}-${SUBTARGET}-${PROFILE}-squashfs-sysupgrade.bin"

if [[ ! -e "${SRCIMAGE}" ]]; then
  echo "Source image '${SRCIMAGE}' not found, must be started from openwrt/LEDE buildroot, make done"
  exit 1
fi



IMGDIR="${IMAGESDIR}/${PROFILE}/${VERSION}_${FEED}"
mkdir -v -p "${IMGDIR}"
if [[ ! -d "${IMGDIR}" ]]; then
  echo "Cannot use directory ${IMGDIR}"
  exit 1
fi
# - remove any previous firmware
rm "${IMGDIR}/${FWPREFIX}"-*-fwimg-*.bin >/dev/null 2>&1
rm "${IMGDIR}/${FWPREFIX}"-*-udscript-*.sh >/dev/null 2>&1
# - checksum and copy image
CKSUM3=$(${CKSUM3TOOL} "${SRCIMAGE}")
cp "${SRCIMAGE}" "${IMGDIR}/${FWPREFIX}-${VERSION}-${FEED}-fwimg-${CKSUM3}.bin"
# - checksum and copy updater script
CKSUM3=$(${CKSUM3TOOL} "${UDSCRIPT}")
cp "${UDSCRIPT}" "${IMGDIR}/${FWPREFIX}-${VERSION}-${FEED}-udscript-${CKSUM3}.sh"

echo "created firmware components in ${IMGDIR}"

exit 0