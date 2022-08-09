# src/CMakeLists.txt
#
# Copyright (C) 2008  Werner Smekal
#
# create library
# =======================================================================
# libharu source files
# =======================================================================
file(GLOB LIBHPDF_SRCS *.c)

# =======================================================================
# create static and shared haru library
# =======================================================================
add_library(hpdf SHARED ${LIBHPDF_SRCS})
target_link_libraries(hpdf ${ADDITIONAL_LIBRARIES})