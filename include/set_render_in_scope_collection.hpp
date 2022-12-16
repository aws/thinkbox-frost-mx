// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <boost/foreach.hpp>

#include <Animatable.h>

class set_render_in_scope_collection {
    TimeValue m_t;
    typedef std::set<Animatable*> animatables_t;
    animatables_t m_animatables;

  public:
    set_render_in_scope_collection( TimeValue t )
        : m_t( t ) {}
    ~set_render_in_scope_collection() {
        BOOST_FOREACH( Animatable* anim, m_animatables ) {
            if( anim ) {
                anim->RenderEnd( m_t );
            }
        }
    }
    bool empty() const { return m_animatables.empty(); }
    void insert( Animatable* anim ) {
        if( anim ) {
            std::pair<animatables_t::iterator, bool> result = m_animatables.insert( anim );
            if( result.second ) {
                anim->RenderBegin( m_t, 0 );
            }
        }
    }
};
