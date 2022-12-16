# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required( VERSION 3.20 FATAL_ERROR )

project( FrostMX )

find_package( thinkboxcmlibrary REQUIRED )
include( PrecompiledHeader )
include( ThinkboxCMLibrary )

set( SUPPORTED_MAX_VERSIONS 2022 2023 )

option( MAX_VERSION "The version of the 3ds Max SDK to build the library against." 2022 )

if( NOT MAX_VERSION IN_LIST SUPPORTED_MAX_VERSIONS )
  message( FATAL_ERROR "ERROR: Cannot build for unsupported 3ds Max version ${MAX_VERSION}" )
endif()

add_library( frostmx SHARED )

set_target_properties( frostmx PROPERTIES OUTPUT_NAME "Frost" )
set_target_properties( frostmx PROPERTIES PREFIX "" )
set_target_properties( frostmx PROPERTIES SUFFIX ".dlo" )

target_include_directories( frostmx PUBLIC
                            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                            $<INSTALL_INTERFACE:include> )

target_include_directories( frostmx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )

file( GLOB_RECURSE H_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        "include/*.h"
        "include/*.hpp"
)

file( GLOB_RECURSE CXX_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "src/*.cpp"
)

target_sources( frostmx PRIVATE
    "about.rc"
    "about_resource.h"
    "Frost.rc"
    "resource.h"
    "FrostMXVersion.rc"
    "stdafx.h"
    "stdafx.cpp"
    ${H_FILES}
    ${CXX_FILES}
)

# The Conan version of Boost was built with this, and it changes the library names.
# As a result, we need to set this to tell Boost to look for the right libraries to
# link against.
target_compile_definitions( frostmx PUBLIC BOOST_AUTO_LINK_SYSTEM )

find_package( thinkboxlibrary REQUIRED )
find_package( thinkboxmxlibrary REQUIRED )
find_package( maxsdk REQUIRED )
find_package( frost REQUIRED )
find_package( libb2 REQUIRED )
find_package( Boost REQUIRED )
find_package( OpenEXR REQUIRED )
find_package( ZLIB REQUIRED )
find_package( TBB REQUIRED )
find_package( tinyxml2 REQUIRED )
find_package( xxHash REQUIRED )

target_include_directories( frostmx PUBLIC ${thinkboxlibrary_INCLUDE_DIRS} )
target_include_directories( frostmx PUBLIC ${maxsdk_INCLUDE_DIRS} )
target_include_directories( frostmx PUBLIC ${thinkboxmxlibrary_INCLUDE_DIRS} )
target_include_directories( frostmx PUBLIC ${frost_INCLUDE_DIRS} )
target_include_directories( frostmx PUBLIC ${libb2_INCLUDE_DIRS} )
target_include_directories( frostmx PUBLIC ${Boost_INCLUDE_DIRS} )
target_include_directories( frostmx PUBLIC ${OpenEXR_INCLUDE_DIRS} )
target_include_directories( frostmx PUBLIC ${ZLIB_INCLUDE_DIRS} )
target_include_directories( frostmx PUBLIC ${TBB_INCLUDE_DIRS} )
target_include_directories( frostmx PUBLIC ${tinyxml2_INCLUDE_DIRS} )
target_include_directories( frostmx PUBLIC ${xxHash_INCLUDE_DIRS} )

target_link_libraries( frostmx PUBLIC thinkboxlibrary::thinkboxlibrary )
target_link_libraries( frostmx PUBLIC maxsdk::maxsdk )
target_link_libraries( frostmx PUBLIC thinkboxmxlibrary::thinkboxmxlibrary )
target_link_libraries( frostmx PUBLIC frost::frost )
target_link_libraries( frostmx PUBLIC libb2::libb2 )
target_link_libraries( frostmx PUBLIC Boost::Boost )
target_link_libraries( frostmx PUBLIC OpenEXR::OpenEXR )
target_link_libraries( frostmx PUBLIC ZLIB::ZLIB )
target_link_libraries( frostmx PUBLIC TBB::tbb )
target_link_libraries( frostmx PUBLIC tinyxml2::tinyxml2 )
target_link_libraries( frostmx PUBLIC xxHash::xxHash )

add_precompiled_header( frostmx stdafx.h SOURCE_CXX stdafx.cpp )

frantic_common_platform_setup( frostmx )
frantic_default_source_groups( frostmx HEADERDIR include SOURCEDIR src )

# Disable optimization for the RelWithDebInfo configuration.
# This allows breakpoints to be hit reliably when debugging in Visual Studio.
target_compile_options( frostmx PRIVATE "$<$<CONFIG:Release>:/O2>$<$<CONFIG:RelWithDebInfo>:/Od>" )

install( TARGETS frostmx
          RUNTIME DESTINATION bin
          LIBRARY DESTINATION lib
          ARCHIVE DESTINATION lib
)
