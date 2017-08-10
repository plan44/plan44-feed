#!/bin/bash
pushd /tmp; rm -rf /tmp/p44b_temp; mkdir p44b_temp; cd p44b_temp
git clone --depth 1 --no-checkout --single-branch --branch master https://github.com/plan44/pixelboardd.git pixelboardd
pushd pixelboardd
git tag -f PIXELBOARD/0.21/pixelboard-omega2 a900cc00041badbc3dfe99b69497d86cda63e6e9
git push -f origin PIXELBOARD/0.21/pixelboard-omega2
popd
git clone --depth 1 --no-checkout --single-branch --branch master https://github.com/plan44/mg44.git mg44
pushd mg44
git tag -f PIXELBOARD/0.21/pixelboard-omega2 4edf2903b6c77c92c84d3a7bbfd3d19530df6cab
git push -f origin PIXELBOARD/0.21/pixelboard-omega2
popd
git clone --depth 1 --no-checkout --single-branch --branch HEAD https://github.com/OnionIoT/omega2-ctrl.git omega2-ctrl
pushd omega2-ctrl
git tag -f PIXELBOARD/0.21/pixelboard-omega2 fa0425d861abbcc34bacd6edc3c7b531ad32c4d9
git push -f origin PIXELBOARD/0.21/pixelboard-omega2
popd
popd
