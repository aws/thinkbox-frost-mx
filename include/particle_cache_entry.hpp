// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <string>

#include <boost/shared_ptr.hpp>

#include <frantic/particles/particle_array.hpp>
#include <frantic/particles/streams/particle_istream.hpp>

class particle_cache_entry {
    frantic::particles::particle_array particles;

    int m_age;

    bool m_hasParticles;

    double m_fraction;
    int m_loadMode;

  public:
    particle_cache_entry();
    particle_cache_entry& operator=( const particle_cache_entry& );
    void set( boost::shared_ptr<frantic::particles::streams::particle_istream> stream, double fraction, int loadMode );
    bool is_current( double fraction, int loadMode );
    void set_no_load( boost::shared_ptr<frantic::particles::streams::particle_istream> stream );
    void clear();
    void clear_and_deallocate();
    frantic::particles::particle_array& get_particles_ref();

    void inc_age();
    void clear_if_old();
};

class node_particle_cache_entry {
    particle_cache_entry m_cacheEntry;
    INode* m_node;
    // AnimHandle m_animHandle;
    Interval m_iv;
    bool m_rendering;
    bool m_renderParticles;

  public:
    node_particle_cache_entry();
    void set( boost::shared_ptr<frantic::particles::streams::particle_istream> stream, INode* node, bool rendering,
              bool renderParticles, Interval interval, double fraction, int loadMode );
    bool is_current( INode* node, bool rendering, bool renderParticles, Interval interval, double fraction,
                     int loadMode );
    void set_no_load( boost::shared_ptr<frantic::particles::streams::particle_istream> stream, INode* node,
                      bool rendering, bool renderParticles, Interval interval );
    bool is_current_channel_map( INode* node, bool rendering, bool renderParticles, Interval interval );
    frantic::particles::particle_array& get_particles_ref();
    void clear();
    void clear_and_deallocate();
    void invalidate();

    void inc_age();
    void clear_if_old();
};

class file_particle_cache_entry {
    particle_cache_entry m_cacheEntry;
    frantic::tstring m_filename;

  public:
    file_particle_cache_entry();
    void set( boost::shared_ptr<frantic::particles::streams::particle_istream> stream, const frantic::tstring& filename,
              double fraction, int loadMode );
    bool is_current( const frantic::tstring& filename, double fraction, int loadMode );
    void set_no_load( boost::shared_ptr<frantic::particles::streams::particle_istream> stream,
                      const frantic::tstring& filename );
    bool is_current_channel_map( const frantic::tstring& filename );
    frantic::particles::particle_array& get_particles_ref();
    void clear();
    void clear_and_deallocate();
    void invalidate();

    void inc_age();
    void clear_if_old();
};
