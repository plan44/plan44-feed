#!/bin/bash

# clean the PATH from dumb utils like little snitch that force-place entries with spaces in there
export PATH=$(echo $PATH | sed -E "s/(:[^: ]* [^:]*):/:/")

gmake $@
