# Note

This package already exists on OpenWrt/packages master branch,
and is copied here only until:
- it is merged into a release (24.xx?)
- P44 has updated to that release (now on 22.03)

The only change to the upstream version is
- in `rtpmidid.init`, the START line is changed to 98 (from 99)
  to make sure rtpmidid is up before P44 specifics are started
  in 99 (in particular, aconnect setup that wants to use
  rtpmidid's network ALSA port)
