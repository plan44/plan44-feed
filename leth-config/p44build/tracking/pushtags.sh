#!/bin/bash
pushd /tmp; rm -rf /tmp/p44b_temp; mkdir p44b_temp; cd p44b_temp
git clone --depth 1 --no-checkout --single-branch --branch master https://github.com/plan44/lethd.git lethd
pushd lethd
git tag -f LETH/1.8/leth-omega2 a730417861b3dc718fc2d888a55fe8d5d5efe170
git push -f origin LETH/1.8/leth-omega2
popd
git clone --depth 1 --no-checkout --single-branch --branch HEAD https://github.com/OnionIoT/omega2-ctrl.git omega2-ctrl
pushd omega2-ctrl
git tag -f LETH/1.8/leth-omega2 fa0425d861abbcc34bacd6edc3c7b531ad32c4d9
git push -f origin LETH/1.8/leth-omega2
popd
popd
