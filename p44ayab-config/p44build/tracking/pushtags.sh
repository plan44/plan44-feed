#!/bin/bash
pushd /tmp; rm -rf /tmp/p44b_temp; mkdir p44b_temp; cd p44b_temp
git clone --depth 1 --no-checkout --single-branch --branch luz ssh://plan44@plan44.ch/home/plan44/sharedgit/p44ayabd.git p44ayabd
pushd p44ayabd
git tag -f P44AYAB/0.1/p44ayab-omega2 3f5027031d5da4bbf63634915141932f00e02241
git push -f origin P44AYAB/0.1/p44ayab-omega2
popd
git clone --depth 1 --no-checkout --single-branch --branch HEAD https://github.com/OnionIoT/omega2-ctrl.git omega2-ctrl
pushd omega2-ctrl
git tag -f P44AYAB/0.1/p44ayab-omega2 fa0425d861abbcc34bacd6edc3c7b531ad32c4d9
git push -f origin P44AYAB/0.1/p44ayab-omega2
popd
popd
