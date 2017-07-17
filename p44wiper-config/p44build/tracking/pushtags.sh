#!/bin/bash
pushd /tmp; rm -rf /tmp/p44b_temp; mkdir p44b_temp; cd p44b_temp
git clone --depth 1 --no-checkout --single-branch --branch ow_testing ssh://plan44@plan44.ch/home/plan44/sharedgit/mg44.git mg44
pushd mg44
git tag -f P44WIPER/0.01/p44wiper-omega2 4edf2903b6c77c92c84d3a7bbfd3d19530df6cab
git push -f origin P44WIPER/0.01/p44wiper-omega2
popd
git clone --depth 1 --no-checkout --single-branch --branch ow_testing ssh://plan44@plan44.ch/home/plan44/gitrepositories/serialfwd.git serialfwd
pushd serialfwd
git tag -f P44WIPER/0.01/p44wiper-omega2 e198c8ec40a31b6e39687125efa159950f55e3e5
git push -f origin P44WIPER/0.01/p44wiper-omega2
popd
git clone --depth 1 --no-checkout --single-branch --branch HEAD git@github.com:OnionIoT/omega2-ctrl.git omega2-ctrl
pushd omega2-ctrl
git tag -f P44WIPER/0.01/p44wiper-omega2 fa0425d861abbcc34bacd6edc3c7b531ad32c4d9
git push -f origin P44WIPER/0.01/p44wiper-omega2
popd
popd
