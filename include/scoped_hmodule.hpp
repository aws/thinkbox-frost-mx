// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <boost/noncopyable.hpp>

class scoped_hmodule : boost::noncopyable {
  public:
    scoped_hmodule()
        : m_module( 0 ) {}

    scoped_hmodule( HMODULE module )
        : m_module( module ) {}

    ~scoped_hmodule() {
        if( m_module ) {
            FreeLibrary( m_module );
        }
        m_module = 0;
    }

    HMODULE get() { return m_module; }

    HMODULE release() {
        HMODULE result = m_module;
        m_module = 0;
        return result;
    }

    void reset( HMODULE module ) {
        if( m_module ) {
            FreeLibrary( m_module );
        }
        m_module = module;
    }

  private:
    HMODULE m_module;
};
