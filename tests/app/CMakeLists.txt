###############################################################################
#
# The MIT License (MIT)
#
# Copyright (c) 2017 Fabien Caylus
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
###############################################################################

cmake_minimum_required(VERSION 2.8)

project(JustPlug-TestApp)
set(EXE_NAME justplug-testapp)
set(PLUGIN_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../include)

# Avoid in source building
if("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
    message(FATAL_ERROR "In-source building is forbiden ! (Please create a build/ dir inside the source dir or everywhere else)")
endif()

# Set to release build by default
if("${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release")
endif()

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE})

#
# Compiler flags
#

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra")

#
# Add plugins projects
#

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}/plugin)
add_subdirectory(plugin/plugin_test)
add_subdirectory(plugin/plugin_1)
add_subdirectory(plugin/plugin_2)
add_subdirectory(plugin/plugin_3)
add_subdirectory(plugin/plugin_4)
add_subdirectory(plugin/plugin_5)
add_subdirectory(plugin/plugin_6)
add_subdirectory(plugin/plugin_7)
add_subdirectory(plugin/plugin_8)
add_subdirectory(plugin/plugin_9)
add_subdirectory(plugin/plugin_10)

# Add JustPlug library
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE})
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../.." "${CMAKE_CURRENT_BINARY_DIR}/justplug")
include_directories(${PLUGIN_INCLUDE_DIR})

# Set executable output
add_executable(
    ${EXE_NAME}
    main.cpp
)

target_link_libraries(${EXE_NAME} justplug)
