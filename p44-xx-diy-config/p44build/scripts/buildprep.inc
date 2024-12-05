# make sure umbrella package changes get committed
pushd ${P44B_SRC_DIR}/..  >/dev/null
git add .
popd  >/dev/null
# (in addition, tracked packages' Makefiles will be added via commitversion --freeze below)

# set defaults for some env vars
if [ -z ${P44B_BUILD_ARGS+x} ]; then
  P44B_BUILD_ARGS="-j$(($(nproc)-1))"
	#P44B_BUILD_ARGS=
fi
if [ -z ${P44B_PACKAGE_ARGS+x} ]; then
	P44B_PACKAGE_ARGS=
fi
if [ -z ${P44B_DEPLOY_ARGS+x} ]; then
	P44B_DEPLOY_ARGS=
fi
if [ -z ${P44B_COMMITVERSION_ARGS+x} ]; then
  P44B_COMMITVERSION_ARGS="--freeze"
fi
if [ -z ${P44B_TAGVERSION_ARGS+x} ]; then
  P44B_TAGVERSION_ARGS="--packages"
fi
if [ -z ${P44B_PUSHVERSION_ARGS+x} ]; then
	P44B_PUSHVERSION_ARGS="origin"
fi

# ready to go
