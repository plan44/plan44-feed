#!/bin/bash
pushd /tmp; rm -rf /tmp/p44b_temp; mkdir p44b_temp; cd p44b_temp
git clone --depth 1 --no-checkout --single-branch --branch ow_testing ssh://plan44@plan44.ch/home/plan44/sharedgit/mg44.git mg44
pushd mg44
git tag -f LETH/0.03/leth-omega2 6df3c33a2d0911f67fe1cf2313aa8cd79a74546e
git push -f origin LETH/0.03/leth-omega2
popd
git clone --depth 1 --no-checkout --single-branch --branch master https://github.com/plan44/lethd.git lethd
pushd lethd
git tag -f LETH/0.03/leth-omega2 8a27889517089debd014cd66e5045e040df61d48
git push -f origin LETH/0.03/leth-omega2
popd
git clone --depth 1 --no-checkout --single-branch --branch HEAD https://github.com/OnionIoT/omega2-ctrl.git omega2-ctrl
pushd omega2-ctrl
git tag -f LETH/0.03/leth-omega2 fa0425d861abbcc34bacd6edc3c7b531ad32c4d9
git push -f origin LETH/0.03/leth-omega2
popd
popd
