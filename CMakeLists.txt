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

cmake_minimum_required(VERSION 3.10)
project(cppwheels LANGUAGES CXX)

file(GLOB WHEELS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(cppwheels STATIC ${WHEELS_SOURCES})

target_include_directories(cppwheels PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_features(cppwheels INTERFACE cxx_std_20)
