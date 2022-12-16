// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include "FrostMXVersion.h"

#include <shlobj.h>

#include <boost/algorithm/string.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>

#include <frantic/max3d/fpwrapper/max_typetraits.hpp>
#include <frantic/max3d/fpwrapper/static_wrapper.hpp>
#include <frantic/max3d/rendering/renderplugin_utils.hpp>

#include <frantic/files/files.hpp>

#include <frantic/volumetrics/implicitsurface/implicit_surface_to_rle_level_set.hpp>
#include <frantic/volumetrics/implicitsurface/particle_implicit_surface_policies.hpp>
#include <frantic/volumetrics/levelset/rle_level_set_file_io.hpp>
#include <frantic/volumetrics/voxel_coord_system.hpp>

#include <frantic/particles/particle_file_stream_factory.hpp>
#include <frantic/particles/streams/apply_function_particle_istream.hpp>
#include <frantic/particles/streams/concatenated_particle_istream.hpp>
#include <frantic/particles/streams/set_channel_particle_istream.hpp>
#include <frantic/particles/streams/transformed_particle_istream.hpp>

#include <frantic/win32/log_window.hpp>

#include "convert_particle_files.hpp"

#include "convert_particle_files_dialog.hpp"

#include "FrostStaticInterface.hpp"

#include "utility.hpp"

using namespace std;
using namespace frantic;
using namespace frantic::graphics;
using namespace frantic::volumetrics;
using namespace frantic::max3d;

namespace fs = boost::filesystem;

extern HINSTANCE ghInstance;

static FrostStaticInterface theFrostStaticInterface;

FrostStaticInterface* GetFrostInterface() { return &theFrostStaticInterface; }

FrostStaticInterface::FrostStaticInterface()
    : m_logWindow( _T("Frost Log Window"), 0, 0, true )
    , m_logPopupError( false ) {
    FFCreateDescriptor c( this, Interface_ID( 0x224f2a5c, 0x4d191a67 ), _T("FrostUtils"), 0 );

    c.add_property( &FrostStaticInterface::GetVersion, _T("Version") );
    c.add_property( &FrostStaticInterface::GetFrostHome, _T("FrostHome") );

#ifdef ENABLE_FROST_DISK_CACHE
    c.add_function( &FrostStaticInterface::set_cache_name, _T("SetCacheName"), _T("FilePattern") );
    c.add_function( &FrostStaticInterface::initialize_cached_level_set, _T("InitializeCachedLevelSet"), _T("Origin"),
                    _T("VoxelLength") );
    c.add_function( &FrostStaticInterface::cache_zhu_bridson_particles_as_level_set,
                    _T("CacheZhuBridsonParticlesAsLevelSet"), _T("particleFiles"), _T("maxParticleRadius"),
                    _T("effectRadius"), _T("lowDensityTrimmingDensity"), _T("lowDensityTrimmingStrength"),
                    _T("particleScaleFactor"), _T("randomizeParticleRadius") );
    c.add_function( &FrostStaticInterface::cache_metaball_particles_as_level_set,
                    _T("CacheMetaballParticlesAsLevelSet"), _T("particleFiles"), _T("maxParticleRadius"),
                    _T("effectRadiusScale"), _T("implicitThreshold"), _T("particleScaleFactor"),
                    _T("randomizeParticleRadius") );

    c.add_function( &FrostStaticInterface::SetSceneRenderBegin, _T("SetSceneRenderBegin") );
    c.add_function( &FrostStaticInterface::SetSceneRenderEnd, _T("SetSceneRenderEnd") );
#endif // #ifdef ENABLE_FROST_DISK_CACHE

    c.add_function( &FrostStaticInterface::LogError, _T("LogError"), _T("Msg") );
    c.add_function( &FrostStaticInterface::LogWarning, _T("LogWarning"), _T("Msg") );
    c.add_function( &FrostStaticInterface::LogProgress, _T("LogProgress"), _T("Msg") );
    c.add_function( &FrostStaticInterface::LogStats, _T("LogStats"), _T("Msg") );
    c.add_function( &FrostStaticInterface::LogDebug, _T("LogDebug"), _T("Msg") );

    c.add_property( &FrostStaticInterface::GetPopupLogWindowOnError, &FrostStaticInterface::SetPopupLogWindowOnError,
                    _T("PopupLogWindowOnError") );
    c.add_property( &FrostStaticInterface::GetLogWindowVisible, &FrostStaticInterface::SetLogWindowVisible,
                    _T("LogWindowVisible") );
    c.add_function( &FrostStaticInterface::FocusLogWindow, _T("FocusLogWindow") );
    c.add_property( &FrostStaticInterface::GetLoggingLevel, &FrostStaticInterface::SetLoggingLevel,
                    _T("LoggingLevel") );

    c.add_function( &FrostStaticInterface::GetSettingsDirectory, _T("GetSettingsDirectory") );
    c.add_function( &FrostStaticInterface::GetSettingsFilename, _T("GetSettingsFilename") );
    c.add_function( &FrostStaticInterface::GetValuePresetsFilename, _T("GetValuePresetsFilename") );

    c.add_function( &FrostStaticInterface::SanitizePresetName, _T("SanitizePresetName") );

    c.add_function( &FrostStaticInterface::ConvertParticleFileSequence, _T("ConvertParticleFileSequence"), _T("inFile"),
                    _T("outFile"), _T("firstFrame"), _T("lastFrame") );
    c.add_function( &FrostStaticInterface::ConvertParticleFile, _T("ConvertParticleFile"), _T("inFile"),
                    _T("outFile") );
    c.add_function( &FrostStaticInterface::ShowConvertToPRTFileDialog, _T("ShowConvertToPRTFileDialog"), _T("inFile") );
    c.add_function( &FrostStaticInterface::GetFileSequenceRange, _T("GetFileSequenceRange"), _T("filename") );
}

struct to_log {
    frantic::win32::log_window& m_wnd;
    std::string m_prefix;

    to_log& operator=( const to_log& );

  public:
    to_log( frantic::win32::log_window& wnd, const char* prefix )
        : m_wnd( wnd )
        , m_prefix( prefix ) {}
    void operator()( const char* msg ) { m_wnd.log( m_prefix + msg ); }
};

struct to_log_with_optional_popup {
    frantic::win32::log_window& m_wnd;
    frantic::tstring m_prefix;
    bool& m_logPopup;

    to_log_with_optional_popup& operator=( const to_log_with_optional_popup& );

  public:
    to_log_with_optional_popup( frantic::win32::log_window& wnd, const TCHAR* prefix, bool& logPopup )
        : m_wnd( wnd )
        , m_prefix( prefix )
        , m_logPopup( logPopup ) {}
    void operator()( const TCHAR* msg ) {
        if( m_logPopup && !m_wnd.is_visible() ) {
            m_wnd.show();
        }
        m_wnd.log( m_prefix + msg );
    }
};

#ifdef ENABLE_FROST_DISK_CACHE
// a level set object to hold the levelsets as we combine them before caching
levelset::rle_level_set m_cachedLevelSet;
float m_sceneScale;

float FrostStaticInterface::id_randomized_radius( boost::int32_t id, float radius, float variation,
                                                  boost::uint32_t randomSeed ) {
    boost::uint32_t hashedID = hashword( reinterpret_cast<boost::uint32_t*>( &id ), 1, randomSeed );
    float unitRandom = static_cast<float>( hashedID ) / static_cast<float>( numeric_limits<boost::uint32_t>::max() );
    // Only shrink the particles, because we are using the original radius as the basis for acceleration grids
    return radius * ( 1.f - unitRandom * variation );
}

// This creates a particle istream for a node.  If loading the particles loads files from the indicated
// file being saved, an exception is thrown.
boost::shared_ptr<frantic::particles::streams::particle_istream>
FrostStaticInterface::get_particle_istream( const vector<frantic::tstring>& filenames, float particleScaleFactor,
                                            float particleRadius, bool randomizeParticleRadius ) {
    vector<boost::shared_ptr<frantic::particles::streams::particle_istream>> pins;

    for( unsigned i = 0; i < filenames.size(); ++i )
        pins.push_back( frantic::particles::particle_file_istream_factory( filenames[i] ) );

    boost::shared_ptr<frantic::particles::streams::particle_istream> result;
    if( pins.empty() )
        throw runtime_error( "FrostUtils::GetParticleIStream() - No particle filenames were specified." );
    else if( pins.size() == 1 )
        result = pins[0];
    else
        result.reset( new frantic::particles::streams::concatenated_particle_istream( pins ) );

    // Scale the particles if necessary
    if( particleScaleFactor != 1 ) {
        result.reset( new frantic::particles::streams::transformed_particle_istream(
            result, transform4f::from_scale( particleScaleFactor ) ) );
    }

    // Add a channel with the particle radius to the stream.
    if( randomizeParticleRadius ) {
        float randomVariation = particleRadius / 2.f;
        boost::uint32_t randomSeed = static_cast<boost::uint32_t>( time( NULL ) );
        if( !result->get_native_channel_map().has_channel( _T("ID") ) )
            throw runtime_error(
                "FrostUtils::GetParticleIStream() - The input particles don't have an \"ID\" channel, as "
                "required when randomizing the particle radius by ID." );
        static boost::array<frantic::tstring, 1> inputParamNames = { _T("ID") };
        result.reset( new frantic::particles::streams::apply_function_particle_istream<float( const boost::int32_t& )>(
            result, boost::bind( &id_randomized_radius, _1, particleRadius, randomVariation, randomSeed ), _T("Radius"),
            inputParamNames ) );
    } else {
        // Set the radius channel to a single value
        result.reset( new frantic::particles::streams::set_channel_particle_istream<float>( result, _T("Radius"),
                                                                                            particleRadius ) );
    }

    // Force the channel map to have all the native channels (The "Radius" channel is just added to the native map
    // above.)
    result->set_channel_map( result->get_native_channel_map() );

    return result;
}
#endif // #ifdef ENABLE_FROST_DISK_CACHE

frantic::tstring FrostStaticInterface::GetFrostHome() {
    // Frost home is one level up from where this .dlr is (or two levels up if there is a win32/x64 subpath being used)
    fs::path homePath = fs::path( win32::GetModuleFileName( ghInstance ) ).branch_path();

    // If this structure has been subdivided into win32 and x64 subdirectories, then go one directory back.
    frantic::tstring pathLeaf = frantic::strings::to_lower( frantic::files::to_tstring( homePath.leaf() ) );
    if( pathLeaf == _T("win32") || pathLeaf == _T("x64") )
        homePath = homePath.branch_path();

    // Now go back past the "3dsMax#Plugin" folder
    homePath = homePath.branch_path();

    return files::ensure_trailing_pathseparator( frantic::files::to_tstring( homePath ) );
}

frantic::tstring FrostStaticInterface::GetVersion() {
    // Version: x.x.x rev.xxxxx
    frantic::tstring version( _T("Version: ") );
    version += _T( FRANTIC_VERSION );

    return version;
}

#ifdef ENABLE_FROST_DISK_CACHE

// Sets the name of file cache
void FrostStaticInterface::set_cache_name( const frantic::tstring& filename ) {

    if( m_cacheName.get_filename_pattern().matches_pattern( filename ) )
        return;
    m_cacheName.get_filename_pattern().set( filename );
    m_cacheName.sync_frame_set();
    if( !m_cacheName.directory_exists() )
        throw std::runtime_error( "FrostUtils::SetCacheName: You must provide a valid path!" );
}

/**
 * Initializes a cached copy of the level set to be dumped to a file.  The voxel coord system
 * is initialized and this is essentially the destination level set for particle to level set
 * function operations.  All units are converted to meters from scene units.
 *
 * @param	VCSOrigin	the origin of the cached level set's voxel coord system
 * @param	voxelLength	the voxel length of the cached level set's voxel coord system
 */
void FrostStaticInterface::initialize_cached_level_set( Point3 origin, float voxelLength ) {
    m_sceneScale = frantic::max3d::get_scale_to_meters(); // grab the scale factor to get to meters
    voxel_coord_system vcs( vector3f( origin ) * m_sceneScale, voxelLength * m_sceneScale );
    m_cachedLevelSet = levelset::rle_level_set( vcs );
}

/**
 *	Caches Zhu/Bridson particles as a level set.  Each call to this function caches the particles given in the
 *vector of filenames to the level set initialized in a previous InitializeCachedLevelSet call, and then writes it out
 *to a level set file specified in a previous SetCacheName call.  All units are converted to meters from scene units.
 *Randomization of particle radius is seeded by the system clock time, and variation is set to half the particle radius.
 *
 *	@param	particleFile				The particle file to cache as a level set.
 *	@param	maxParticleRadius			The largest particle radius in the given particle files
 *	@param	effectRadius				The compact support of the Zhu/Bridson kernel
 *	@param	lowDensityTrimmingDensity	A Zhu/Bridson isosurface param
 *	@param	lowDensityTrimmingStrength	A Zhu/Bridson isosurface param
 *	@param	particleScaleFactor			Any scale to apply to the particles
 *	@param	randomizeParticleRadius		Any scale to apply to the particles
 *	@param	t							Time to cache at.
 */
void FrostStaticInterface::cache_zhu_bridson_particles_as_level_set_wrapped(
    const std::vector<frantic::tstring>& particleFiles, float maxParticleRadius, float effectRadius,
    float lowDensityTrimmingDensity, float lowDensityTrimmingStrength, float particleScaleFactor,
    bool randomizeParticleRadius, FPTimeValue t ) {

    // convert all the params to meters
    maxParticleRadius *= m_sceneScale;
    effectRadius *= m_sceneScale;
    lowDensityTrimmingDensity *= m_sceneScale;
    lowDensityTrimmingStrength *= m_sceneScale;
    particleScaleFactor *= m_sceneScale;

    // if there are no particle files, dump an empty level set to disk
    double frame = double( t / GetTicksPerFrame() );
    if( particleFiles.empty() ) {
        m_cachedLevelSet.clear();
        write_rls_rle_level_set_file( m_cacheName[frame], m_cachedLevelSet );
        return;
    }

    // create a particle grid tree for the file particles
    boost::shared_ptr<frantic::particles::streams::particle_istream> pin =
        get_particle_istream( particleFiles, particleScaleFactor, maxParticleRadius, randomizeParticleRadius );
    if( pin->particle_count() == 0 ) {
        m_cachedLevelSet.clear();
        write_rls_rle_level_set_file( m_cacheName[frame], m_cachedLevelSet );
        return;
    }

    voxel_coord_system particleVCS( vector3f( 0 ), maxParticleRadius * effectRadius );

    frantic::particles::particle_grid_tree pgt( pin->get_channel_map(), particleVCS );
    pgt.clear();
    pgt.insert_particles( pin );

    // create a zhu/bridson IS policy using the input parameters
    implicitsurface::particle_zhu_bridson_is_policy isp( pgt, maxParticleRadius, effectRadius,
                                                         lowDensityTrimmingDensity, lowDensityTrimmingStrength,
                                                         m_cachedLevelSet.get_voxel_coord_system(), 0 );

    // build a level set from the policy and save it out
    frantic::channels::channel_propagation_policy cpp;
    implicitsurface::build_rle_level_set( cpp, isp, m_cachedLevelSet );
    levelset::write_rls_rle_level_set_file( m_cacheName[frame], m_cachedLevelSet );
}

/**
 *	Wrapper for cache_zhu_bridson_particles_as_level_set with the vector of strings.
 */
void FrostStaticInterface::cache_zhu_bridson_particles_as_level_set(
    Value* fileNames, float maxParticleRadius, float effectRadius, float lowDensityTrimmingDensity,
    float lowDensityTrimmingStrength, float particleScaleFactor, bool randomizeParticleRadius, FPTimeValue t ) {

    if( !is_array( fileNames ) )
        throw std::runtime_error(
            "FrostUtils::cache_zhu_bridson_particles_as_level_set - First argument was not an array. "
            " It should be an array of strings." );

    std::vector<frantic::tstring> particleFiles;

    int count = ( (Array*)fileNames )->size;

    for( int i = 0; i < count; ++i )
        particleFiles.push_back( ( *( (Array*)fileNames ) )[i]->to_string() );

    // mprintf("file array:\n");
    // for( int i = 0; i < particleFiles.size(); ++i )
    //	mprintf("%s\n", particleFiles[i].c_str());

    cache_zhu_bridson_particles_as_level_set_wrapped( particleFiles, maxParticleRadius, effectRadius,
                                                      lowDensityTrimmingDensity, lowDensityTrimmingStrength,
                                                      particleScaleFactor, randomizeParticleRadius, t );
}

/**
 *	Caches metaball particles as a level set.  Each call to this function caches the particles given in the vector
 *of filenames to the level set initialized in a previous InitializeCachedLevelSet call, and then writes it out to a
 *level set file specified in a previous SetCacheName call.  All units are converted to meters from scene units.
 *Randomization of particle radius is seeded by the system clock time, and variation is set to half the particle radius.
 *
 *	@param	particleFile				The particle file to cache as a level set.
 *	@param	maxParticleRadius			The largest particle radius in the given particle files
 *	@param	effectRadiusScale			The compact support of the Zhu/Bridson kernel
 *	@param	implicitThreshold			A Zhu/Bridson isosurface param
 *	@param	particleScaleFactor			Any scale to apply to the particles
 *	@param	randomizeParticleRadius		Any scale to apply to the particles
 *	@param	t							Time to cache at.
 */
void FrostStaticInterface::cache_metaball_particles_as_level_set_wrapped(
    const std::vector<frantic::tstring>& particleFiles, float maxParticleRadius, float effectRadiusScale,
    float implicitThreshold, float particleScaleFactor, bool randomizeParticleRadius, FPTimeValue t ) {

    // convert all the params to meters
    maxParticleRadius *= m_sceneScale;
    effectRadiusScale *= m_sceneScale;
    implicitThreshold *= m_sceneScale;
    particleScaleFactor *= m_sceneScale;

    // if there are no particle files, dump an empty level set to disk
    double frame = double( t / GetTicksPerFrame() );
    if( particleFiles.empty() ) {
        m_cachedLevelSet.clear();
        write_rls_rle_level_set_file( m_cacheName[frame], m_cachedLevelSet );
        return;
    }

    // create a particle grid tree for the file particles
    boost::shared_ptr<frantic::particles::streams::particle_istream> pin =
        get_particle_istream( particleFiles, particleScaleFactor, maxParticleRadius, randomizeParticleRadius );
    if( pin->particle_count() == 0 ) {
        m_cachedLevelSet.clear();
        write_rls_rle_level_set_file( m_cacheName[frame], m_cachedLevelSet );
        return;
    }

    voxel_coord_system particleVCS( vector3f( 0 ), maxParticleRadius * effectRadiusScale );

    frantic::particles::particle_grid_tree pgt( pin->get_channel_map(), particleVCS );
    pgt.clear();
    pgt.insert_particles( pin );

    // create a metaball IS policy using the input parameters
    implicitsurface::particle_metaball_is_policy isp( pgt, maxParticleRadius, effectRadiusScale, implicitThreshold,
                                                      m_cachedLevelSet.get_voxel_coord_system(), 0 );

    // build a level set from the policy and save it out
    frantic::channels::channel_propagation_policy cpp;
    implicitsurface::build_rle_level_set( cpp, isp, m_cachedLevelSet );
    levelset::write_rls_rle_level_set_file( m_cacheName[frame], m_cachedLevelSet );
}

/**
 *	Wrapper for cache_metaball_particles_as_level_set with the vector of strings.
 */
void FrostStaticInterface::cache_metaball_particles_as_level_set( Value* fileNames, float maxParticleRadius,
                                                                  float effectRadiusScale, float implicitThreshold,
                                                                  float particleScaleFactor,
                                                                  bool randomizeParticleRadius, FPTimeValue t ) {

    if( !is_array( fileNames ) )
        throw std::runtime_error(
            "FrostUtils::cache_zhu_bridson_particles_as_level_set - First argument was not an array. "
            " It should be an array of strings." );

    std::vector<frantic::tstring> particleFiles;

    int count = ( (Array*)fileNames )->size;

    for( int i = 0; i < count; ++i )
        particleFiles.push_back( ( *( (Array*)fileNames ) )[i]->to_string() );

    cache_metaball_particles_as_level_set_wrapped( particleFiles, maxParticleRadius, effectRadiusScale,
                                                   implicitThreshold, particleScaleFactor, randomizeParticleRadius, t );
}

void FrostStaticInterface::SetSceneRenderBegin( void ) {
    Interface* ip = GetCOREInterface();
    if( ip ) {
        const TimeValue t = ip->GetTime();
        std::set<ReferenceMaker*> doneNodes;
        frantic::max3d::rendering::refmaker_call_recursive( ip->GetRootNode(), doneNodes,
                                                            frantic::max3d::rendering::render_begin_function( t, 0 ) );
    }
}

void FrostStaticInterface::SetSceneRenderEnd( void ) {
    Interface* ip = GetCOREInterface();
    if( ip ) {
        const TimeValue t = ip->GetTime();
        std::set<ReferenceMaker*> doneNodes;
        frantic::max3d::rendering::refmaker_call_recursive( ip->GetRootNode(), doneNodes,
                                                            frantic::max3d::rendering::render_end_function( t ) );
    }
}

#endif // #ifdef ENABLE_FROST_DISK_CACHE

void init_notify_proc( void* param, NotifyInfo* /*info*/ ) {
    try {
        frantic::win32::log_window* pLogWindow = reinterpret_cast<frantic::win32::log_window*>( param );
        if( pLogWindow ) {
            pLogWindow->init( ghInstance, GetCOREInterface()->GetMAXHWnd() );
        }
    } catch( std::exception& e ) {
        if( GetCOREInterface() ) {
            LogSys* log = GetCOREInterface()->Log();
            if( log ) {
                const frantic::tstring errmsg = frantic::strings::to_tstring( e.what() );
                log->LogEntry( SYSLOG_ERROR, DISPLAY_DIALOG, _T( "Frost" ),
                               _T( "Unable to initialize Frost logging.\n\n%s" ), errmsg.c_str() );
            }
        }
    }
}

void to_frost_debug_log( const TCHAR* szMsg ) {
    if( frantic::logging::is_logging_debug() ) {
        GetFrostInterface()->LogMessageInternal( frantic::tstring( _T("DBG: ") ) + szMsg );
    }
}

void to_frost_stats_log( const TCHAR* szMsg ) {
    if( frantic::logging::is_logging_stats() ) {
        GetFrostInterface()->LogMessageInternal( frantic::tstring( _T("STS: ") ) + szMsg );
    }
}

void to_frost_progress_log( const TCHAR* szMsg ) {
    if( frantic::logging::is_logging_progress() ) {
        GetFrostInterface()->LogMessageInternal( frantic::tstring( _T("PRG: ") ) + szMsg );
    }
}

void FrostStaticInterface::InitializeLogging() {
    frantic::logging::set_logging_level( frantic::logging::level::warning );

    frantic::logging::debug.rdbuf( frantic::logging::new_ffstreambuf( &to_frost_debug_log ) );
    frantic::logging::stats.rdbuf( frantic::logging::new_ffstreambuf( &to_frost_stats_log ) );
    frantic::logging::progress.rdbuf( frantic::logging::new_ffstreambuf( &to_frost_progress_log ) );
    frantic::logging::warning.rdbuf(
        frantic::logging::new_ffstreambuf( to_log_with_optional_popup( m_logWindow, _T("WRN: "), m_logPopupError ) ) );
    frantic::logging::error.rdbuf(
        frantic::logging::new_ffstreambuf( to_log_with_optional_popup( m_logWindow, _T("ERR: "), m_logPopupError ) ) );

    int result = RegisterNotification( &init_notify_proc, &m_logWindow, NOTIFY_SYSTEM_STARTUP );
    // BEWARE!!! DO NOT THROW AN EXCEPTION BELOW IF init_notify_proc HAS REGISTERED A CALLBACK
    // If you throw an exception, the DLL will be unloaded.
    // The callback will then try to call code from the DLL that has been unloaded.
    if( !result ) {
        if( GetCOREInterface() && !is_network_render_server() ) {
            m_logWindow.init( ghInstance, NULL );
        }
    }
}

void FrostStaticInterface::LogError( const frantic::tstring& msg ) { FF_LOG( error ) << msg << std::endl; }

void FrostStaticInterface::LogWarning( const frantic::tstring& msg ) { FF_LOG( warning ) << msg << std::endl; }

void FrostStaticInterface::LogProgress( const frantic::tstring& msg ) { FF_LOG( progress ) << msg << std::endl; }

void FrostStaticInterface::LogStats( const frantic::tstring& msg ) { FF_LOG( stats ) << msg << std::endl; }

void FrostStaticInterface::LogDebug( const frantic::tstring& msg ) { FF_LOG( debug ) << msg << std::endl; }

bool FrostStaticInterface::GetPopupLogWindowOnError() { return m_logPopupError; }

void FrostStaticInterface::SetPopupLogWindowOnError( bool popupError ) { m_logPopupError = popupError; }

bool FrostStaticInterface::GetLogWindowVisible() { return m_logWindow.is_visible(); }

void FrostStaticInterface::SetLogWindowVisible( bool visible ) { m_logWindow.show( visible ); }

void FrostStaticInterface::FocusLogWindow() {
    if( m_logWindow.is_visible() ) {
        SetFocus( m_logWindow.handle() );
    }
}

frantic::tstring FrostStaticInterface::GetSettingsDirectory() {
    std::vector<TCHAR> buffer( MAX_PATH + 2, 0 );
    HRESULT result = SHGetFolderPath( GetCOREInterface()->GetMAXHWnd(), CSIDL_LOCAL_APPDATA, NULL, 0, &buffer[0] );
    frantic::tstring appDataPath = _T("");
    if( result == S_OK ) {
        appDataPath = frantic::tstring( &buffer[0] );
    } else if( result == S_FALSE ) {
        // ANSI
        // ok, but folder does not exist
        appDataPath = frantic::tstring( &buffer[0] );
    } else if( result == E_FAIL ) {
        // UNICODE
        // ok, but folder does not exit
        appDataPath = frantic::tstring( &buffer[0] );
    } else {
        throw std::runtime_error( "GetSettingsDirectory: Error getting directory for user settings." );
    }

    return frantic::files::to_tstring( ( boost::filesystem::path( appDataPath ) / _T("Thinkbox") / _T("Frost") ) );
}

frantic::tstring FrostStaticInterface::GetSettingsFilename() {
    const frantic::tstring settingsDirectory = GetSettingsDirectory();
    return frantic::files::to_tstring( ( boost::filesystem::path( settingsDirectory ) / _T("FrostPreferences.ini") ) );
}

frantic::tstring FrostStaticInterface::GetValuePresetsFilename() {
    const frantic::tstring settingsDirectory = GetSettingsDirectory();
    return frantic::files::to_tstring( ( boost::filesystem::path( settingsDirectory ) / _T("FrostValuePresets.ini") ) );
}

frantic::tstring FrostStaticInterface::SanitizePresetName( const frantic::tstring& presetNameIn ) {
    frantic::tstring presetName( presetNameIn );

    // forbid:
    // 0
    //< > : " / \ | ? *
    //[1,31]
    std::set<TCHAR> forbiddenCharacters =
        boost::assign::list_of( '<' )( '>' )( ':' )( '\"' )( '/' )( '\\' )( '|' )( '?' )( '*' );
    for( char c = 0; c <= 31; ++c ) {
        forbiddenCharacters.insert( c );
    }

    for( frantic::tstring::size_type i = 0; i < presetName.size(); ++i ) {
        if( forbiddenCharacters.find( presetName[i] ) != forbiddenCharacters.end() ) {
            presetName[i] = '-';
        }
    }

    // no space or period at the end of the filename
    boost::algorithm::trim_right_if( presetName, boost::algorithm::is_any_of( ". " ) );

    std::set<frantic::tstring> forbiddenNames = boost::assign::list_of( _T("CON") )( _T("PRN") )( _T("AUX") )(
        _T("NUL") )( _T("COM1") )( _T("COM2") )( _T("COM3") )( _T("COM4") )( _T("COM5") )( _T("COM6") )( _T("COM7") )(
        _T("COM8") )( _T("COM9") )( _T("LPT1") )( _T("LPT2") )( _T("LPT3") )( _T("LPT4") )( _T("LPT5") )( _T("LPT6") )(
        _T("LPT7") )( _T("LPT8") )( _T("LPT9") );
    if( forbiddenNames.find( presetName ) != forbiddenNames.end() ) {
        presetName.append( _T("-") );
    }

    return presetName;
}

void FrostStaticInterface::SetLoggingLevel( int loggingLevel ) { frantic::logging::set_logging_level( loggingLevel ); }

int FrostStaticInterface::GetLoggingLevel() { return frantic::logging::get_logging_level(); }

void FrostStaticInterface::LogMessageInternal( const frantic::tstring& msg ) { m_logWindow.log( msg ); }

bool FrostStaticInterface::ConvertParticleFileSequence( const frantic::tstring& infile, const frantic::tstring& outfile,
                                                        int firstFrame, int lastFrame ) {
    frantic::tstring errorString = _T("An unknown error occurred.");
    try {
        convert_particle_files_params params;
        params.infile = infile;
        params.outfile = outfile;
        params.frameRangeMode = convert_particle_files_params::RANGE;
        params.firstFrame = firstFrame;
        params.lastFrame = lastFrame;
        params.ignoreMissingInputFiles = false;
        params.replaceOutputFiles = is_network_render_server() != FALSE;
        if( accept_convert_particle_files_params( GetCOREInterface()->GetMAXHWnd(), params ) ) {
            bool done = convert_particle_files( params );
            return done;
        }
        return false;
    } catch( std::exception& e ) {
        errorString = frantic::strings::to_tstring( e.what() );
    } catch( ... ) {
    }
    throw MAXException(
        const_cast<TCHAR*>( ( _T("FrostUtils.ConvertParticleFileSequence() - ") + errorString ).c_str() ) );
}

bool FrostStaticInterface::ConvertParticleFile( const frantic::tstring& infile, const frantic::tstring& outfile ) {
    frantic::tstring errorString = _T("An unknown error occurred.");
    try {
        convert_particle_files_params params;
        params.infile = infile;
        params.outfile = outfile;
        params.frameRangeMode = convert_particle_files_params::SINGLE;
        params.firstFrame = 0;
        params.lastFrame = 0;
        params.ignoreMissingInputFiles = false;
        params.replaceOutputFiles = is_network_render_server() != FALSE;
        if( accept_convert_particle_files_params( GetCOREInterface()->GetMAXHWnd(), params ) ) {
            bool done = convert_particle_files( params );
            return done;
        }
        return false;
    } catch( std::exception& e ) {
        errorString = frantic::strings::to_tstring( e.what() );
    } catch( ... ) {
    }
    throw MAXException( const_cast<TCHAR*>( ( _T("FrostUtils.ConvertParticleFile() - ") + errorString ).c_str() ) );
}

frantic::tstring FrostStaticInterface::ShowConvertToPRTFileDialog( const frantic::tstring& infile ) {
    frantic::tstring errorString = _T("An unknown error occurred.");
    try {
        const frantic::tstring outFile = show_convert_particle_files_dialog( infile );
        return outFile;
    } catch( std::exception& e ) {
        errorString = frantic::strings::to_tstring( e.what() );
    } catch( ... ) {
    }
    throw MAXException( const_cast<TCHAR*>( ( _T("FrostUtils.ShowConvertToPRTDialog() - ") + errorString ).c_str() ) );
}

Value* FrostStaticInterface::GetFileSequenceRange( const frantic::tstring& filename ) {
    try {
        frantic::files::filename_sequence sequence( filename );
        sequence.sync_frame_set();
        std::pair<int, int> range = sequence.get_frame_set().wholeframe_range();
        std::vector<int> result;
        result.push_back( range.first );
        result.push_back( range.second );
        return frantic::max3d::fpwrapper::MaxTypeTraits<std::vector<int>>::to_max_type( result );
    } catch( std::exception& /*e*/ ) {
    }
    return &undefined;
}

void InitializeFrostLogging() { theFrostStaticInterface.InitializeLogging(); }
