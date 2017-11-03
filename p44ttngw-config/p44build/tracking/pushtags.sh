#!/bin/bash
pushd /tmp; rm -rf /tmp/p44b_temp; mkdir p44b_temp; cd p44b_temp
git clone --depth 1 --no-checkout --single-branch --branch HEAD https://github.com/OnionIoT/omega2-ctrl.git omega2-ctrl
pushd omega2-ctrl
git tag -f P44TTNGW/0.2/p44ttngw-omega2 fa0425d861abbcc34bacd6edc3c7b531ad32c4d9
git push -f origin P44TTNGW/0.2/p44ttngw-omega2
popd
git clone --depth 1 --no-checkout --single-branch --branch legacy https://github.com/berkutta/lora_gateway.git lora_gateway
pushd lora_gateway
git tag -f P44TTNGW/0.2/p44ttngw-omega2 0d624e9af330915e68c5347d10d887a537aa91c0
git push -f origin P44TTNGW/0.2/p44ttngw-omega2
popd
git clone --depth 1 --no-checkout --single-branch --branch legacy https://github.com/TheThingsNetwork/packet_forwarder.git packet_forwarder
pushd packet_forwarder
git tag -f P44TTNGW/0.2/p44ttngw-omega2 884bb13504137bb2af32ce487698d30a10952c86
git push -f origin P44TTNGW/0.2/p44ttngw-omega2
popd
popd
