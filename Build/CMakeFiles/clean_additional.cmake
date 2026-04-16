# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Build")
  file(REMOVE_RECURSE
  "AkisVG_autogen"
  "CMakeFiles/AkisVG_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/AkisVG_autogen.dir/ParseCache.txt"
  )
endif()
