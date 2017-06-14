#!/bin/bash
pushd /tmp; rm -rf /tmp/p44b_temp; mkdir p44b_temp; cd p44b_temp
git clone --depth 1 --no-checkout --single-branch --branch ow_testing ssh://plan44@plan44.ch/home/plan44/gitrepositories/serialfwd.git serialfwd
pushd serialfwd
git tag p44bandit-omega2_0.01 e198c8ec40a31b6e39687125efa159950f55e3e5
git push origin p44bandit-omega2_0.01
popd
git clone --depth 1 --no-checkout --single-branch --branch HEAD git@github.com:OnionIoT/omega2-ctrl.git omega2-ctrl
pushd omega2-ctrl
git tag p44bandit-omega2_0.01 fa0425d861abbcc34bacd6edc3c7b531ad32c4d9
git push origin p44bandit-omega2_0.01
popd
popd
