// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include "roi_dlg_proc.hpp"

#include <frantic/max3d/max_utility.hpp>

#include "Frost.hpp"
#include "ScopedICustButton.hpp"
#include "frost_param_block_desc.hpp"
#include "resource.h"

using frantic::max3d::is_network_render_server;

roi_dlg_proc::roi_dlg_proc()
    : m_obj( 0 )
    , m_hwnd( 0 ) {}

INT_PTR roi_dlg_proc::DlgProc( TimeValue /*t*/, IParamMap2* map, HWND hwnd, UINT msg, WPARAM wParam,
                               LPARAM /*lParam*/ ) {
    m_hwnd = hwnd;
    try {
        if( !m_obj ) {
            throw std::runtime_error( "Error: class object is NULL" );
        }
        if( !map ) {
            throw std::runtime_error( "Error: parameter map is NULL" );
        }

        int id = LOWORD( wParam );

        switch( msg ) {
        case WM_INITDIALOG: {
            ScopedICustButton button( GetDlgItem( m_hwnd, IDC_SHOW_MANIPULATOR ) );
            button->SetType( CBT_CHECK );

            update_enable_roi( hwnd );

            UpdateManipulateMode();
        } break;
        case WM_COMMAND:
            switch( id ) {
            case IDC_ENABLEROI: {
                const HWND hWndButton = GetDlgItem( hwnd, IDC_ENABLEROI );
                const LRESULT check = SendMessage( hWndButton, BM_GETCHECK, 0, 0 );
                const bool newEnable = check == BST_UNCHECKED;
                m_obj->set_enable_render_roi( newEnable );
                m_obj->set_enable_viewport_roi( newEnable );
            } break;
            case IDC_SHOW_MANIPULATOR: {
                const HWND hWndButton = GetDlgItem( hwnd, IDC_SHOW_MANIPULATOR );
                ScopedICustButton button( hWndButton );
                const BOOL check = button->IsChecked();

                Interface7* i = GetCOREInterface7();
                if( check ) {
                    i->StartManipulateMode();
                } else {
                    i->EndManipulateMode();
                }
            } break;
            }
            break;
        }
    } catch( const std::exception& e ) {
        frantic::tstring errmsg =
            _T( "roi_dlg_proc::DlgProc: " ) + frantic::strings::to_tstring( e.what() ) + _T( "\n" );
        FF_LOG( error ) << errmsg << std::endl;
        LogSys* log = GetCOREInterface()->Log();
        log->LogEntry( SYSLOG_ERROR, NO_DIALOG, _T( "Frost Error" ), _T( "%s" ), errmsg.c_str() );
        if( is_network_render_server() ) {
            throw MAXException( const_cast<TCHAR*>( errmsg.c_str() ) );
        }
    }
    return FALSE;
}

void roi_dlg_proc::Update( TimeValue /*t*/ ) {}

void roi_dlg_proc::SetFrost( Frost* frost ) {
    m_obj = frost;
    m_hwnd = 0;
}

void roi_dlg_proc::DeleteThis() {}

void roi_dlg_proc::InvalidateUI( HWND hwnd, int element ) {
    if( !hwnd ) {
        return;
    }
    switch( element ) {
    case pb_enableRenderROI:
    case pb_enableViewportROI:
        update_enable_roi( hwnd );
        UpdateManipulateMode();
        break;
    }
}

void roi_dlg_proc::UpdateManipulateMode() {
    if( !m_hwnd || !m_obj ) {
        return;
    }

    const bool enableRenderROI = m_obj->get_enable_render_roi();
    const bool enableViewportROI = m_obj->get_enable_viewport_roi();
    const bool enableROI = enableRenderROI || enableViewportROI;

    Interface7* i = GetCOREInterface7();
    const bool inManipMode = i->InManipMode() != FALSE;

    ScopedICustButton button( GetDlgItem( m_hwnd, IDC_SHOW_MANIPULATOR ) );
    button->Enable( enableROI );
    button->SetCheck( enableROI && inManipMode );
}

void roi_dlg_proc::update_enable_roi( HWND hwnd ) {
    if( !hwnd ) {
        return;
    }

    bool enableRenderROI = m_obj->get_enable_render_roi();
    bool enableViewportROI = m_obj->get_enable_viewport_roi();

    int check = BST_UNCHECKED;
    if( enableRenderROI && enableViewportROI ) {
        check = BST_CHECKED;
    } else if( enableRenderROI || enableViewportROI ) {
        check = BST_INDETERMINATE;
    } else {
        check = BST_UNCHECKED;
    }

    HWND hWndButton = GetDlgItem( hwnd, IDC_ENABLEROI );
    SendMessage( hWndButton, BM_SETCHECK, check, 0 );
}
