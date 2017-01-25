# plan44 feed for OpenWrt/LEDE

This is my source feed for building various LEDE/OpenWrt packages I wrote for projects on Onion Omega1 and Omega2.

If you have a LEDE or OpenWrt build environment set up, you can use the plan44 feed by adding the line

    src-git plan44 ssh://plan44@plan44.ch/home/plan44/sharedgit/plan44-public-feed.git;master
    
to your *feeds.conf.default*

## Work in progress!

At this time, there's a lot of work-in-progress in this feed, so don't expect everything turnkey ready. In particular, the p44sbbd, pixelboard and pixelboard-config packages are parts of ongoing projects not ready for prime time.

Note that *i2c-tools* and *libpng* are available in the standard feeds, but in slightly different variations - I needed some modifications for my own projects so I duplicated the packages. Fortunately, OpenWrt/LEDE source feed management is prepared for having multiple versions of the same package - just use the -p option to choose the feed to install a particular package from:

    ./scripts/feeds update plan44
    ./scripts/feeds install -p plan44 libpng
 

