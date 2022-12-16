// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include "ScopedICustButton.hpp"

ScopedICustButton::ScopedICustButton( HWND hCtrl ) {
    m_button = GetICustButton( hCtrl );
    if( !m_button ) {
        throw std::runtime_error( "ScopedICustButton Error: Could not get ICustButton from control" );
    }
}

ScopedICustButton::~ScopedICustButton() {
    ReleaseICustButton( m_button );
    m_button = 0;
}

ICustButton* ScopedICustButton::operator->() { return m_button; }
