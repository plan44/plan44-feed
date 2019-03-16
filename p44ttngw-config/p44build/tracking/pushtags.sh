#!/bin/bash
pushd /tmp; rm -rf /tmp/p44b_temp; mkdir p44b_temp; cd p44b_temp
# onion/omega2-ctrl: WARNING: There is no branch/tag currently at fa0425d861abbcc34bacd6edc3c7b531ad32c4d9. We need to checkout whole repo
git clone --no-checkout https://github.com/OnionIoT/omega2-ctrl.git omega2-ctrl
pushd omega2-ctrl
git tag -f P44TTNGW/0.3/p44ttngw-omega2 fa0425d861abbcc34bacd6edc3c7b531ad32c4d9
git push -f origin P44TTNGW/0.3/p44ttngw-omega2
popd
git clone --depth 1 --no-checkout --single-branch --branch legacy https://github.com/berkutta/lora_gateway.git lora_gateway
pushd lora_gateway
git tag -f P44TTNGW/0.3/p44ttngw-omega2 0d624e9af330915e68c5347d10d887a537aa91c0
git push -f origin P44TTNGW/0.3/p44ttngw-omega2
popd
git clone --depth 1 --no-checkout --single-branch --branch legacy https://github.com/TheThingsNetwork/packet_forwarder.git packet_forwarder
pushd packet_forwarder
git tag -f P44TTNGW/0.3/p44ttngw-omega2 90a1d7e1d2101689e4ec9b7d6bcd80f311fe9e25
git push -f origin P44TTNGW/0.3/p44ttngw-omega2
popd
popd
