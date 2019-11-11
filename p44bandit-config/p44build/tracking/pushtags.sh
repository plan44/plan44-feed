#!/bin/bash
pushd /tmp; rm -rf /tmp/p44b_temp; mkdir p44b_temp; cd p44b_temp
git clone --depth 1 --no-checkout --single-branch --branch master ssh://plan44@plan44.ch/home/plan44/sharedgit/mg44.git mg44
pushd mg44
git tag -f P44BANDIT/0.6/p44bandit-omega2 87d807c0ba3b782f92dbc8bb6a0bbacaaf2c6ea0
git push -f origin P44BANDIT/0.6/p44bandit-omega2
popd
git clone --depth 1 --no-checkout --single-branch --branch master ssh://plan44@plan44.ch/home/plan44/gitrepositories/serialfwd.git serialfwd
pushd serialfwd
git tag -f P44BANDIT/0.6/p44bandit-omega2 e198c8ec40a31b6e39687125efa159950f55e3e5
git push -f origin P44BANDIT/0.6/p44bandit-omega2
popd
git clone --depth 1 --no-checkout --single-branch --branch master https://github.com/plan44/p44banditd.git p44banditd
pushd p44banditd
git tag -f P44BANDIT/0.6/p44bandit-omega2 af7f6c0714b7a604297d36a2b0813b00b1a10fb4
git push -f origin P44BANDIT/0.6/p44bandit-omega2
popd
git clone --depth 1 --no-checkout --single-branch --branch HEAD https://github.com/OnionIoT/omega2-ctrl.git omega2-ctrl
pushd omega2-ctrl
git tag -f P44BANDIT/0.6/p44bandit-omega2 e7152ba869b1af10e3c4d22938100962eb1d9b0e
git push -f origin P44BANDIT/0.6/p44bandit-omega2
popd
popd
