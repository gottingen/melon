

include(abel_debug)
include(abel_platform)

set(ABEL_GENERATOR "TGZ")

abel_print("on platform ${CMAKE_HOST_SYSTEM_NAME} package type tgz")

string(TOLOWER ${CMAKE_HOST_SYSTEM_NAME} HOST_SYSTEM_NAME)

if(SYSTEM_NAME MATCHES "centos")
    set(ABEL_GENERATOR "TGZ;RPM")
    include(abel_package_rpm)
    string(REGEX MATCH "([0-9])" ELV "${LINUX_VER}")
    set(HOST_SYSTEM_NAME el${CMAKE_MATCH_1})
elseif(SYSTEM_NAME MATCHES "rhel")
    set(ABEL_GENERATOR "TGZ;RPM")
    include(abel_package_rpm)
    string(REGEX MATCH "([0-9])" ELV "${LINUX_VER}")
    set(HOST_SYSTEM_NAME el${CMAKE_MATCH_1})
endif()
