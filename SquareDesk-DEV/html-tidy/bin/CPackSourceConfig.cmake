# This file will be configured to contain variables for CPack. These variables
# should be set in the CMake list file of the project before CPack module is
# included. The list of available CPACK_xxx variables and their associated
# documentation may be obtained using
#  cpack --help-variable-list
#
# Some variables are common to all generators (e.g. CPACK_PACKAGE_NAME)
# and some are specific to a generator
# (e.g. CPACK_NSIS_EXTRA_INSTALL_COMMANDS). The generator specific variables
# usually begin with CPACK_<GENNAME>_xxxx.


SET(CPACK_BINARY_7Z "")
SET(CPACK_BINARY_BUNDLE "")
SET(CPACK_BINARY_CYGWIN "")
SET(CPACK_BINARY_DEB "")
SET(CPACK_BINARY_DRAGNDROP "")
SET(CPACK_BINARY_IFW "")
SET(CPACK_BINARY_NSIS "")
SET(CPACK_BINARY_OSXX11 "")
SET(CPACK_BINARY_PACKAGEMAKER "")
SET(CPACK_BINARY_PRODUCTBUILD "")
SET(CPACK_BINARY_RPM "")
SET(CPACK_BINARY_STGZ "")
SET(CPACK_BINARY_TBZ2 "")
SET(CPACK_BINARY_TGZ "")
SET(CPACK_BINARY_TXZ "")
SET(CPACK_BINARY_TZ "")
SET(CPACK_BINARY_WIX "")
SET(CPACK_BINARY_ZIP "")
SET(CPACK_BUILD_SOURCE_DIRS "/Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/html-tidy;/Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/html-tidy/bin")
SET(CPACK_CMAKE_GENERATOR "Unix Makefiles")
SET(CPACK_COMPONENT_UNSPECIFIED_HIDDEN "TRUE")
SET(CPACK_COMPONENT_UNSPECIFIED_REQUIRED "TRUE")
SET(CPACK_DEBIAN_PACKAGE_HOMEPAGE "http://www.html-tidy.org")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "maintainer@htacg.org")
SET(CPACK_DEBIAN_PACKAGE_SECTION "Libraries")
SET(CPACK_GENERATOR "TGZ")
SET(CPACK_IGNORE_FILES "/Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/html-tidy/test/;/Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/html-tidy/build/;/Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/html-tidy/.git/")
SET(CPACK_INSTALLED_DIRECTORIES "/Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/html-tidy;/")
SET(CPACK_INSTALL_CMAKE_PROJECTS "")
SET(CPACK_INSTALL_PREFIX "/usr/local")
SET(CPACK_MODULE_PATH "")
SET(CPACK_NSIS_DISPLAY_NAME "tidy 5.5.31")
SET(CPACK_NSIS_INSTALLER_ICON_CODE "")
SET(CPACK_NSIS_INSTALLER_MUI_ICON_CODE "")
SET(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES")
SET(CPACK_NSIS_PACKAGE_NAME "tidy 5.5.31")
SET(CPACK_OSX_SYSROOT "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.12.sdk")
SET(CPACK_OUTPUT_CONFIG_FILE "/Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/html-tidy/bin/CPackConfig.cmake")
SET(CPACK_PACKAGE_CONTACT "maintainer@htacg.org")
SET(CPACK_PACKAGE_DEFAULT_LOCATION "/")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "/Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/html-tidy/README/README.html")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "tidy - HTML syntax checker")
SET(CPACK_PACKAGE_FILE_NAME "tidy-5.5.31-Source")
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "tidy 5.5.31")
SET(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "tidy 5.5.31")
SET(CPACK_PACKAGE_NAME "tidy")
SET(CPACK_PACKAGE_RELOCATABLE "true")
SET(CPACK_PACKAGE_VENDOR "HTML Tidy Advocacy Community Group")
SET(CPACK_PACKAGE_VERSION "5.5.31")
SET(CPACK_PACKAGE_VERSION_MAJOR "5")
SET(CPACK_PACKAGE_VERSION_MINOR "5")
SET(CPACK_PACKAGE_VERSION_PATCH "31")
SET(CPACK_RESOURCE_FILE_LICENSE "/Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/html-tidy/README/LICENSE.txt")
SET(CPACK_RESOURCE_FILE_README "/Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/html-tidy/README/README.html")
SET(CPACK_RESOURCE_FILE_WELCOME "/Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/html-tidy/README/README.html")
SET(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "/usr/share/man;/usr/share/man/man1")
SET(CPACK_RPM_PACKAGE_SOURCES "ON")
SET(CPACK_SET_DESTDIR "OFF")
SET(CPACK_SOURCE_7Z "")
SET(CPACK_SOURCE_CYGWIN "")
SET(CPACK_SOURCE_GENERATOR "TGZ")
SET(CPACK_SOURCE_IGNORE_FILES "/Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/html-tidy/test/;/Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/html-tidy/build/;/Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/html-tidy/.git/")
SET(CPACK_SOURCE_INSTALLED_DIRECTORIES "/Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/html-tidy;/")
SET(CPACK_SOURCE_OUTPUT_CONFIG_FILE "/Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/html-tidy/bin/CPackSourceConfig.cmake")
SET(CPACK_SOURCE_PACKAGE_FILE_NAME "tidy-5.5.31-Source")
SET(CPACK_SOURCE_RPM "")
SET(CPACK_SOURCE_TBZ2 "")
SET(CPACK_SOURCE_TGZ "")
SET(CPACK_SOURCE_TOPLEVEL_TAG "Darwin-Source")
SET(CPACK_SOURCE_TXZ "")
SET(CPACK_SOURCE_TZ "")
SET(CPACK_SOURCE_ZIP "")
SET(CPACK_STRIP_FILES "")
SET(CPACK_SYSTEM_NAME "Darwin")
SET(CPACK_TOPLEVEL_TAG "Darwin-Source")
SET(CPACK_WIX_SIZEOF_VOID_P "8")

if(NOT CPACK_PROPERTIES_FILE)
  set(CPACK_PROPERTIES_FILE "/Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/html-tidy/bin/CPackProperties.cmake")
endif()

if(EXISTS ${CPACK_PROPERTIES_FILE})
  include(${CPACK_PROPERTIES_FILE})
endif()
