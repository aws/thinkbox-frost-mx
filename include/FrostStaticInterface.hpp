// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/win32/log_window.hpp>

#include <frantic/max3d/fpwrapper/max_typetraits.hpp>
#include <frantic/max3d/fpwrapper/static_wrapper.hpp>

class FrostStaticInterface : public frantic::max3d::fpwrapper::FFStaticInterface<FrostStaticInterface, FP_CORE> {

    frantic::win32::log_window m_logWindow;
    bool m_logPopupError;

#ifdef ENABLE_FROST_DISK_CACHE
    // required to manage file sequences
    frantic::files::filename_sequence m_cacheName;

    // a level set object to hold the levelsets as we combine them before caching
    frantic::volumetrics::levelset::rle_level_set m_cachedLevelSet;
    float m_sceneScale;

    static float id_randomized_radius( boost::int32_t id, float radius, float variation, boost::uint32_t randomSeed );

    // This creates a particle istream for a node.  If loading the particles loads files from the indicated
    // file being saved, an exception is thrown.
    boost::shared_ptr<frantic::particles::streams::particle_istream>
    get_particle_istream( const std::vector<frantic::tstring>& filenames, float particleScaleFactor,
                          float particleRadius, bool randomizeParticleRadius );
#endif // #ifdef ENABLE_FROST_DISK_CACHE

  public:
    FrostStaticInterface();

    void InitializeLogging();
    void LogError( const frantic::tstring& msg );
    void LogWarning( const frantic::tstring& msg );
    void LogProgress( const frantic::tstring& msg );
    void LogStats( const frantic::tstring& msg );
    void LogDebug( const frantic::tstring& msg );
    bool GetPopupLogWindowOnError();
    void SetPopupLogWindowOnError( bool popupError );
    bool GetLogWindowVisible();
    void SetLogWindowVisible( bool visible );
    void FocusLogWindow();
    void SetLoggingLevel( int loggingLevel );
    int GetLoggingLevel();
    void LogMessageInternal( const frantic::tstring& msg );

    frantic::tstring GetSettingsDirectory();
    frantic::tstring GetSettingsFilename();
    frantic::tstring GetValuePresetsFilename();

    frantic::tstring SanitizePresetName( const frantic::tstring& presetName );

    frantic::tstring GetFrostHome();
    frantic::tstring GetVersion();

    bool ConvertParticleFileSequence( const frantic::tstring& infile, const frantic::tstring& outfile, int firstFrame,
                                      int lastFrame );
    bool ConvertParticleFile( const frantic::tstring& infile, const frantic::tstring& outfile );
    frantic::tstring ShowConvertToPRTFileDialog( const frantic::tstring& infile );

    Value* GetFileSequenceRange( const frantic::tstring& filename );

#ifdef ENABLE_FROST_DISK_CACHE

    // Sets the name of file cache
    void set_cache_name( const frantic::tstring& filename );

    /**
     * Initializes a cached copy of the level set to be dumped to a file.  The voxel coord system
     * is initialized and this is essentially the destination level set for particle to level set
     * function operations.  All units are converted to meters from scene units.
     *
     * @param	VCSOrigin	the origin of the cached level set's voxel coord system
     * @param	voxelLength	the voxel length of the cached level set's voxel coord system
     */
    void initialize_cached_level_set( Point3 origin, float voxelLength );

    /**
     *	Caches Zhu/Bridson particles as a level set.  Each call to this function caches the particles given in the
     *vector of filenames to the level set initialized in a previous InitializeCachedLevelSet call, and then writes it
     *out to a level set file specified in a previous SetCacheName call.  All units are converted to meters from scene
     *units. Randomization of particle radius is seeded by the system clock time, and variation is set to half the
     *particle radius.
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
    void cache_zhu_bridson_particles_as_level_set_wrapped( const std::vector<frantic::tstring>& particleFiles,
                                                           float maxParticleRadius, float effectRadius,
                                                           float lowDensityTrimmingDensity,
                                                           float lowDensityTrimmingStrength, float particleScaleFactor,
                                                           bool randomizeParticleRadius, FPTimeValue t );

    /**
     *	Wrapper for cache_zhu_bridson_particles_as_level_set with the vector of strings.
     */
    void cache_zhu_bridson_particles_as_level_set( Value* fileNames, float maxParticleRadius, float effectRadius,
                                                   float lowDensityTrimmingDensity, float lowDensityTrimmingStrength,
                                                   float particleScaleFactor, bool randomizeParticleRadius,
                                                   FPTimeValue t );

    /**
     *	Caches metaball particles as a level set.  Each call to this function caches the particles given in the vector
     *of filenames to the level set initialized in a previous InitializeCachedLevelSet call, and then writes it out to a
     *level set file specified in a previous SetCacheName call.  All units are converted to meters from scene units.
     *	Randomization of particle radius is seeded by the system clock time, and variation is set to half
     *	the particle radius.
     *
     *	@param	particleFile				The particle file to cache as a level set.
     *	@param	maxParticleRadius			The largest particle radius in the given particle files
     *	@param	effectRadiusScale			The compact support of the Zhu/Bridson kernel
     *	@param	implicitThreshold			A Zhu/Bridson isosurface param
     *	@param	particleScaleFactor			Any scale to apply to the particles
     *	@param	randomizeParticleRadius		Any scale to apply to the particles
     *	@param	t							Time to cache at.
     */
    void cache_metaball_particles_as_level_set_wrapped( const std::vector<frantic::tstring>& particleFiles,
                                                        float maxParticleRadius, float effectRadiusScale,
                                                        float implicitThreshold, float particleScaleFactor,
                                                        bool randomizeParticleRadius, FPTimeValue t );

    /**
     *	Wrapper for cache_metaball_particles_as_level_set with the vector of strings.
     */
    void cache_metaball_particles_as_level_set( Value* fileNames, float maxParticleRadius, float effectRadiusScale,
                                                float implicitThreshold, float particleScaleFactor,
                                                bool randomizeParticleRadius, FPTimeValue t );

    void SetSceneRenderBegin( void );

    void SetSceneRenderEnd( void );

#endif // #ifdef ENABLE_FROST_DISK_CACHE
};

FrostStaticInterface* GetFrostInterface();
void InitializeFrostLogging();
