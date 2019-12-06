#!/bin/bash
pushd /tmp; rm -rf /tmp/p44b_temp; mkdir p44b_temp; cd p44b_temp
git clone --depth 1 --no-checkout --single-branch --branch master ssh://plan44@plan44.ch/home/plan44/sharedgit/mg44.git mg44
pushd mg44
git tag -f P44-Audiobox/1.0/p44audiobox-raspberrypi-1 87d807c0ba3b782f92dbc8bb6a0bbacaaf2c6ea0
git push -f origin P44-Audiobox/1.0/p44audiobox-raspberrypi-1
popd
git clone --depth 1 --no-checkout --single-branch --branch master ssh://plan44@plan44.ch/home/plan44/gitrepositories/serialfwd.git serialfwd
pushd serialfwd
git tag -f P44-Audiobox/1.0/p44audiobox-raspberrypi-1 e198c8ec40a31b6e39687125efa159950f55e3e5
git push -f origin P44-Audiobox/1.0/p44audiobox-raspberrypi-1
popd
popd
