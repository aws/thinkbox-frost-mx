// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
extern TCHAR* GetString( int id );

class presets_dlg_proc : public ParamMap2UserDlgProc {
    Frost* m_obj;
    HWND m_hwnd;
    std::vector<frantic::tstring> m_presetNames;

    void fill_presets( HWND hwndDlg ) {
        const frantic::tstring customSettings = _T("------Custom Settings------");
        m_presetNames.clear();
        m_obj->get_preset_name_list( m_presetNames );
        HWND hwndList = GetDlgItem( hwndDlg, LB_PRESETS );
        if( hwndList ) {
            frantic::win32::ffListBox_Clear( hwndList );
            ListBox_AddString( hwndList, customSettings.c_str() );
            frantic::win32::ffListBox_AddStrings( hwndList, m_presetNames );
        }
    }

    void select_current_preset( HWND hwndDlg ) {
        HWND hwndList = GetDlgItem( hwndDlg, LB_PRESETS );
        if( hwndList ) {
            if( ListBox_GetCount( hwndList ) <= 0 ) {
                fill_presets( hwndDlg );
            }
            const frantic::tstring preset = m_obj->get_preset_name();
            if( preset.empty() ) {
                ListBox_SetCurSel( hwndList, 0 );
            } else {
                int sel = 0;
                for( std::size_t i = 0; i < m_presetNames.size(); ++i ) {
                    if( preset == m_presetNames[i] ) {
                        sel = static_cast<int>( i + 1 );
                    }
                }
                ListBox_SetCurSel( hwndList, sel );
            }
        }
    }

    frantic::tstring get_selected_preset_name( HWND hwndDlg ) {
        HWND hwndList = GetDlgItem( hwndDlg, LB_PRESETS );
        if( hwndList ) {
            int sel = ListBox_GetCurSel( hwndList );
            if( sel == LB_ERR ) {
                return _T("");
            } else if( sel == 0 ) {
                return _T("");
            } else if( sel > 0 && static_cast<std::size_t>( sel ) <= m_presetNames.size() ) {
                return m_presetNames[sel - 1];
            } else {
                throw std::runtime_error(
                    "get_selected_preset Internal Error: selected item is out of range of the stored presets." );
            }
        } else {
            throw std::runtime_error( "get_selected_preset Internal Error: could not find presets list box." );
        }
    }

  public:
    presets_dlg_proc()
        : m_obj( 0 )
        , m_hwnd( 0 ) {}
    presets_dlg_proc( Frost* obj )
        : m_obj( obj )
        , m_hwnd( 0 ) {}

    INT_PTR DlgProc( TimeValue /*t*/, IParamMap2* map, HWND hwnd, UINT msg, WPARAM wParam, LPARAM /*lParam*/ ) {
        m_hwnd = hwnd;
        try {
            if( !m_obj ) {
                throw std::runtime_error( "Error: class object is NULL" );
            }
            if( !map ) {
                throw std::runtime_error( "Error: parameter map is NULL" );
            }
            int id = LOWORD( wParam );
            int notifyCode = HIWORD( wParam );
            switch( msg ) {
            case WM_INITDIALOG: {
                typedef std::pair<int, int> control_tooltip_t;
                control_tooltip_t buttonTooltips[] = {
                    std::make_pair( BTN_PRESETS_SAVE, IDS_PRESETS_SAVE_TOOLTIP ),
                    std::make_pair( BTN_PRESETS_SET_DEFAULT, IDS_PRESETS_SET_DEFAULT_TOOLTIP ),
                    std::make_pair( BTN_PRESETS_DELETE, IDS_PRESETS_DELETE_TOOLTIP ),
                    std::make_pair( BTN_PRESETS_EXPLORE, IDS_PRESETS_EXPLORE_TOOLTIP ) };
                BOOST_FOREACH( const control_tooltip_t& buttonTooltip, buttonTooltips ) {
                    CustButton_SetTooltip( GetDlgItem( hwnd, buttonTooltip.first ), GetString( buttonTooltip.second ) );
                }
                fill_presets( hwnd );
                select_current_preset( hwnd );
            } break;
            case WM_COMMAND:
                switch( id ) {
                case BTN_PRESETS_SAVE: {
                    notify_if_missing_frost_mxs();
                    frantic::tstring newPreset =
                        frantic::max3d::mxs::expression(
                            _T("if (FrostUi!=undefined) then (FrostUi.on_savePreset_pressed frostNode) else \"\"") )
                            .bind( _T("frostNode"), m_obj )
                            .evaluate<frantic::tstring>();
                    if( !newPreset.empty() ) {
                        m_obj->set_preset_name( newPreset );
                        fill_presets( hwnd );
                        select_current_preset( hwnd );
                    }
                } break;
                case BTN_PRESETS_SET_DEFAULT: {
                    notify_if_missing_frost_mxs();
                    const frantic::tstring presetName = get_selected_preset_name( hwnd );
                    bool changed =
                        frantic::max3d::mxs::expression(
                            _T("if (FrostUi!=undefined) then (FrostUi.on_setDefaultPreset_pressed frostNode \"") +
                            presetName + _T("\") else false") )
                            .bind( _T("frostNode"), m_obj )
                            .evaluate<bool>();
                    changed;
                } break;
                case BTN_PRESETS_DELETE: {
                    const frantic::tstring presetName = get_selected_preset_name( hwnd );
                    if( !presetName.empty() ) {
                        notify_if_missing_frost_mxs();
                        bool changed =
                            frantic::max3d::mxs::expression(
                                _T("if (FrostUi!=undefined) then (FrostUi.on_deletePreset_pressed frostNode \"") +
                                presetName + _T("\") else false") )
                                .bind( _T("frostNode"), m_obj )
                                .evaluate<bool>();
                        if( changed ) {
                            fill_presets( hwnd );
                        }
                    }
                } break;
                case BTN_PRESETS_EXPLORE: {
                    notify_if_missing_frost_mxs();
                    frantic::max3d::mxs::expression(
                        _T("if (FrostUi!=undefined) do (FrostUi.on_explorePresets_pressed() )") )
                        .evaluate<void>();
                    break;
                }
                case LB_PRESETS:
                    switch( notifyCode ) {
                    case LBN_SELCHANGE: {
                        HWND hwndList = GetDlgItem( hwnd, id );
                        if( hwndList ) {
                            const frantic::tstring presetName = get_selected_preset_name( hwnd );
                            if( !presetName.empty() ) {
                                const frantic::tstring presetFilename = Frost::get_preset_filename( presetName );
                                if( boost::filesystem::exists( boost::filesystem::path( presetFilename ) ) ) {
                                    m_obj->load_preset_from_filename( presetFilename );
                                    m_obj->set_preset_name( presetName );
                                } else {
                                    m_obj->report_warning( _T("Could not find preset file: ") + presetFilename );
                                }
                            }
                        }
                    } break;
                    }
                    break;
                }
                break;
            }
        } catch( const std::exception& e ) {
            frantic::tstring errmsg =
                _T("presets_dlg_proc::DlgProc: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
            FF_LOG( error ) << errmsg << std::endl;
            LogSys* log = GetCOREInterface()->Log();
            log->LogEntry( SYSLOG_ERROR, NO_DIALOG, _T("Frost Error"), _T("%s"), errmsg.c_str() );
            if( is_network_render_server() ) {
                throw MAXException( const_cast<TCHAR*>( errmsg.c_str() ) );
            }
        }
        return FALSE;
    }

    void Update( TimeValue /*t*/ ) {
        try {
            UpdateEnables( m_hwnd );
        } catch( const std::exception& e ) {
            frantic::tstring errmsg =
                _T("presets_dlg_proc::Update: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
            FF_LOG( error ) << errmsg << std::endl;
            LogSys* log = GetCOREInterface()->Log();
            log->LogEntry( SYSLOG_ERROR, NO_DIALOG, _T("Frost Error"), _T("%s"), errmsg.c_str() );
            if( is_network_render_server() ) {
                throw MAXException( const_cast<TCHAR*>( errmsg.c_str() ) );
            }
        }
    }

    void invalidate_preset( HWND hwnd ) {
        if( m_obj && hwnd ) {
            select_current_preset( hwnd );
        }
    }

    void UpdateEnables( HWND hwnd ) {
        if( !m_obj ) {
            return;
        }
        IParamBlock2* pb = m_obj->GetParamBlockByID( frost_param_block );
        if( !pb ) {
            return;
        }
        IParamMap2* pm = pb->GetMap( frost_meshing_param_map );
        if( !pm ) {
            return;
        }
        if( !hwnd ) {
            return;
        }
    }

    void SetThing( ReferenceTarget* obj ) { m_obj = (Frost*)obj; }

    void SetFrost( Frost* frost ) {
        m_obj = frost;
        m_hwnd = 0;
    }

    void DeleteThis() {
        // static !
        // delete this;
    }
};
