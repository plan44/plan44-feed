#!/bin/bash
pushd /tmp; rm -rf /tmp/p44b_temp; mkdir p44b_temp; cd p44b_temp
git clone --depth 1 --no-checkout --single-branch --branch ow_testing ssh://plan44@plan44.ch/home/plan44/sharedgit/mg44.git mg44
pushd mg44
git tag -f P44BANDIT/0.4/p44bandit-omega2 4edf2903b6c77c92c84d3a7bbfd3d19530df6cab
git push -f origin P44BANDIT/0.4/p44bandit-omega2
popd
git clone --depth 1 --no-checkout --single-branch --branch ow_testing ssh://plan44@plan44.ch/home/plan44/gitrepositories/serialfwd.git serialfwd
pushd serialfwd
git tag -f P44BANDIT/0.4/p44bandit-omega2 e198c8ec40a31b6e39687125efa159950f55e3e5
git push -f origin P44BANDIT/0.4/p44bandit-omega2
popd
git clone --depth 1 --no-checkout --single-branch --branch master https://github.com/plan44/p44banditd.git p44banditd
pushd p44banditd
git tag -f P44BANDIT/0.4/p44bandit-omega2 554a2eb5c11d3f1b8a8a0e276a97939a9c062ff7
git push -f origin P44BANDIT/0.4/p44bandit-omega2
popd
git clone --depth 1 --no-checkout --single-branch --branch HEAD https://github.com/OnionIoT/omega2-ctrl.git omega2-ctrl
pushd omega2-ctrl
git tag -f P44BANDIT/0.4/p44bandit-omega2 fa0425d861abbcc34bacd6edc3c7b531ad32c4d9
git push -f origin P44BANDIT/0.4/p44bandit-omega2
popd
popd
