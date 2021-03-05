#!/bin/bash
pushd /tmp; rm -rf /tmp/p44b_temp; mkdir p44b_temp; cd p44b_temp
git clone --depth 1 --no-checkout --single-branch --branch master https://github.com/plan44/p44featured.git p44featured
pushd p44featured
git tag -f P44-RFIDCTRL/1.1/p44rfidctrl-omega2 676a7ae7762c203b572b0657557e9325f8a54b25
git push -f origin P44-RFIDCTRL/1.1/p44rfidctrl-omega2
popd
git clone --depth 1 --no-checkout --single-branch --branch HEAD https://github.com/OnionIoT/omega2-ctrl.git omega2-ctrl
pushd omega2-ctrl
git tag -f P44-RFIDCTRL/1.1/p44rfidctrl-omega2 e7152ba869b1af10e3c4d22938100962eb1d9b0e
git push -f origin P44-RFIDCTRL/1.1/p44rfidctrl-omega2
popd
popd
