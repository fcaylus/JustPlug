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

project(JustPlug)
set(JP_SO_NAME justplug)

#
# Build options
#

# Avoid in source building
if("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
    message(FATAL_ERROR "In-source building is forbiden ! (Please create a build/ dir inside the source dir or everywhere else)")
endif()

# Set to release build by default
if("${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(CMAKE_BUILD_TYPE Release)
endif()

# Needed on Windows
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

#
# Compiler flags
#

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -fvisibility=hidden")

if(UNIX OR MINGW)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")
endif()

#
# Include thirdparty folder
#
include_directories(thirdparty)

# Add whereami dependency
file(
    GLOB_RECURSE
    whereami_files
    thirdparty/whereami/src/*.c
    thirdparty/whereami/src/*.h
)

# Add custom definitions
add_definitions(
    -DJP_PLUGIN_API=\"1.0.0\"
)

# Add src files
file (
    GLOB_RECURSE
    source_files
    src/*.cpp
    src/*.h
)
file( # Only for IDE
    GLOB_RECURSE
    include_files
    include/*.h
)

include_directories(include)

# Set library output
add_library(
    ${JP_SO_NAME}
    SHARED
    ${source_files}
    ${include_files}
    ${whereami_files}
)

if(UNIX)
    target_link_libraries(${JP_SO_NAME} dl)
endif()
