#!/bin/bash
pushd /tmp >/dev/null; rm -rf /tmp/p44b_temp; mkdir p44b_temp; cd p44b_temp
echo "vdcd-2.8.2.0: cannot push to http URL https://github.com/plan44/vdcd.git -> cannot tag package sources"
echo "p44mbrd-1.3.0.10: cannot push to http URL https://github.com/plan44/p44mbrd.git -> cannot tag package sources"
echo "p44maintd-4.5.1: cannot push to http URL https://github.com/plan44/p44maintd.git -> cannot tag package sources"
echo "mg44-3.14: cannot push to http URL https://github.com/plan44/mg44.git -> cannot tag package sources"
echo "serialfwd-1.3: cannot push to http URL https://github.com/plan44/serialfwd.git -> cannot tag package sources"
popd >/dev/null
