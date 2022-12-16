// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include "FrostVRayLibrary.hpp"

#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/make_shared.hpp>
#include <boost/regex.hpp>

#include <frantic/strings/to_string_classic.hpp>
#include <frantic/win32/utility.hpp>

extern HINSTANCE ghInstance;

FrostVRayLibrary::FrostVRayLibrary( const std::wstring& filename ) {
    boost::int64_t frostVersion = frantic::win32::GetVersion( frantic::win32::GetModuleFileName( ghInstance ) );
    boost::int64_t frostVRVersion = frantic::win32::GetVersion( frantic::strings::to_tstring( filename ) );

    if( frostVersion != frostVRVersion ) {
        throw std::runtime_error(
            "Mismatched product version between Frost and \"" + frantic::strings::to_string( filename ) +
            "\".\n\nThis may be caused by an incorrect Frost installation.\n\nPlease reinstall." );
    }

    // should possibly use SetErrorMode ?
    scoped_hmodule module( LoadLibraryW( filename.c_str() ) );
    if( !module.get() ) {
        throw std::runtime_error( "FrostVRayLibrary: An error occurred when attempting to load \"" +
                                  frantic::strings::to_string( filename ) +
                                  "\": " + frantic::win32::GetLastErrorMessageA() );
    }

    get_frost_version_hash_function_t getFrostVersionHash = (get_frost_version_hash_function_t)GetProcAddress(
        module.get(), GET_FROST_VERSION_HASH_DECORATED_FUNCTION_NAME );
    if( !getFrostVersionHash ) {
        throw std::runtime_error(
            "FrostVRayLibrary: An error occurred when attempting to get function GetFrostVersionHash from \"" +
            frantic::strings::to_string( filename ) + "\": " + frantic::win32::GetLastErrorMessageA() );
    }
    if( getFrostVersionHash() != FROST_VERSION_HASH ) {
        throw std::runtime_error(
            "Mismatched internal version between Frost and \"" + frantic::strings::to_string( filename ) +
            "\".\n\nThis may be caused by an incorrect Frost installation.\n\nPlease reinstall." );
    }

    m_createVRenderObject =
        (create_vrender_object_function_t)GetProcAddress( module.get(), CREATE_VRENDER_OBJECT_DECORATED_FUNCTION_NAME );
    if( !m_createVRenderObject ) {
        throw std::runtime_error(
            "FrostVRayLibrary: An error occurred when attempting to get function CreateVRenderObject from \"" +
            frantic::strings::to_string( filename ) + "\": " + frantic::win32::GetLastErrorMessageA() );
    }

    m_destroyVRenderObject = (destroy_vrender_object_function_t)GetProcAddress(
        module.get(), DESTROY_VRENDER_OBJECT_DECORATED_FUNCTION_NAME );
    if( !m_createVRenderObject ) {
        throw std::runtime_error(
            "FrostVRayLibrary: An error occurred when attempting to get function DestroyVRenderObject from \"" +
            frantic::strings::to_string( filename ) + "\": " + frantic::win32::GetLastErrorMessageA() );
    }

    m_module.reset( module.release() );
}

VUtils::VRenderObject* FrostVRayLibrary::CreateVRenderObject( FrostInterface* frost ) {
    assert( m_createVRenderObject );
    return m_createVRenderObject( frost );
}

void FrostVRayLibrary::DestroyVRenderObject( VUtils::VRenderObject* obj ) {
    assert( m_destroyVRenderObject );
    m_destroyVRenderObject( obj );
}

namespace {

int get_human_readable_max_version() {
    const MCHAR* currentVersionRaw = UtilityInterface::GetCurrentVersion();
    if( !currentVersionRaw ) {
        throw std::runtime_error( "get_human_readable_max_version Error: currentVersion is NULL" );
    }

    const frantic::tstring currentVersion( currentVersionRaw );
    size_t i = currentVersion.find( '.' );
    if( i == frantic::tstring::npos ) {
        throw std::runtime_error( "get_human_readable_max_version Error: missing \'.\' in version string: \"" +
                                  frantic::strings::to_string( currentVersion ) + "\"" );
    }

    const frantic::tstring majorVersionString = currentVersion.substr( 0, i );

    int majorVersion = boost::lexical_cast<int>( majorVersionString );
    // Switch to years w/ max 2008 (was Max 10)
    if( majorVersion > 9 ) {
        majorVersion += 1998;
    }

    return majorVersion;
}

std::wstring get_vray_dll_name() {
    return L"vray" + frantic::strings::to_string_classic<frantic::tstring>( get_human_readable_max_version() ) +
           L".dll";
}

std::string get_pretty_vray_version_string( unsigned int version ) {
    unsigned int major = version >> 16;
    unsigned int minor = ( version >> 8 ) & 0xFF;
    unsigned int patch = version & 0xFF;
    return ( boost::format( "%x.%x.%x" ) % major % minor % patch ).str();
}

std::wstring get_vray_version_string( unsigned int version ) { return ( boost::wformat( L"%x" ) % version ).str(); }

void get_filenames_in_directory( const boost::filesystem::path& directory,
                                 std::vector<boost::filesystem::path>& fileListing ) {
    fileListing.clear();

    if( !boost::filesystem::exists( directory ) ) {
        throw std::runtime_error( "get_filenames_in_directory(): Directory does not exist: " + directory.string() );
    }

    for( boost::filesystem::directory_iterator i( directory ), ie; i != ie; ++i ) {
        bool accept = false;

        if( boost::filesystem::exists( *i ) ) {
            if( boost::filesystem::is_regular_file( *i ) ) {
                accept = true;
            } else if( boost::filesystem::is_symlink( *i ) ) {
                // TODO: switch to canonical when we update boost
                // boost::filesystem::path p( boost::filesystem::canonical( *i ) );
                boost::filesystem::path p( boost::filesystem::read_symlink( *i ) );
                if( boost::filesystem::exists( p ) && boost::filesystem::is_regular_file( p ) ) {
                    accept = true;
                }
            }
        }

        if( accept ) {
            fileListing.push_back( i->path().filename() );
        }
    }
}

std::wstring get_frost_vray_dll_name( const std::wstring& vrayVersion, const std::wstring& maxVersion ) {
    return L"Frost_vray" + vrayVersion + L"_max" + maxVersion + L".dll";
}

std::wstring get_frost_vray_dll_name( const std::wstring& vrayVersion ) {
    return get_frost_vray_dll_name(
        vrayVersion, frantic::strings::to_string_classic<frantic::tstring>( get_human_readable_max_version() ) );
}

boost::filesystem::path get_frost_dll_directory() {
    namespace fs = boost::filesystem;
    return fs::path( frantic::win32::GetModuleFileName( ghInstance ) ).branch_path();
}

template <class BidirectionalIterator, class T>
BidirectionalIterator find_closest_not_greater_than( BidirectionalIterator first, BidirectionalIterator last,
                                                     const T& value ) {
    BidirectionalIterator i = std::upper_bound( first, last, value );
    if( i == first ) {
        return last;
    } else {
        return --i;
    }
}

} // anonymous namespace

FrostVRayLibrary::ptr_type LoadFrostVRayLibrary() {
    const std::wstring dll = get_vray_dll_name();
    scoped_hmodule hmodule( LoadLibraryW( dll.c_str() ) );
    if( !hmodule.get() ) {
        throw std::runtime_error( "LoadFrostVRayLibrary: Unable to load VRay DLL (" +
                                  frantic::strings::to_string( dll ) + "): \"" +
                                  frantic::win32::GetLastErrorMessageA() + "\"" );
    }
    typedef unsigned int ( *get_vray_revision_t )( void );
    get_vray_revision_t getVRayRevision =
        (get_vray_revision_t)GetProcAddress( hmodule.get(), "?getVRayRevision@VUtils@@YAIXZ" );
    if( !getVRayRevision ) {
        throw std::runtime_error( "LoadFrostVRayLibrary: Unable to find getVRayRevision() in " +
                                  frantic::strings::to_string( dll ) );
    }

    const unsigned int vrayRevision = getVRayRevision();
    if( vrayRevision < 0x40000 ) {
        throw std::runtime_error( "You are using an older version of V-Ray (" +
                                  get_pretty_vray_version_string( vrayRevision ) +
                                  ") that is not supported by Frost.\n"
                                  "To fix this error, you can update to V-Ray 4.00 or later." );
    }

    // We don't actually need to call this function.
    // My understanding is that the numbers after the function name correspond
    // to the API version, and I want to make sure that it hasn't changed.
    // This check isn't really necessary, but I want to give a helpful error
    // message, and I don't want to set an artifically low upper bound
    // on the vrayVersion.
    FARPROC getVRayVersion = GetProcAddress( hmodule.get(), "?getVRayVersion_1017@VUtils@@YAIXZ" );
    if( !getVRayVersion ) {
        throw std::runtime_error( "You are using a newer version of V-Ray (" +
                                  get_pretty_vray_version_string( vrayRevision ) +
                                  ") that is not yet supported by Frost.\n"
                                  "Please contact support@thinkboxsoftware.com" );
    }

    std::vector<boost::filesystem::path> filenames;
    get_filenames_in_directory( get_frost_dll_directory(), filenames );

    typedef std::vector<int> versions_t;
    versions_t versions;
    boost::wregex r( L"^" + get_frost_vray_dll_name( L"(\\d+)" ) + L"$" );
    BOOST_FOREACH( const boost::filesystem::path& filename, filenames ) {
        boost::wsmatch what;
        if( boost::regex_search( filename.native(), what, r ) ) {
            // V-Ray's internal version numbers are stored as human-readable hex.
            // For example, version 40000 in the filename corresponds to 0x040000 in V-Ray,
            // so we need to convert the version number.
            const frantic::tstring hexVersion = what[1].str();
            const unsigned int intVersion = std::stoul( hexVersion, nullptr, 16 );
            versions.push_back( intVersion );
        }
    }
    std::sort( versions.begin(), versions.end() );

    const int maxVersion = get_human_readable_max_version();

    if( versions.empty() ) {
        // Couldn't find an appropriate plugin.
        // Are there *any* plugins?
        std::vector<int> maxVersions;
        boost::wregex r( L"^" + get_frost_vray_dll_name( L"(\\d+)", L"(\\d+)" ) + L"$" );
        BOOST_FOREACH( const boost::filesystem::path& filename, filenames ) {
            boost::wsmatch what;
            if( boost::regex_search( filename.native(), what, r ) ) {
                maxVersions.push_back( boost::lexical_cast<int>( what[2].str() ) );
            }
        }
        std::sort( maxVersions.begin(), maxVersions.end() );
        if( maxVersions.empty() ) {
            throw std::runtime_error( "Could not find Frost V-Ray plugin (" +
                                      frantic::strings::to_string( get_frost_vray_dll_name( L"*" ) ) +
                                      ").\n"
                                      "This is likely caused by an incorrect Frost installation.  Please reinstall." );
        } else if( maxVersion > maxVersions.back() ) {
            throw std::runtime_error( "You are using a newer version of 3ds Max (" +
                                      frantic::strings::to_string_classic<std::string>( maxVersion ) +
                                      ") that is not yet supported by Frost\'s "
                                      "V-Ray Geometry Instancing.\n"
                                      "Please contact support@thinkboxsoftware.com" );
        } else {
            throw std::runtime_error( "Could not find Frost V-Ray plugin (" +
                                      frantic::strings::to_string( get_frost_vray_dll_name( L"*" ) ) +
                                      ").\n"
                                      "This is likely caused by an incorrect Frost installation.  Please reinstall." );
        }
    }

    versions_t::iterator i = find_closest_not_greater_than( versions.begin(), versions.end(), vrayRevision );
    if( i == versions.end() ) {
        // older version of V-Ray
        // but somehow newer than 4.00, our minimum supported version, which we checked for above
        throw std::runtime_error( "You are using a version of V-Ray "
                                  "(" +
                                  get_pretty_vray_version_string( vrayRevision ) + " with 3ds Max " +
                                  frantic::strings::to_string_classic<std::string>( maxVersion ) +
                                  ") "
                                  "that is not yet supported by Frost.\n"
                                  "Please contact support@thinkboxsoftware.com" );
    }
    const unsigned int frostVRVersion = *i;
    const std::wstring frostVRDLL = get_frost_vray_dll_name( get_vray_version_string( frostVRVersion ) );
    boost::filesystem::path frostVRPath = get_frost_dll_directory() / frostVRDLL;
    return boost::make_shared<FrostVRayLibrary>( frostVRPath.native() );
}
