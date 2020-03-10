#!/bin/bash
pushd /tmp; rm -rf /tmp/p44b_temp; mkdir p44b_temp; cd p44b_temp
git clone --depth 1 --no-checkout --single-branch --branch p44featured ssh://plan44@plan44.ch/home/plan44/sharedgit/p44featured.git p44featured
pushd p44featured
git tag -f P44-RFIDCTRL/0.2/p44rfidctrl-omega2 0f5bf32b985a4664a5d17eb63bf9bcf989bac031
git push -f origin P44-RFIDCTRL/0.2/p44rfidctrl-omega2
popd
git clone --depth 1 --no-checkout --single-branch --branch HEAD https://github.com/OnionIoT/omega2-ctrl.git omega2-ctrl
pushd omega2-ctrl
git tag -f P44-RFIDCTRL/0.2/p44rfidctrl-omega2 e7152ba869b1af10e3c4d22938100962eb1d9b0e
git push -f origin P44-RFIDCTRL/0.2/p44rfidctrl-omega2
popd
popd
