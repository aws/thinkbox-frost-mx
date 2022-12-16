// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include <boost/foreach.hpp>

class filter_particle_flow_events_dlg_state {
  public:
    std::vector<INode*> events;
    std::vector<LRESULT> eventListIndex;
};

class add_particle_flow_events_dlg_proc {
  public:
    static INT_PTR CALLBACK DlgProc( __in HWND hwndDlg, __in UINT uMsg, __in WPARAM wParam, __in LPARAM lParam ) {
        try {
            filter_particle_flow_events_dlg_state* pWindow =
                reinterpret_cast<filter_particle_flow_events_dlg_state*>( GetWindowLongPtr( hwndDlg, DWLP_USER ) );

            switch( uMsg ) {
            case WM_INITDIALOG: {
                pWindow = reinterpret_cast<filter_particle_flow_events_dlg_state*>( lParam );

                SetWindowLongPtr( hwndDlg, DWLP_USER, (LONG_PTR)pWindow );

                HWND hwndListBox = GetDlgItem( hwndDlg, IDC_EVENT_LIST );

                if( pWindow ) {
                    pWindow->eventListIndex.clear();
                    pWindow->eventListIndex.resize( pWindow->events.size(), LB_ERR );

                    if( hwndListBox ) {
                        SendMessage( hwndListBox, LB_RESETCONTENT, 0, 0 );

                        LPARAM prealloc = 0;

                        BOOST_FOREACH( INode* node, pWindow->events ) {
                            if( node ) {
                                prealloc += 1 + strlen( node->GetName() );
                            }
                        }

                        LRESULT lRet = SendMessage( hwndListBox, LB_INITSTORAGE, pWindow->events.size(), prealloc );

                        for( std::size_t i = 0, ie = pWindow->events.size(); i < ie; ++i ) {
                            INode* node = pWindow->events[i];
                            if( node ) {
                                lRet = SendMessage( hwndListBox, LB_ADDSTRING, 0, (LPARAM)( node->GetName() ) );
                                if( lRet == LB_ERR ) {
                                    // An error occurred
                                    MessageBox(
                                        GetCOREInterface()->GetMAXHWnd(),
                                        "An error occurred while populating the node list.  Please contact support.",
                                        "Frost : Error", MB_ICONEXCLAMATION | MB_OK );
                                } else if( lRet == LB_ERRSPACE ) {
                                    // Insufficient space to store new string
                                    MessageBox( GetCOREInterface()->GetMAXHWnd(),
                                                "Insufficient space to store the node list.  Please contact support.",
                                                "Frost : Error", MB_ICONEXCLAMATION | MB_OK );
                                } else if( lRet >= 0 ) {
                                    pWindow->eventListIndex[i] = lRet;
                                } else {
                                    // unknown error occurred
                                    MessageBox( GetCOREInterface()->GetMAXHWnd(),
                                                "An unknown error occurred while populating the node list.  Please "
                                                "contact support.",
                                                "Frost : Error", MB_ICONEXCLAMATION | MB_OK );
                                }
                            }
                        }

                    } else {
                        MessageBox( GetCOREInterface()->GetMAXHWnd(),
                                    "Internal Error: unable to find node list control.  Please contact support.",
                                    "Frost : Error", MB_ICONEXCLAMATION | MB_OK );
                    }
                } else {
                    MessageBox( GetCOREInterface()->GetMAXHWnd(),
                                "Internal Error: node list state is NULL.  Please contact support.", "Frost : Error",
                                MB_ICONEXCLAMATION | MB_OK );
                }

                return TRUE;
            }
            case WM_DESTROY:
                SetWindowLongPtr( hwndDlg, DWLP_USER, 0 );
                break;
            case WM_COMMAND: {
                int id = LOWORD( wParam );
                // int notifyCode = HIWORD( wParam );

                switch( id ) {
                case IDC_ALL: {
                    HWND hwndListBox = GetDlgItem( hwndDlg, IDC_EVENT_LIST );
                    if( hwndListBox ) {
                        SendMessage( hwndListBox, LB_SETSEL, TRUE, -1 );
                    }
                } break;
                case IDC_NONE: {
                    HWND hwndListBox = GetDlgItem( hwndDlg, IDC_EVENT_LIST );
                    if( hwndListBox ) {
                        SendMessage( hwndListBox, LB_SETSEL, FALSE, -1 );
                    }
                } break;
                case IDC_INVERT: {
                    HWND hwndListBox = GetDlgItem( hwndDlg, IDC_EVENT_LIST );
                    if( hwndListBox ) {
                        LRESULT itemCount = SendMessage( hwndListBox, LB_GETCOUNT, 0, 0 );
                        if( itemCount != LB_ERR && itemCount > 0 ) {
                            for( LRESULT i = 0; i < itemCount; ++i ) {
                                LRESULT currentState = SendMessage( hwndListBox, LB_GETSEL, i, 0 );
                                if( currentState != LB_ERR ) {
                                    LRESULT newState = currentState == 0 ? TRUE : FALSE;
                                    SendMessage( hwndListBox, LB_SETSEL, newState, i );
                                }
                            }
                        }
                    }
                } break;
                case IDOK: {
                    if( pWindow ) {
                        HWND hwndListBox = GetDlgItem( hwndDlg, IDC_EVENT_LIST );
                        if( hwndListBox ) {
                            LRESULT selCount = SendMessage( hwndListBox, LB_GETSELCOUNT, 0, 0 );
                            std::vector<INode*> out;
                            if( selCount == LB_ERR ) {
                            } else if( selCount > 0 ) {
                                for( std::size_t i = 0, ie = pWindow->eventListIndex.size(); i < ie; ++i ) {
                                    LRESULT listIndex = pWindow->eventListIndex[i];
                                    if( listIndex != LB_ERR && listIndex >= 0 ) {
                                        LRESULT selState = SendMessage( hwndListBox, LB_GETSEL, listIndex, 0 );
                                        if( selState > 0 ) {
                                            out.push_back( pWindow->events[i] );
                                        }
                                    }
                                }
                            }
                            pWindow->events.swap( out );
                            EndDialog( hwndDlg, IDOK );
                            break;
                        }
                    }
                    EndDialog( hwndDlg, IDCANCEL );
                    break;
                }
                case IDCANCEL:
                    EndDialog( hwndDlg, IDCANCEL );
                    break;
                }
            } break;
            default:
                return FALSE;
            }

            return TRUE;
        } catch( const std::exception& e ) {
            frantic::tstring errmsg = _T("add_particle_flow_events_dlg_proc::DlgProc: ") +
                                      frantic::strings::to_tstring( e.what() ) + _T("\n");
            FF_LOG( error ) << errmsg << std::endl;
            LogSys* log = GetCOREInterface()->Log();
            log->LogEntry( SYSLOG_ERROR, NO_DIALOG, _T("Frost Error"), _T("%s"), errmsg.c_str() );
            if( is_network_render_server() ) {
                throw MAXException( const_cast<TCHAR*>( errmsg.c_str() ) );
            }
        }
        return FALSE;
    }
};

static void show_filter_particle_flow_events_dialog( HINSTANCE hinstance, HWND hwnd, const std::vector<INode*>& in,
                                                     std::vector<INode*>& out ) {
    out.clear();

    filter_particle_flow_events_dlg_state state;
    state.events = in;

    INT_PTR result = DialogBoxParam( hinstance, MAKEINTRESOURCE( IDD_SELECT_PARTICLE_FLOW_EVENTS ), hwnd,
                                     &add_particle_flow_events_dlg_proc::DlgProc, (LPARAM)( &state ) );
    if( result == IDOK ) {
        out.swap( state.events );
    }
}
