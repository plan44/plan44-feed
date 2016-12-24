#!/bin/sh

# Update script for OpenWRT based P44-DSB-E2

# debug
echo "P44_FWKEY    : ${P44_FWKEY}"
echo "P44_VERSION  : ${P44_VERSION}"
echo "P44_FEED     : ${P44_FEED}"
echo "P44_GTIN     : ${P44_GTIN}"
echo "P44_MODEL    : ${P44_MODEL}"
echo "P44_VARIANT  : ${P44_VARIANT}"
echo "P44_PLATFORM : ${P44_PLATFORM}"

if [ $# -eq 0 ]; then

  # first phase, prepare and download

  # to make sure - stop services (should already be stopped)
  sv stop mg44 vdcd

  # delete logs to make room
  rm /var/log/vdcd/current
  rm /var/log/vdcd/@*

  # make sure no previous firmware images or packages occupy space
  rm /tmp/fwimg.bin
  rm /tmp/*.ipk

  # create the marker signalling we have started a firmware upgrade process
  touch /flash/fw_updated; sync

  # download the rootfs image
  CKSUM3=$(p44maintd -l 5 --hexsum --download fwimg --firmwarekey ${P44_FWKEY} --file /tmp/fwimg.bin)
  if [ $? -eq 0 ]; then
    echo "CKSUM3       : 0x${CKSUM3}"
    # installation package download successful and with correct CRC
    # - request second phase
    exit 2
  else
    echo "Download of fwimg failed"
    /usr/bin/p44maintd --reporterror "download of fwimg failed"
    exit 1
  fi

elif [ "$1" = "exec" ]; then

  # second phase, actual upgrade
  # - configure LEDs to blink during upgrade
  echo 1 >/sys/class/leds/p44:red:status2/brightness
  echo timer >/sys/class/leds/p44:green:status1/trigger
  echo 200 >/sys/class/leds/p44:green:status1/delay_on
  echo 50 >/sys/class/leds/p44:green:status1/delay_off
  # - actually install
  cd /tmp
  sysupgrade fwimg.bin
  if [[ $? != 0 ]]; then
    # failed, report
    /usr/bin/p44maintd --reporterror "sysupgrade failed"
    exit 1
  fi
  # in case we get here (normally not, sysupgrade does not terminate when successful
  reboot
  exit 0

else
  echo "Update script must be called without parameters (phase 1) or with 1 param 'exec' (phase 2)"
fi
