// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/particles/streams/culling_particle_istream.hpp>

class box_culling_policy : public frantic::particles::streams::culling_policy_base<box_culling_policy> {
  public:
    box_culling_policy( const frantic::graphics::boundbox3f& bounds, const frantic::channels::channel_map& pcm )
        : m_bounds( bounds ) {
        set_channel_map( pcm );
    }

    // Need a splitting constructor to support TBB
    box_culling_policy( const box_culling_policy& other, tbb::split )
        : m_positionAcc( other.m_positionAcc )
        , m_bounds( other.m_bounds ) {}

    void set_channel_map( const frantic::channels::channel_map& pcm ) {
        m_positionAcc = pcm.get_accessor<frantic::graphics::vector3f>( _T("Position") );
    }

    bool cull( char* particle ) const { return !m_bounds.contains( m_positionAcc.get( particle ) ); }

  private:
    frantic::channels::channel_accessor<frantic::graphics::vector3f> m_positionAcc;
    frantic::graphics::boundbox3f m_bounds;
};

typedef frantic::particles::streams::culling_particle_istream<box_culling_policy> box_culling_particle_istream;
