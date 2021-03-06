#=============================================================================
#  MusE
#  Linux Music Editor
#  $Id:$
#
#  Copyright (C) 1999-2011 by Werner Schweer and others
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the
#  Free Software Foundation, Inc.,
#  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
#=============================================================================

include(FindPkgConfig)
include(CheckIncludeFile)
include(CheckIncludeFiles)
include (CheckCSourceCompiles)
include(cmake/Summary.cmake)

project(muse)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.1)
if (COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
    cmake_policy (SET CMP0007 NEW)
endif(COMMAND cmake_policy)

if (NOT DEFINED LIB_SUFFIX)
        SET(LIB_SUFFIX "" CACHE STRING "Suffix for installed library path. Ex. 64 for lib64")
endif (NOT DEFINED LIB_SUFFIX)

SET(VST_SDK_QUIRK "")
if (NOT DEFINED VST_HEADER_PATH)
        SET(VST_HEADER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/vestige" CACHE PATH "Path to vst header files (aeffectx.h). Default is ./vestige. See ENABLE_VST_VESTIGE.")
else (NOT DEFINED VST_HEADER_PATH)
    if (NOT APPLE)
        if (UNIX)
            SET(VST_SDK_QUIRK "-DVST_SDK_QUIRK") # "Building with real vstsdk under Linux requires a quirk")
        endif (UNIX)
    endif (NOT APPLE)
endif (NOT DEFINED VST_HEADER_PATH)
message(" vst header path: " ${VST_HEADER_PATH} )
include_directories(${VST_HEADER_PATH})

# set libdir if not use -DLIB_INSTALL_DIR
if (NOT LIB_INSTALL_DIR)
        if(CMAKE_INSTALL_LIBDIR)
                SET(LIB_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR})
        else(CMAKE_INSTALL_LIBDIR)
                SET(LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}")
        endif(CMAKE_INSTALL_LIBDIR)
endif (NOT LIB_INSTALL_DIR)

if (LIB_SUFFIX)
        message(" Install libraries to: " ${LIB_INSTALL_DIR} )
endif (LIB_SUFFIX)

IF(NOT DEFINED SHARE_INSTALL_PREFIX)
        SET(SHARE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}/share")
ENDIF(NOT DEFINED SHARE_INSTALL_PREFIX)

#set(CMAKE_BUILD_TYPE debug)
#set(CMAKE_BUILD_TYPE release)

# If no CMAKE_BUILD_TYPE is given on the command line,
#  cmake either uses the cached value, or 'empty' (plain un-opt build).
# And yet the only way a user can reset a cached CMAKE_BUILD_TYPE
#  is with "-DCMAKE_BUILD_TYPE=". So we cannot interfere with this.
# We should probably not attempt to do this at all.
# Installation instructions now updated to be more specific, give options.
#endif (NOT DEFINED CMAKE_BUILD_TYPE)

#UPDATE: most of users even don't know what CMAKE_BUILD_TYPE is.
#So it will be better to set unspecified build type to RelWithDebInfo
if (NOT CMAKE_BUILD_TYPE)
    message("No CMAKE_BUILD_TYPE specified. Setting to RelWithDebInfo.")
     set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif (NOT CMAKE_BUILD_TYPE)


# for debugging the make system uncomment next line:
#set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_INCLUDE_CURRENT_DIR TRUE)
set(CMAKE_SKIP_RULE_DEPENDENCY TRUE)

# The MusE version number.
SET(MusE_VERSION_MAJOR 3)
SET(MusE_VERSION_MINOR 0)
SET(MusE_VERSION_PATCH 0)
SET(MusE_VERSION       "3.0.0pre1")
SET(MusE_VERSION_FULL  "3.0.0pre1")
SET(MusE_INSTALL_NAME  "muse-3.0")
SET(MusE_EXEC_NAME     "muse3")

## The directory where we will install the shared components:
SET(MusE_MODULES_DIR ${LIB_INSTALL_DIR}/${MusE_INSTALL_NAME}/modules)

## Synthesizers directory
SET(MusE_SYNTHI_DIR ${LIB_INSTALL_DIR}/${MusE_INSTALL_NAME}/synthi)

## Plugins directory
SET(MusE_PLUGINS_DIR ${LIB_INSTALL_DIR}/${MusE_INSTALL_NAME}/plugins)

## Top documentation dir
IF(NOT DEFINED MusE_DOC_DIR)
      SET(MusE_DOC_DIR ${SHARE_INSTALL_PREFIX}/doc/${MusE_INSTALL_NAME}/)
ENDIF(NOT DEFINED MusE_DOC_DIR)

## Data directory
SET(MusE_SHARE_DIR ${SHARE_INSTALL_PREFIX}/${MusE_INSTALL_NAME})

## Lib directory
SET(MusE_LIB_DIR ${LIB_INSTALL_DIR}/${MusE_INSTALL_NAME})

include(FindGit)
if (GIT_FOUND)
      EXEC_PROGRAM( ${GIT_EXECUTABLE}
            ARGS "status"
            OUTPUT_VARIABLE GIT_RET_NOTUSED
            RETURN_VALUE IS_GIT_REPO )
    if(IS_GIT_REPO EQUAL 0)
      EXEC_PROGRAM( ${GIT_EXECUTABLE}
            ARGS "log -1 --format='%ci'"
            OUTPUT_VARIABLE MusE_GITDATE )
      EXEC_PROGRAM( ${GIT_EXECUTABLE}
            ARGS "log -1 --format='%t'"
            OUTPUT_VARIABLE MusE_GITHASH )
      EXEC_PROGRAM( ${GIT_EXECUTABLE}
            ARGS "rev-parse --abbrev-ref HEAD"
            OUTPUT_VARIABLE MusE_GITBRANCH )
      SET(MusE_GITSTRING "git: ${MusE_GITBRANCH} - ${MusE_GITHASH} - ${MusE_GITDATE}")
    else (IS_GIT_REPO EQUAL 0)
        SET(MusE_GITSTRING "")
    endif (IS_GIT_REPO EQUAL 0)
else (GIT_FOUND)
      SET(MusE_GITSTRING "")
endif (GIT_FOUND)


option ( ENABLE_LASH        "Enable LASH Audio Session Handler (or LADISH compatibility layer)"     ON)
option ( ENABLE_OSC         "Enable Lightweight Open Sound Control (liblo) (DSSI also recommended)"  ON)
option ( ENABLE_DSSI        "Enable Disposable Soft Synth Interface (dssi) (OSC also needed)" ON)
option ( ENABLE_VST         "Enable VST/win support (deprecated)"                                OFF)
option ( ENABLE_VST_NATIVE  "Enable Native VST support (see ENABLE_VST_VESTIGE and VST_HEADER_PATH)" ON)
option ( ENABLE_VST_VESTIGE "Set VST header type is Vestige"                                         ON)
option ( ENABLE_LV2        "Enable LV2 plugins and synths support" ON)
option ( ENABLE_LV2_SUPPLIED "Use supplied or system LV2 libraries (lv2, lilv, sord)" OFF)
option ( ENABLE_LV2_DEBUG        "Enable LV2 debugging messages" OFF)
option ( ENABLE_FLUID       "Enable fluidsynth softsynth plugins."                                ON)
option ( ENABLE_EXPERIMENTAL "Enable various experimental features (not recommended)."              OFF)
option ( ENABLE_PYTHON      "Enable experimental python control support (not recommended)."         OFF)
option ( UPDATE_TRANSLATIONS "Update source translation share/locale/*.ts files (WARNING: This will modify the .ts files in the source tree!!)" OFF)
option ( MODULES_BUILD_STATIC "Build type of internal modules"                                   OFF)

if ( MODULES_BUILD_STATIC )
      SET(MODULES_BUILD STATIC )
else ( MODULES_BUILD_STATIC )
      SET(MODULES_BUILD SHARED )
      SET(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
      SET(CMAKE_INSTALL_RPATH ${MusE_MODULES_DIR})
endif ( MODULES_BUILD_STATIC )

##
## Just print a notice if this is OS X
##
if (APPLE)
	message("OS X found.")
else (APPLE)
	if (UNIX)
		message("Unix (probably linux) found")
	endif(UNIX)

endif (APPLE)

##
##  look for Qt5
##


set(QT_MIN_VERSION "5.1.0")
set(QT_USE_QTXML TRUE)
set(QT_USE_QTDESIGNER TRUE)
set(QT_USE_QTSVG TRUE)

find_package(Qt5Widgets)

if (NOT Qt5Widgets_FOUND)
      message(FATAL_ERROR "Fatal error: QT (version >= 5.1.0) required.\n"
      "Cmake tries to detect QT5 by searching for 'Qt5Widgets' package\n"
      "If you have QT5 installed, make sure 'Qt5Widgets' package is in PKG_CONFIG_PATH."
      )
endif (NOT Qt5Widgets_FOUND)

find_package(Qt5Core REQUIRED)
find_package(Qt5UiTools REQUIRED)
find_package(Qt5LinguistTools REQUIRED)
find_package(Qt5Xml REQUIRED)

# Needed for plugins factory:
SET(QT_USE_QTUITOOLS TRUE)

include_directories (${Qt5Widgets_INCLUDE_DIRS})
include_directories (${Qt5Core_INCLUDE_DIRS})
include_directories (${Qt5UiTools_INCLUDE_DIRS})
include_directories (${Qt5Xml_INCLUDE_DIRS})

LINK_DIRECTORIES(${Qt5Widgets_LIBRARY_DIRS})
LINK_DIRECTORIES(${Qt5Core_LIBRARY_DIRS})
LINK_DIRECTORIES(${Qt5UiTools_LIBRARY_DIRS})
LINK_DIRECTORIES(${Qt5Xml_LIBRARY_DIRS})

SET(QT_LIBRARIES ${Qt5Widgets_LIBRARIES} ${Qt5UiTools_LIBRARIES} ${Qt5Xml_LIBRARIES} ${Qt5Core_LIBRARIES})


##include(${QT_USE_FILE})

##
##   Begin MANDATORY packages...
##

##
## find ladspa.h
##

CHECK_INCLUDE_FILE(ladspa.h HAVE_LADSPA_H)
if(NOT HAVE_LADSPA_H)
      message(FATAL_ERROR "** ERROR: header file ladspa.h is required, but was not found."
        ${HAVE_LADSPA_H})
endif(NOT HAVE_LADSPA_H)

##
## alsa >= 0.9.0
##

if (APPLE)
	message("Disabling ALSA support due to OS X build.")
else (APPLE)
PKG_CHECK_MODULES(ALSA REQUIRED alsa>=0.9.0)
include_directories(${ALSA_INCLUDE_DIRS})
endif (APPLE)

##
## find sndfile >= 1.0.0
##

PKG_CHECK_MODULES(SNDFILE REQUIRED sndfile>=1.0.0)
include_directories(${SNDFILE_INCLUDE_DIRS})

##
## find libsamplerate >= 0.1.0
##

PKG_CHECK_MODULES(SAMPLERATE REQUIRED samplerate>=0.1.0)
include_directories(${SAMPLERATE_INCLUDE_DIRS})

##
## find libuuid
##

PKG_CHECK_MODULES(UUID REQUIRED uuid>=0.0.1)
include_directories(${UUID_INCLUDE_DIRS})

##
## find jack >= 0.103.0
##

PKG_CHECK_MODULES(JACK REQUIRED jack>=0.103)
include_directories(${JACK_INCLUDE_DIRS})

##
##   End MANDATORY packages.
##

##
##   Begin OPTIONAL packages...
##

##
## find LASH
##

if (ENABLE_LASH)
      PKG_CHECK_MODULES(LASH lash-1.0>=0.2)
      if (LASH_FOUND)
            include_directories(${LASH_INCLUDE_DIRS})
            set(HAVE_LASH ON)
      endif (LASH_FOUND)
else (ENABLE_LASH)
      message("LASH disabled")
endif (ENABLE_LASH)

##
## check for python >= 2.4
##

if (ENABLE_PYTHON)
      include(FindPythonLibs)
      if (PYTHONLIBS_FOUND)
# Older cmake does not define PYTHON_INCLUDE_DIRS and PYTHONLIBS_VERSION_STRING.
# We need to define them ourselves to do a version comparison
            if(DEFINED PYTHON_INCLUDE_PATH AND NOT DEFINED PYTHON_INCLUDE_DIRS)
                  set(PYTHON_INCLUDE_DIRS ${PYTHON_INCLUDE_PATH})
            endif(DEFINED PYTHON_INCLUDE_PATH AND NOT DEFINED PYTHON_INCLUDE_DIRS)
            if(NOT DEFINED PYTHONLIBS_VERSION_STRING)
                  STRING(REGEX REPLACE ".*python(2\\.[0-9]+)" "\\1"
                         PYTHONLIBS_VERSION_STRING "${PYTHON_INCLUDE_DIRS}")
            endif(NOT DEFINED PYTHONLIBS_VERSION_STRING)
            if (NOT PYTHONLIBS_VERSION_STRING VERSION_LESS 2.4)
                  set(PYTHON_SUPPORT ON)
            endif (NOT PYTHONLIBS_VERSION_STRING VERSION_LESS 2.4)
      endif (PYTHONLIBS_FOUND)
endif (ENABLE_PYTHON)

##
## check for dssi >= 0.9.0 and liblo >= 0.23
##

if (ENABLE_DSSI)
      PKG_CHECK_MODULES(DSSI dssi>=0.9.0)
      if (DSSI_FOUND)        
          if (ENABLE_OSC)
              PKG_CHECK_MODULES(LIBLO liblo>=0.23)
              if (LIBLO_FOUND)
                    include_directories(${LIBLO_INCLUDE_DIRS})
                    set(OSC_SUPPORT ON)
                    include_directories(${DSSI_INCLUDE_DIRS})
                    set(DSSI_SUPPORT ON)
              endif (LIBLO_FOUND)
          else (ENABLE_OSC)
              message("DSSI disabled")
          endif (ENABLE_OSC)
      endif (DSSI_FOUND)
else (ENABLE_DSSI)
      message("DSSI disabled")
endif (ENABLE_DSSI)

##
## Depricated FST-based VST support
##

if (ENABLE_VST)
      message("deprecated VST support enabled")
      set (VST_SUPPORT TRUE)
else (ENABLE_VST)
      #message("VST support disabled")
      set (VST_SUPPORT FALSE)
endif (ENABLE_VST)

##
## Native VST support
##

SET (AEFFECT_H_FOUND FALSE)
SET (VST_NATIVE_SUPPORT FALSE)

if (ENABLE_VST_NATIVE)
      find_file(VST_HEADER_CHECK aeffectx.h PATHS ${VST_HEADER_PATH} NO_DEFAULT_PATH)
      if (VST_HEADER_CHECK STREQUAL "VST_HEADER_CHECK-NOTFOUND")
        message("Native VST support disabled")
      else (VST_HEADER_CHECK STREQUAL "VST_HEADER_CHECK-NOTFOUND")
        SET (AEFFECT_H_FOUND TRUE)
        message("Native VST support enabled")
        set (VST_NATIVE_SUPPORT TRUE)
      endif (VST_HEADER_CHECK STREQUAL "VST_HEADER_CHECK-NOTFOUND")

else (ENABLE_VST_NATIVE)
      message("Native VST support disabled")
endif (ENABLE_VST_NATIVE)

# find_file creates a cached variable. We don't want VST_HEADER_CHECK cached.
UNSET (VST_HEADER_CHECK CACHE)

if (ENABLE_VST_VESTIGE)
  SET (VST_VESTIGE_SUPPORT TRUE)
else (ENABLE_VST_VESTIGE)
  SET (VST_VESTIGE_SUPPORT FALSE)
endif (ENABLE_VST_VESTIGE)

##
## LV2 support
##

if (ENABLE_LV2)
    set(LV2_SUPPORT ON)
    
    ##
    ## find Gtk2
    ##
    
    find_package(GTK2 COMPONENTS gtk gtkmm)
    if(GTK2_FOUND)
        set(HAVE_GTK2 ON)
    endif(GTK2_FOUND)
    
    set (muse_lilv_ver "0.22.0" )
    set (muse_lv2_ver "1.12.0")
    set (muse_sratom_ver "0.4.6")
    set (muse_sord_ver "0.14.0")
    set (muse_serd_ver "0.22.0")

    PKG_CHECK_MODULES(LILV lilv-0>=${muse_lilv_ver})
    if (LILV_FOUND)
            include_directories(${LILV_INCLUDE_DIRS})
    endif (LILV_FOUND)
    
    PKG_CHECK_MODULES(SORD sord-0>=${muse_sord_ver})
    if (SORD_FOUND)
            include_directories(${SORD_INCLUDE_DIRS})
    endif (SORD_FOUND)
    
    PKG_CHECK_MODULES(LV2 lv2>=${muse_lv2_ver})
    if (LV2_FOUND)
            include_directories(${LV2_INCLUDE_DIRS})
    endif (LV2_FOUND)

    if (ENABLE_LV2_SUPPLIED)
        set (LV2_USE_SUPPLIED_VERSION TRUE)
    else (ENABLE_LV2_SUPPLIED)
        set (LV2_USE_SUPPLIED_VERSION FALSE)
    endif (ENABLE_LV2_SUPPLIED)
    
    if (NOT (LILV_FOUND AND SORD_FOUND AND LV2_FOUND))
        set (LV2_USE_SUPPLIED_VERSION TRUE)
    endif (NOT (LILV_FOUND AND SORD_FOUND AND LV2_FOUND))
    
    if (LV2_USE_SUPPLIED_VERSION)
        PKG_CHECK_MODULES(PCRE libpcre)
        set (muse_lv2_dir "${PROJECT_SOURCE_DIR}/muse/lv2Support/lv2-${muse_lv2_ver}")
        set (muse_sratom_dir "${PROJECT_SOURCE_DIR}/muse/lv2Support/sratom-${muse_sratom_ver}")
        set (muse_sord_dir "${PROJECT_SOURCE_DIR}/muse/lv2Support/sord-${muse_sord_ver}")
        set (muse_serd_dir "${PROJECT_SOURCE_DIR}/muse/lv2Support/serd-${muse_serd_ver}")
        set (muse_lilv_dir "${PROJECT_SOURCE_DIR}/muse/lv2Support/lilv-${muse_lilv_ver}" )
        include_directories(
            ${muse_lv2_dir}
            ${muse_sord_dir}
            ${muse_sratom_dir}
            ${muse_serd_dir}
            ${muse_lilv_dir}
        )
    endif (LV2_USE_SUPPLIED_VERSION)
    
else (ENABLE_LV2)
      message("LV2 disabled")
endif (ENABLE_LV2)


if (ENABLE_LV2_DEBUG)
      set (DEBUG_LV2 ON)
      add_definitions(-DDEBUG_LV2)
endif (ENABLE_LV2_DEBUG)

##
## TODO
##
## Optimizations
##

SET (USE_SSE false)

##
## check for fluidsynth
##

if ( ENABLE_FLUID )
      # Be careful with variable name here since we have a shared
      #  synth library named 'fluid_synth' to be built later.
      PKG_CHECK_MODULES(FLUIDSYN fluidsynth>=0.9.0)
      if (FLUIDSYN_FOUND)
            include_directories(${FLUIDSYN_INCLUDE_DIRS})
            set(HAVE_FLUIDSYNTH ON)
      endif (FLUIDSYN_FOUND)
else ( ENABLE_FLUID )
      message("Fluidsynth disabled")
endif ( ENABLE_FLUID )

##
##   End OPTIONAL packages.
##


if ( ENABLE_EXPERIMENTAL )
      set(CMAKE_CXX_FLAGS -DBUILD_EXPERIMENTAL ${CMAKE_CXX_FLAGS})
endif ( ENABLE_EXPERIMENTAL )

#
# produce config.h file
#
configure_file (
      ${PROJECT_SOURCE_DIR}/config.h.in
      ${PROJECT_BINARY_DIR}/config.h
      )

add_custom_command(
      OUTPUT ${PROJECT_BINARY_DIR}/all.h
      COMMAND cp ${PROJECT_SOURCE_DIR}/all.h ${PROJECT_BINARY_DIR}/all.h
      DEPENDS ${PROJECT_SOURCE_DIR}/all.h
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
      )

add_custom_command(
      OUTPUT ${PROJECT_BINARY_DIR}/all-pic.h
      COMMAND cp ${PROJECT_SOURCE_DIR}/all.h ${PROJECT_BINARY_DIR}/all-pic.h
      DEPENDS ${PROJECT_SOURCE_DIR}/all.h
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
      )

add_custom_command(
      OUTPUT ${PROJECT_BINARY_DIR}/all-pic-debug.h
      COMMAND cp ${PROJECT_SOURCE_DIR}/all.h ${PROJECT_BINARY_DIR}/all-pic-debug.h
      DEPENDS ${PROJECT_SOURCE_DIR}/all.h
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
      )

include_directories(
   .
   ${PROJECT_SOURCE_DIR}
   ${PROJECT_SOURCE_DIR}/muse
   ${PROJECT_SOURCE_DIR}/muse/widgets
   ${PROJECT_BINARY_DIR}
   ${PROJECT_BINARY_DIR}/muse
   ${PROJECT_BINARY_DIR}/muse/widgets
   ${PROJECT_BINARY_DIR}/muse/instruments
   ${PROJECT_BINARY_DIR}/muse/ctrl
   )

# include (pch.txt)

#
# If the cmake version includes cpack, use it
#
IF(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")

  IF(EXISTS "${CMAKE_ROOT}/Modules/InstallRequiredSystemLibraries.cmake")
      SET(CMAKE_INSTALL_MFC_LIBRARIES 1)
      INCLUDE(InstallRequiredSystemLibraries)
  ENDIF(EXISTS "${CMAKE_ROOT}/Modules/InstallRequiredSystemLibraries.cmake")

  SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "MusE is a virtual audio studio environment")
  SET(CPACK_PACKAGE_VENDOR "Werner Schweer and others")
  SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
  SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
  SET(CPACK_PACKAGE_VERSION_MAJOR "${MusE_VERSION_MAJOR}")
  SET(CPACK_PACKAGE_VERSION_MINOR "${MusE_VERSION_MINOR}")
  SET(CPACK_PACKAGE_VERSION_PATCH "${MusE_VERSION_PATCH}")
  SET(CPACK_PACKAGE_INSTALL_DIRECTORY "CMake ${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}")
  SET(CPACK_SOURCE_PACKAGE_FILE_NAME "${MusE_INSTALL_NAME}")

  # Don't pack the binary tree and the subversion directories
  SET(CPACK_SOURCE_IGNORE_FILES "/\\\\.svn/;${CMAKE_BINARY_DIR}/*")

  SET(CPACK_SYSTEM_NAME ${CMAKE_SYSTEM_NAME})

  SET(CPACK_PACKAGE_FILE_NAME "${CPACK_SOURCE_PACKAGE_FILE_NAME}-${CPACK_SYSTEM_NAME}")
  SET(CPACK_STRIP_FILES "bin/muse;bin/grepmidi")
  SET(CPACK_PACKAGE_EXECUTABLES "muse" "MusE" "grepmidi" "grepmidi")
  INCLUDE(CPack)
ENDIF(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")

set(CMAKE_CXX_FLAGS         "-Werror=format-security -Wextra -Winvalid-pch -fno-exceptions -Wall -fPIC ${CMAKE_CXX_FLAGS}" CACHE STRING "Default CXX flags" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -fomit-frame-pointer -ffast-math -Wall -Wextra -fPIC ${CMAKE_CXX_FLAGS_RELEASE}" CACHE STRING "Release CXX flags" FORCE)
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -fomit-frame-pointer -ffast-math -Wall -Wextra -fPIC ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}" CACHE STRING "Release w/deb info CXX flags" FORCE)
set(CMAKE_CXX_FLAGS_DEBUG   "-g -O0 -DQT_DEBUG -Werror -Wall -Wextra -fPIC ${CMAKE_CXX_FLAGS_DEBUG}" CACHE STRING "Debug CXX flags" FORCE)

# NOTE: share/ directory needs to be at the end so that the translations
#       are scanned before coming to share/locale
subdirs(doc libs al awl grepmidi man plugins muse synti packaging utils demos share)

## Install doc files
file (GLOB doc_files
      AUTHORS
      ChangeLog
      COPYING
      NEWS
      README
      README.de
      README.effects-rack
      README.instruments
      README.ladspaguis
      README.shortcuts
      README.softsynth
      README.translate
      README.usage
      SECURITY
      )
INSTALL( FILES ${doc_files} DESTINATION ${MusE_DOC_DIR})

## Uninstall support
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)

add_custom_target(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake")

##
## Report errors and warnings and hints
##

message("\n")

if (NOT ALSA_FOUND)
      message("** ERROR: alsa >= 0.9.0 is required, but development files were not found.")
endif (NOT ALSA_FOUND)

if (NOT SNDFILE_FOUND)
      message("** ERROR: sndfile >= 1.0.0 is required, but development files were not found.")
endif (NOT SNDFILE_FOUND)

if (NOT SAMPLERATE_FOUND)
      message("** ERROR: samplerate >= 0.1.0 is required, but development files were not found.")
endif (NOT SAMPLERATE_FOUND)

if (NOT UUID_FOUND)
      message("** ERROR: uuid >= 0.0.1 is required, but development files were not found.")
endif (NOT UUID_FOUND)

if (NOT JACK_FOUND)
      message("** ERROR: jack >= 0.103 is required, but development files were not found.")
endif (NOT JACK_FOUND)

if (ENABLE_LASH AND (NOT LASH_FOUND))
        message("** WARNING: lash (>= 0.2) was enabled, but development files were not found. ")
        message("** HINT: Don't have LASH? Try installing the LADISH LASH compatibility package instead.")
endif (ENABLE_LASH AND (NOT LASH_FOUND))

if (ENABLE_OSC AND (NOT LIBLO_FOUND))
        message("** WARNING: liblo (>= 0.23) (Lightweight Open Sound Control) was enabled, but development files were not found.")
endif (ENABLE_OSC AND (NOT LIBLO_FOUND))

if (ENABLE_PYTHON AND (NOT PYTHONLIBS_FOUND))
        message("** WARNING: python was enabled, but development files were not found.")
endif (ENABLE_PYTHON AND (NOT PYTHONLIBS_FOUND))

if (ENABLE_DSSI AND (NOT DSSI_FOUND))
        message("** WARNING: dssi (>= 0.9.0) was enabled, but development files were not found.")
endif (ENABLE_DSSI AND (NOT DSSI_FOUND))

if (ENABLE_VST_NATIVE AND (NOT AEFFECT_H_FOUND))
        message("** WARNING: Native VST was enabled, but development files were not found.")
        message("** HINT: Check the VST_HEADER_PATH variable. Points to ./vestige by default.")
endif (ENABLE_VST_NATIVE AND (NOT AEFFECT_H_FOUND))

if (ENABLE_FLUID AND (NOT FLUIDSYN_FOUND))
        message("** WARNING: fluidsynth (>= 0.9.0) was enabled, but development files were not found.")
endif (ENABLE_FLUID AND (NOT FLUIDSYN_FOUND))

if (ENABLE_LV2)
    if(NOT GTK2_FOUND)
        message("** WARNING: LV2 support is enabled, but Gtk2 (gtkmm-2, gtk+-2) development files were not found. LV2 Gtk2 UI support is disabled.")
    endif(NOT GTK2_FOUND)
    
    if (LV2_USE_SUPPLIED_VERSION AND (NOT ENABLE_LV2_SUPPLIED))
        message("** WARNING: System LV2 support libraries were chosen, but development files were not found or too old:")
        message("   Requires lv2 >= " ${muse_lv2_ver} ", lilv-0 >= " ${muse_lilv_ver} ", sord-0 >= " ${muse_sord_ver})
        message("   Building supplied internal versions instead.")
    endif (LV2_USE_SUPPLIED_VERSION AND (NOT ENABLE_LV2_SUPPLIED))
endif (ENABLE_LV2)

message("")

## Show a summary of what we got
summary_add("Lash support" HAVE_LASH)
summary_add("OSC (Liblo) support" OSC_SUPPORT)
summary_add("Python support" PYTHON_SUPPORT)
summary_add("DSSI support" DSSI_SUPPORT)
summary_add("LV2 support" LV2_SUPPORT)
if(LV2_SUPPORT)
  summary_add("LV2 Gtk2 UI support" HAVE_GTK2)
endif(LV2_SUPPORT)
#summary_add("VST support" VST_SUPPORT)
summary_add("Native VST support" VST_NATIVE_SUPPORT)
summary_add("Fluidsynth support" HAVE_FLUIDSYNTH)
summary_add("Experimental features" ENABLE_EXPERIMENTAL)
summary_show()

if ( MODULES_BUILD_STATIC )
    message (" Internal modules will be built statically into the final binary.")
else ( MODULES_BUILD_STATIC )
    message (" Internal modules will be built as shared components.")
endif ( MODULES_BUILD_STATIC )

## Make the user aware of what type they're building.
if (NOT CMAKE_BUILD_TYPE)
    message(" Build type: CMAKE_BUILD_TYPE is empty. Plain un-optimized build.")
else (NOT CMAKE_BUILD_TYPE)
    message(" Build type: " ${CMAKE_BUILD_TYPE} )
endif (NOT CMAKE_BUILD_TYPE)
message("")

