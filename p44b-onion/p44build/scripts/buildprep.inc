# Here you can set environment variables and perform other preparation steps for a 'buildtargets' run.
# This file is sourced into the buildtargets run. If it does not return success, buildtargets is aborted.

# Example: when running commitversion, make sure not only changes in the p44build dir, but also
#   in the enclosing dir (which is usually the "umbrella" openwrt package for the project) are committed,
#   so stage these in advance now:
#pushd ${P44B_SRC_DIR}/..  >/dev/null
#git add .
#popd  >/dev/null

# Example: get a user input needed for building and store it in a environment variable
#   for a buildtargets step, for example a passphrase for a signing key etc.
#if [ -z "${NEEDEDVAR}" ]; then
#  read -s -p "Need a user input: " NEEDEDVAR; echo
#  export NEEDEDVAR
#fi
