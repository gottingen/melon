include(GNUInstallDirs)

# abel_VERSION is only set if we are an LTS release being installed, in which
# case it may be into a system directory and so we need to make subdirectories
# for each installed version of abel.  This mechanism is implemented in
# abel's internal Copybara (https://github.com/google/copybara) workflows and
# isn't visible in the CMake buildsystem itself.

if(abel_VERSION)
  set(ABEL_SUBDIR "${PROJECT_NAME}_${PROJECT_VERSION}")
  set(ABEL_INSTALL_BINDIR "${CMAKE_INSTALL_BINDIR}/${ABEL_SUBDIR}")
  set(ABEL_INSTALL_CONFIGDIR "${CMAKE_INSTALL_LIBDIR}/cmake/${ABEL_SUBDIR}")
  set(ABEL_INSTALL_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}/{ABEL_SUBDIR}")
  set(ABEL_INSTALL_LIBDIR "${CMAKE_INSTALL_LIBDIR}/${ABEL_SUBDIR}")
else()
  set(ABEL_INSTALL_BINDIR "${CMAKE_INSTALL_BINDIR}")
  set(ABEL_INSTALL_CONFIGDIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")
  set(ABEL_INSTALL_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}")
  set(ABEL_INSTALL_LIBDIR "${CMAKE_INSTALL_LIBDIR}")
endif()