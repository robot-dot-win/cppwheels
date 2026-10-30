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
# Options
#----------------------------------------------------------------------
option(CPPWHEELS_ENABLE_FCGI
    "Build fcgiext module (requires libfcgi / fcgi-devel)"
    ON
)

#----------------------------------------------------------------------
# Collect all .cpp sources dynamically
#----------------------------------------------------------------------
file(GLOB CPPWHEELS_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

#----------------------------------------------------------------------
# Optional FastCGI support (fcgiext)
#----------------------------------------------------------------------
set(CPPWHEELS_HAS_FCGI FALSE)

if(CPPWHEELS_ENABLE_FCGI)
    # Prefer pkg-config when available (Linux, MinGW, etc.)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(FCGI QUIET fcgi)
    endif()

    if(NOT FCGI_FOUND)
        # Fallback: manual search for headers and library
        find_path(FCGI_INCLUDE_DIR
            NAMES fcgio.h fcgiapp.h
            PATHS
                /usr/include
                /usr/local/include
                /opt/local/include
                "C:/Program Files/fcgi/include"
                "C:/fcgi/include"
            DOC "FastCGI include directory"
        )
        find_library(FCGI_LIBRARY
            NAMES fcgi libfcgi
            PATHS
                /usr/lib
                /usr/lib64
                /usr/local/lib
                /usr/local/lib64
                /opt/local/lib
                "C:/Program Files/fcgi/lib"
                "C:/fcgi/lib"
            DOC "FastCGI library"
        )
        if(FCGI_INCLUDE_DIR AND FCGI_LIBRARY)
            set(FCGI_FOUND TRUE)
            set(FCGI_INCLUDE_DIRS ${FCGI_INCLUDE_DIR})
            set(FCGI_LIBRARIES ${FCGI_LIBRARY})
        endif()
    endif()

    if(FCGI_FOUND)
        set(CPPWHEELS_HAS_FCGI TRUE)
        message(STATUS "cppwheels: FastCGI found — building fcgiext module")
    else()
        message(STATUS "cppwheels: FastCGI not found — fcgiext module excluded")
    endif()
else()
    message(STATUS "cppwheels: FastCGI support disabled by user (CPPWHEELS_ENABLE_FCGI=OFF)")
endif()

# Exclude fcgiext.cpp when FastCGI is unavailable
if(NOT CPPWHEELS_HAS_FCGI)
    list(FILTER CPPWHEELS_SOURCES EXCLUDE REGEX "fcgiext\\.cpp$")
endif()

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

# Propagate FastCGI dependency when the module is enabled
if(CPPWHEELS_HAS_FCGI)
    target_include_directories(cppwheels PUBLIC ${FCGI_INCLUDE_DIRS})
    target_link_libraries(cppwheels PUBLIC ${FCGI_LIBRARIES})
    target_compile_definitions(cppwheels PUBLIC CPPWHEELS_HAS_FCGI=1)
endif()

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

# Install public headers; exclude fcgiext.hpp when FastCGI is unavailable
file(GLOB CPPWHEELS_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)
if(NOT CPPWHEELS_HAS_FCGI)
    list(FILTER CPPWHEELS_HEADERS EXCLUDE REGEX "fcgiext\\.hpp$")
endif()

install(FILES ${CPPWHEELS_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT cppwheelsTargets
    FILE cppwheelsTargets.cmake
    NAMESPACE cppwheels::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cppwheels
)
