// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include "Frost.hpp"
#include "particle_cache_entry.hpp"

#include <frantic/particles/streams/fractional_by_id_particle_istream.hpp>
#include <frantic/particles/streams/fractional_particle_istream.hpp>

particle_cache_entry::particle_cache_entry()
    : m_fraction( 0 )
    , m_loadMode( VIEWPORT_LOAD_MODE::HEAD )
    , m_age( 0 )
    , m_hasParticles( false ) {}

particle_cache_entry& particle_cache_entry::operator=( const particle_cache_entry& other ) {
    particles = other.particles;
    m_age = other.m_age;
    m_hasParticles = other.m_hasParticles;
    m_fraction = other.m_fraction;
    m_loadMode = other.m_loadMode;
    return *this;
}

void particle_cache_entry::set( boost::shared_ptr<frantic::particles::streams::particle_istream> stream,
                                double fraction, int loadMode ) {
    m_fraction = fraction;
    m_loadMode = loadMode;
    m_age = 0;
    m_hasParticles = true;

    boost::shared_ptr<frantic::particles::streams::particle_istream> pin = stream;

    if( fraction < 1.0 ) {
        switch( m_loadMode ) {
        case VIEWPORT_LOAD_MODE::HEAD:
            pin = frantic::particles::streams::apply_fractional_particle_istream(
                pin, m_fraction, std::numeric_limits<boost::int64_t>::max(), false );
            break;
        case VIEWPORT_LOAD_MODE::STRIDE:
            pin = frantic::particles::streams::apply_fractional_particle_istream(
                pin, m_fraction, std::numeric_limits<boost::int64_t>::max(), true );
            break;
        case VIEWPORT_LOAD_MODE::ID:
            pin = frantic::particles::streams::apply_fractional_by_id_particle_istream( pin, m_fraction,
                                                                                        Frost_IDCHANNEL );
            break;
        }
    }

    particles.clear();
    particles.set_channel_map( pin->get_channel_map() );
    particles.insert_particles( pin );
}

bool particle_cache_entry::is_current( double fraction, int loadMode ) {
    if( !m_hasParticles ) {
        return false;
    }
    if( fraction < 1.0 ) {
        return m_fraction == fraction && m_loadMode == loadMode;
    } else {
        return m_fraction >= 1.0;
    }
}

void particle_cache_entry::set_no_load( boost::shared_ptr<frantic::particles::streams::particle_istream> stream ) {
    m_fraction = 0;
    m_loadMode = VIEWPORT_LOAD_MODE::HEAD;
    m_age = 0;
    m_hasParticles = false;

    particles.clear();
    particles.set_channel_map( stream->get_channel_map() );
}

void particle_cache_entry::clear() {
    m_fraction = 0;
    m_loadMode = VIEWPORT_LOAD_MODE::HEAD;
    m_age = 0;
    m_hasParticles = false;
}

void particle_cache_entry::clear_and_deallocate() {
    clear();
    particles.clear();
    particles.trim();
}

frantic::particles::particle_array& particle_cache_entry::get_particles_ref() {
    m_age = 0;
    return particles;
}

void particle_cache_entry::inc_age() { ++m_age; }

void particle_cache_entry::clear_if_old() {
    if( m_age > 0 ) {
        clear();
    }
}

/////////////////////////////////////////////////////////////////////////////
//
// node_particle_cache_entry
//

node_particle_cache_entry::node_particle_cache_entry()
    : m_node( 0 )
    ,
    // m_animHandle( 0 ),
    m_iv( NEVER )
    , m_rendering( false )
    , m_renderParticles( false ) {}

void node_particle_cache_entry::set( boost::shared_ptr<frantic::particles::streams::particle_istream> stream,
                                     INode* node, bool rendering, bool renderParticles, Interval interval,
                                     double fraction, int loadMode ) {
    m_node = node;
    // m_animHandle = Animatable::GetHandleByAnim( node );
    m_iv = interval;
    m_rendering = rendering;
    m_renderParticles = renderParticles;
    m_cacheEntry.set( stream, fraction, loadMode );
}

bool node_particle_cache_entry::is_current( INode* node, bool rendering, bool renderParticles, Interval interval,
                                            double fraction, int loadMode ) {
    if( m_rendering != rendering )
        return false;
    if( m_renderParticles != renderParticles )
        return false;
    if( m_node != node )
        return false;
    // if( m_animHandle != Animatable::GetHandleByAnim( node ) )
    // return false;
    if( m_iv.InInterval( interval ) == 0 ) // Return non-zero if interval passed is contained within the interval
        return false;
    return m_cacheEntry.is_current( fraction, loadMode );
}

void node_particle_cache_entry::set_no_load( boost::shared_ptr<frantic::particles::streams::particle_istream> stream,
                                             INode* node, bool rendering, bool renderParticles, Interval interval ) {
    m_node = node;
    m_iv = interval;
    m_rendering = rendering;
    m_renderParticles = renderParticles;
    m_cacheEntry.set_no_load( stream );
}

bool node_particle_cache_entry::is_current_channel_map( INode* node, bool rendering, bool renderParticles,
                                                        Interval interval ) {
    if( m_rendering != rendering )
        return false;
    if( m_renderParticles != renderParticles )
        return false;
    if( m_node != node )
        return false;
    if( m_iv.InInterval( interval ) == 0 ) // Return non-zero if interval passed is contained within the interval
        return false;
    return true;
}

frantic::particles::particle_array& node_particle_cache_entry::get_particles_ref() {
    return m_cacheEntry.get_particles_ref();
}

void node_particle_cache_entry::clear() {
    m_node = 0;
    // m_animHandle = 0;
    m_iv = NEVER;
    m_rendering = false;
    m_renderParticles = false;
    m_cacheEntry.clear();
}

void node_particle_cache_entry::clear_and_deallocate() {
    clear();
    m_cacheEntry.clear_and_deallocate();
}

void node_particle_cache_entry::invalidate() { clear(); }

void node_particle_cache_entry::inc_age() { m_cacheEntry.inc_age(); }

void node_particle_cache_entry::clear_if_old() { m_cacheEntry.clear_if_old(); }

/////////////////////////////////////////////////////////////////////////////
//
// file_particle_cache_entry
//

file_particle_cache_entry::file_particle_cache_entry()
    : m_filename( _T("/") ) {}

void file_particle_cache_entry::set( boost::shared_ptr<frantic::particles::streams::particle_istream> stream,
                                     const frantic::tstring& filename, double fraction, int loadMode ) {
    m_filename = filename;
    m_cacheEntry.set( stream, fraction, loadMode );
}

bool file_particle_cache_entry::is_current( const frantic::tstring& filename, double fraction, int loadMode ) {
    if( m_filename != filename )
        return false;
    return m_cacheEntry.is_current( fraction, loadMode );
}

void file_particle_cache_entry::set_no_load( boost::shared_ptr<frantic::particles::streams::particle_istream> stream,
                                             const frantic::tstring& filename ) {
    m_filename = filename;
    m_cacheEntry.set_no_load( stream );
}

bool file_particle_cache_entry::is_current_channel_map( const frantic::tstring& filename ) {
    if( m_filename != filename )
        return false;
    return true;
}

frantic::particles::particle_array& file_particle_cache_entry::get_particles_ref() {
    return m_cacheEntry.get_particles_ref();
}

void file_particle_cache_entry::clear() {
    m_filename = _T("/");
    m_cacheEntry.clear();
}

void file_particle_cache_entry::clear_and_deallocate() {
    clear();
    m_cacheEntry.clear_and_deallocate();
}

void file_particle_cache_entry::invalidate() { clear(); }

void file_particle_cache_entry::inc_age() { m_cacheEntry.inc_age(); }

void file_particle_cache_entry::clear_if_old() { m_cacheEntry.clear_if_old(); }
