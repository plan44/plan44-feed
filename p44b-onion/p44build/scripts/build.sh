# Speed up make world
echo "Note: using all but one of the available processors/cores for building"
# prefer gmake over make (because latter might be outdated on Apple)
MAKE=$(which gmake)
if [[ $? -ne 0 ]]; then
  MAKE=$(which make)
fi
${MAKE} -j$(($(nproc)-1))
