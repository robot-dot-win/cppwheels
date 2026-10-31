#  cppwheels static library CMakeLists.txt
#
#  Copyright (C) 2026, Martin Young <martin_young@live.cn>
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program. If not, see <https://www.gnu.org/licenses/>.
#------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.14)
project(cppwheels
    VERSION 1.0.0
    LANGUAGES CXX
    DESCRIPTION "C++20 utility library (cppwheels)"
)

#----------------------------------------------------------------------
# Collect all .cpp sources dynamically
#----------------------------------------------------------------------
file(GLOB CPPWHEELS_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

#----------------------------------------------------------------------
# Library target
#----------------------------------------------------------------------
add_library(cppwheels STATIC ${CPPWHEELS_SOURCES})
add_library(cppwheels::cppwheels ALIAS cppwheels)

target_include_directories(cppwheels
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(cppwheels PUBLIC cxx_std_20)

#----------------------------------------------------------------------
# Platform-specific settings (keep minimal and portable)
#----------------------------------------------------------------------
if(MSVC)
    target_compile_options(cppwheels PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(cppwheels PRIVATE -Wall -Wextra -Wpedantic)
endif()

#----------------------------------------------------------------------
# Optional install support
#----------------------------------------------------------------------
include(GNUInstallDirs)

install(TARGETS cppwheels
    EXPORT cppwheelsTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

file(GLOB CPPWHEELS_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)

install(FILES ${CPPWHEELS_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT cppwheelsTargets
    FILE cppwheelsTargets.cmake
    NAMESPACE cppwheels::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cppwheels
)
