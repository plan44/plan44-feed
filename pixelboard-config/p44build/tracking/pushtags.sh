#!/bin/bash
pushd /tmp; rm -rf /tmp/p44b_temp; mkdir p44b_temp; cd p44b_temp
git clone --depth 1 --no-checkout --single-branch --branch ow_testing ssh://plan44@plan44.ch/home/plan44/sharedgit/mg44.git mg44
pushd mg44
git tag -f PIXELBOARD/0.37/pixelboard-omega2 87d807c0ba3b782f92dbc8bb6a0bbacaaf2c6ea0
git push -f origin PIXELBOARD/0.37/pixelboard-omega2
popd
git clone --depth 1 --no-checkout --single-branch --branch ow_testing ssh://plan44@plan44.ch/home/plan44/gitrepositories/serialfwd.git serialfwd
pushd serialfwd
git tag -f PIXELBOARD/0.37/pixelboard-omega2 e198c8ec40a31b6e39687125efa159950f55e3e5
git push -f origin PIXELBOARD/0.37/pixelboard-omega2
popd
# onion/omega2-ctrl: WARNING: There is no branch/tag currently at fa0425d861abbcc34bacd6edc3c7b531ad32c4d9. We need to checkout whole repo
git clone --no-checkout https://github.com/OnionIoT/omega2-ctrl.git omega2-ctrl
pushd omega2-ctrl
git tag -f PIXELBOARD/0.37/pixelboard-omega2 fa0425d861abbcc34bacd6edc3c7b531ad32c4d9
git push -f origin PIXELBOARD/0.37/pixelboard-omega2
popd
popd
