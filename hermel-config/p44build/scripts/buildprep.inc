# make sure feed changes get committed
pushd ${P44B_SRC_DIR}/../..  >/dev/null
git add .
popd  >/dev/null

# set some env vars
P44B_BUILD_ARGS=
P44B_PACKAGE_ARGS=
P44B_DEPLOY_ARGS=
P44B_TAGVERSION_ARGS="--packages"
P44B_PUSHVERSION_ARGS="origin"
