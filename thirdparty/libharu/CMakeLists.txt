# CMakeLists.txt
#
# Copyright (C) 2008  Werner Smekal
#
# Process this file with cmake to produce Makefiles or project files
# for your specific compiler tool set
#
# TODO:
# - shared and static library
# - packaging
# - devpackage
# =======================================================================
# libharu project
# =======================================================================
project(libharu C)

# we want cmake version 2.4.8 at least
cmake_minimum_required(VERSION 2.4.8)


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# =======================================================================
# look for headers and libraries
# =======================================================================
# include(haru)
# include(summary)

# check zlib availibility
find_package(ZLIB)
if(ZLIB_FOUND)
  include_directories(${ZLIB_INCLUDE_DIR})
  set(ADDITIONAL_LIBRARIES ${ZLIB_LIBRARIES})
endif(ZLIB_FOUND)
  
# check png availibility
find_package(PNG)
if(PNG_FOUND)
  include_directories(${PNG_INCLUDE_DIR})
  add_definitions(${PNG_DEFINITIONS})
  set(ADDITIONAL_LIBRARIES ${ADDITIONAL_LIBRARIES} ${PNG_LIBRARIES})
endif(PNG_FOUND)


# Just set to 1, we'll assume they are always available.
# If not, then someone will have to add some tests in here to correctly determine
# the headers existance.
set (LIBHPDF_STDC_HEADERS 1)

# Don't do anything with this, it doesn't seem to be used anywhere anyhow
# set (LIBHPDF_size_t ???)

# support all of the different variations of LIBPNG defines in HARU
set (LIBHPDF_HAVE_LIBPNG ${PNG_FOUND})
if (NOT PNG_FOUND)
   set (LIBHPDF_HAVE_NOPNGLIB 1)
   set (HPDF_NOPNGLIB 1)
endif (NOT PNG_FOUND)

# support different zlib defines
set (LIBHPDF_HAVE_LIBZ ${ZLIB_FOUND})
if (NOT ZLIB_FOUND)
   set (LIBHPDF_HAVE_NOZLIB 1)
endif (NOT ZLIB_FOUND)


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# =======================================================================
# create library and demos
# =======================================================================
add_subdirectory(src)
# add_subdirectory(demo)