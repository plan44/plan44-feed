#!/bin/bash
pushd /tmp; rm -rf /tmp/p44b_temp; mkdir p44b_temp; cd p44b_temp
git clone --depth 1 --no-checkout --single-branch --branch master ssh://plan44@plan44.ch/home/plan44/gitrepositories/serialfwd.git serialfwd
pushd serialfwd
git tag -f KKS-DCM/1.0.0/kksdcm-omega2 e198c8ec40a31b6e39687125efa159950f55e3e5
git push -f origin KKS-DCM/1.0.0/kksdcm-omega2
popd
git clone --depth 1 --no-checkout --single-branch --branch master ssh://plan44@plan44.ch/home/plan44/sharedgit/kksdcmd.git kksdcmd
pushd kksdcmd
git tag -f KKS-DCM/1.0.0/kksdcm-omega2 d49d4655110c6d243ba38ac0d216053276e59a55
git push -f origin KKS-DCM/1.0.0/kksdcm-omega2
popd
git clone --depth 1 --no-checkout --single-branch --branch HEAD https://github.com/OnionIoT/omega2-ctrl.git omega2-ctrl
pushd omega2-ctrl
git tag -f KKS-DCM/1.0.0/kksdcm-omega2 e7152ba869b1af10e3c4d22938100962eb1d9b0e
git push -f origin KKS-DCM/1.0.0/kksdcm-omega2
popd
popd
