# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required( VERSION 3.20 FATAL_ERROR )

project( FrostVRY )

find_package( thinkboxcmlibrary REQUIRED )
include( PrecompiledHeader )
include( ThinkboxCMLibrary )

set( SUPPORTED_MAX_VERSIONS 2022 2023 )

option( MAX_VERSION "The version of the 3ds Max SDK to build the library against." 2022 )

if( NOT MAX_VERSION IN_LIST SUPPORTED_MAX_VERSIONS )
  message( FATAL_ERROR "ERROR: Cannot build for unsupported 3ds Max version ${MAX_VERSION}" )
endif()

option( VRAY_VERSION "The version of V-Ray we are using the SDK from." 0 )

if( VRAY_VERSION EQUAL 0 )
  message( FATAL_ERROR "Error: VRAY_VERSION not provided. This is required to set the correct DLL name.")
endif()

add_library( frostvry SHARED )

set_target_properties( frostvry PROPERTIES OUTPUT_NAME "Frost_vray${VRAY_VERSION}_max${MAX_VERSION}" )
set_target_properties( frostvry PROPERTIES PREFIX "" )
set_target_properties( frostvry PROPERTIES SUFFIX ".dll" )

target_include_directories( frostvry PUBLIC
                            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                            $<INSTALL_INTERFACE:include> )

target_include_directories( frostvry PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )

file( GLOB_RECURSE H_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        "include/*.h"
        "include/*.hpp"
)

file( GLOB_RECURSE CXX_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "src/*.cpp"
)

target_sources( frostvry PRIVATE
    "FrostVRay.rc"
    "resource.h"
    "FrostVersion.rc"
    "stdafx.h"
    "stdafx.cpp"
    ${H_FILES}
    ${CXX_FILES}
)

add_precompiled_header( frostvry stdafx.h SOURCE_CXX stdafx.cpp )

# The Conan version of Boost was built with this, and it changes the library names.
# As a result, we need to set this to tell Boost to look for the right libraries to
# link against.
target_compile_definitions( frostvry PUBLIC BOOST_AUTO_LINK_SYSTEM )

# This varible must be defined as in VRAY 5 they are developing mulitple DR modes.
# In VRay 5.2x this is defined. In VRay 5.0x and 5.1x this is undefined and in 5.1x it is required.
# When DR2 is fully available this should be revisited.
target_compile_definitions(frostvry PRIVATE WITH_DR1)

find_package( thinkboxlibrary REQUIRED )
find_package( thinkboxmxlibrary REQUIRED )
find_package( maxsdk REQUIRED )
find_package( vraysdk REQUIRED )
find_package( frost REQUIRED )
find_package( libb2 REQUIRED )
find_package( Boost REQUIRED )
find_package( OpenEXR REQUIRED )
find_package( ZLIB REQUIRED )
find_package( TBB REQUIRED )
find_package( tinyxml2 REQUIRED )
find_package( xxHash REQUIRED )

target_include_directories( frostvry PUBLIC ${thinkboxlibrary_INCLUDE_DIRS} )
target_include_directories( frostvry PUBLIC ${maxsdk_INCLUDE_DIRS} )
target_include_directories( frostvry PUBLIC ${vraysdk_INCLUDE_DIRS} )
target_include_directories( frostvry PUBLIC ${thinkboxmxlibrary_INCLUDE_DIRS} )
target_include_directories( frostvry PUBLIC ${frost_INCLUDE_DIRS} )
target_include_directories( frostvry PUBLIC ${libb2_INCLUDE_DIRS} )
target_include_directories( frostvry PUBLIC ${Boost_INCLUDE_DIRS} )
target_include_directories( frostvry PUBLIC ${OpenEXR_INCLUDE_DIRS} )
target_include_directories( frostvry PUBLIC ${ZLIB_INCLUDE_DIRS} )
target_include_directories( frostvry PUBLIC ${TBB_INCLUDE_DIRS} )
target_include_directories( frostvry PUBLIC ${tinyxml2_INCLUDE_DIRS} )
target_include_directories( frostvry PUBLIC ${xxHash_INCLUDE_DIRS} )

target_link_libraries( frostvry PUBLIC thinkboxlibrary::thinkboxlibrary )
target_link_libraries( frostvry PUBLIC maxsdk::maxsdk )
target_link_libraries( frostvry PUBLIC vraysdk::vraysdk )
target_link_libraries( frostvry PUBLIC thinkboxmxlibrary::thinkboxmxlibrary )
target_link_libraries( frostvry PUBLIC frost::frost )
target_link_libraries( frostvry PUBLIC libb2::libb2 )
target_link_libraries( frostvry PUBLIC Boost::Boost )
target_link_libraries( frostvry PUBLIC OpenEXR::OpenEXR )
target_link_libraries( frostvry PUBLIC ZLIB::ZLIB )
target_link_libraries( frostvry PUBLIC TBB::tbb )
target_link_libraries( frostvry PUBLIC tinyxml2::tinyxml2 )
target_link_libraries( frostvry PUBLIC xxHash::xxHash )

frantic_common_platform_setup( frostvry )
frantic_default_source_groups( frostvry HEADERDIR include SOURCEDIR src )

# Disable optimization for the RelWithDebInfo configuration.
# This allows breakpoints to be hit reliably when debugging in Visual Studio.
target_compile_options( frostvry PRIVATE "$<$<CONFIG:Release>:/O2>$<$<CONFIG:RelWithDebInfo>:/Od>" )

install( TARGETS frostvry
          RUNTIME DESTINATION bin
          LIBRARY DESTINATION lib
          ARCHIVE DESTINATION lib
)
