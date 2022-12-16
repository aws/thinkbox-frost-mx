// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/logging/progress_logger.hpp>

namespace frantic {
namespace max3d {
namespace logging {

/**
 *  Check if the render has been aborted.
 *  Does not display the progress in any way.
 */
class check_for_render_abort_progress_logger : public frantic::logging::progress_logger {
    Interface* m_ip;
    frantic::tstring m_title;

    bool is_cancelled() {
        if( m_ip ) {
            return m_ip->CheckForRenderAbort() ? true : false;
        } else {
            return false;
        }
    }

  public:
    check_for_render_abort_progress_logger( Interface* ip = GetCOREInterface(),
                                            const frantic::tstring& title = _T("Render") )
        : m_ip( ip )
        , m_title( title ) {}

    void set_title( const frantic::tstring& title ) { m_title = title; }

    void update_progress( long long /*completed*/, long long /*maximum*/ ) {
        if( is_cancelled() ) {
            throw frantic::logging::progress_cancel_exception( frantic::strings::to_string( m_title ) );
        }
    }

    void update_progress( float /*percent*/ ) {
        if( is_cancelled() ) {
            throw frantic::logging::progress_cancel_exception( frantic::strings::to_string( m_title ) );
        }
    }
};

} // namespace logging
} // namespace max3d
} // namespace frantic
