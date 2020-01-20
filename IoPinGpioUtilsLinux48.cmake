#  IoPinGpioUtilsLinux48.cmake
#   Created on: 17.01.2020
#       Author: joluxer

# set sources
SET(src
    ${CMAKE_CURRENT_LIST_DIR}/IoPinGpioUtilsLinux48.cpp
    ${CMAKE_CURRENT_LIST_DIR}/gpio-utils.c
)

SET(CMAKE_INSTALL_RPATH "$ORIGIN")

# compile library as shared object to avoid license issues
ADD_LIBRARY(IoPinGpioUtilsLinux48 SHARED ${src})
#ADD_LIBRARY(IoPinGpioUtilsLinux48_static STATIC ${src})

INSTALL(TARGETS IoPinGpioUtilsLinux48 LIBRARY DESTINATION lib)

