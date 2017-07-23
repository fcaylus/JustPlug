cmake_minimum_required(VERSION 2.8)

project(JustPLug)
set(EXE_NAME justplug)
set(PLUGIN_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Avoid in source building
if("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
    message(FATAL_ERROR "In-source building is forbiden ! (Please create a build/ dir inside the source dir or everywhere else)")
endif()

# Set to release build by default
if("${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release")
endif()

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}/plugin)

#
# C++14 activation and other flags
#

#
# -fvisibility=hidden must be set for every plugin in case several plugins have the same class names
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -Wextra -fvisibility=hidden")

#
# Find Boost
#

find_package(Boost COMPONENTS filesystem system)
if(NOT Boost_FOUND)
    message(FATAL_ERROR "Boost library not found !")
endif()
include_directories(${Boost_INCLUDE_DIRS})

#
# Include thirdparty folder
#
include_directories(thirdparty)

#
# Add plugins projects
#
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

# Add custom definitions
add_definitions(
    -DAPPLICATION_NAME="${PROJECT_NAME}"
    -DAPPLICATION_EXE_NAME="${EXE_NAME}"
    -DPLUGIN_API="1.0.0"
)

# Add src files
file(
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

include_directories(${PLUGIN_INCLUDE_DIR})
include(${PLUGIN_INCLUDE_DIR}/EmbedMetadata.cmake)

# Set executable output
add_executable(
    ${EXE_NAME}
    ${source_files}
    ${include_files}
)

target_link_libraries(${EXE_NAME} ${Boost_LIBRARIES})
target_link_libraries(${EXE_NAME} dl)
