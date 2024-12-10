#!/bin/bash

# p44b changing firmware version

# Helpers
err()
{
  echo -e "$_ERR$@$_N"
}

warn()
{
  echo -e "$_WARN$@$_N"
}

notice()
{
  echo -e "$_NOTICE$@$_N"
}

info()
{
  echo -e "$_INFO$@$_N"
}

plain()
{
  echo -e "$_N$@"
}

die() {
  err "*** Error:" $@
  exit 1
}

# colorisation
if [ -t 1 ]; then
  if [ -z "$NO_COLOR" ]; then
    _INFO='\033[90m' # info: gray (bright black)
    _NOTICE='\033[92m' # notice: bright green
    _ERR='\033[91m' # error: bright red
    _WARN='\033[93m' # warning: bright yellow
    _N='\033[m' # normal
  fi
fi


if [ -z ${P44B_SCRIPTS_DIR+x} ]; then
  die "This script must be called via 'p44b run'"
fi

UPDATE_CONFIGS=0
UPDATE_VDCD=0
ARGS=()
for ARG in $@ ; do
  case "$ARG" in
    "-c"|"--configs") UPDATE_CONFIGS=1;;
    "-v"|"--vdcd") UPDATE_VDCD=1;;
    "-m"|"--p44mbrd") UPDATE_P44MBRD=1;;
    *) ARGS+=("$ARG");;
  esac
done

if [ -z ${ARGS} ]; then
  echo "usage: newversion [-v|--vdcd] [-m|--p44mbrd] [-c|--configs] <new version number> [<new feed>]"
  echo "usage: newversion status"
  exit 1
fi


NEWVERSION="${ARGS[0]}"
NEWFEED=""
if [[ "${#ARGS[*]}" -gt 1 ]]; then
  NEWFEED="${ARGS[1]}"
fi

P=${P44B_SRC_DIR%%-config/p44build}
PLATFORM=${P##*/}

#echo "version=${NEWVERSION}, feed=${NEWFEED}"

# configs
if [[ ${UPDATE_CONFIGS} -ne 0 ]]; then
  # p44build diffconfigs and, optionally, .config
  CONFIGS="${P44B_CONFIGS_DIR}/diffconfig"*
  if [[ -f "${BUILDROOT}/.config" ]]; then
    CONFIGS="${CONFIGS}
    ${BUILDROOT}/.config"
  else
    warn "no ${BUILDROOT}/.config"
  fi
  # iterate through diffconfigs and currently loaded config
  for CFG in ${CONFIGS}; do
    info "- ${CFG}:"
    VERSION=$(sed -E -n -e "/CONFIG_VERSION_NUMBER=/s/CONFIG_VERSION_NUMBER=\"(.*)\"/\\1/p" "${CFG}")
    FEED=$(sed -E -n -e "/CONFIG_P44_FEED_NAME=/s/CONFIG_P44_FEED_NAME=\"(.*)\"/\\1/p" "${CFG}")
    if [[ "${NEWVERSION}" = "status" ]]; then
      echo "  ${VERSION} / ${FEED}"
    else
      if [[ "${VERSION}" != "${NEWVERSION}" ]]; then
        notice "  changing version to ${NEWVERSION}"
        sed -i "" -E -e "/CONFIG_VERSION_NUMBER=/s|CONFIG_VERSION_NUMBER=\"(.*)\"$|CONFIG_VERSION_NUMBER=\"${NEWVERSION}\"|" "${CFG}"
      else
        echo "  config version is already ${NEWVERSION}"
      fi
      if [[ -n "${NEWFEED}" ]]; then
        if [[ "${FEED}" != "${NEWFEED}" ]]; then
          notice "  changing feed to ${NEWFEED}"
          sed -i "" -E -e "/CONFIG_P44_FEED_NAME=/s|CONFIG_P44_FEED_NAME=\"(.*)\"$|CONFIG_P44_FEED_NAME=\"${NEWFEED}\"|" "${CFG}"
        else
          echo "  config feed is already ${NEWFEED}"
        fi
      else
        echo "  config feed stays at ${FEED}"
      fi
    fi
  done
fi


# makefiles
MAKEFILES=()
if [[ ${UPDATE_CONFIGS} -ne 0 ]]; then
  MK_CFG="${P44B_SRC_DIR}/../Makefile"
  if [ -f "${MK_CFG}" ]; then
    MAKEFILES+=("${MK_CFG}")
  else
    warn "- NOT FOUND: ${MK_CFG}"
  fi
fi
if [[ ${UPDATE_VDCD} -ne 0 ]]; then
  MK_VDCD="${P44B_SRC_DIR}/../../vdcd/Makefile"
  if [ -f "${MK_VDCD}" ]; then
    MAKEFILES+=("${MK_VDCD}")
  else
    warn "- NOT FOUND: ${MK_VDCD}"
  fi
fi
if [[ ${UPDATE_P44MBRD} -ne 0 ]]; then
  MK_P44MBRD="${P44B_SRC_DIR}/../../p44mbrd/Makefile"
  if [ -f "${MK_P44MBRD}" ]; then
    MAKEFILES+=("${MK_P44MBRD}")
  else
    warn "- NOT FOUND: ${MK_P44MBRD}"
  fi
fi
# iterate through makefiles
for MKF in ${MAKEFILES[@]}; do
  info "- ${MKF}:"
  PKGVERSION=$(sed -E -n -e "/PKG_VERSION:=/s/PKG_VERSION:=//p" "${MKF}")
  PKGRELEASE=$(sed -E -n -e "/PKG_RELEASE:=/s/PKG_RELEASE:=//p" "${MKF}")
  if [[ "${NEWVERSION}" = "status" ]]; then
    echo "  ${PKGVERSION} / PKG_RELEASE ${PKGRELEASE}"
  elif [[ "${PKGVERSION}" != "${NEWVERSION}" ]]; then
    NEWPKGRELEASE="$(($PKGRELEASE + 1))"
    notice "  changing package version to ${NEWVERSION}"
    notice "  incrementing package release to ${NEWPKGRELEASE}"
    sed -i ".orig~" -E -e "/PKG_VERSION:=/s|PKG_VERSION:=(.*)$|PKG_VERSION:=${NEWVERSION}|" -e "/PKG_RELEASE:=/s|PKG_RELEASE:=(.*)$|PKG_RELEASE:=${NEWPKGRELEASE}|"  "${MKF}"
  else
    echo "  package version is already ${NEWVERSION} -> not incrementing package release"
  fi
done

exit 0
