// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/win32/utility.hpp>

#include "resource.h"

extern HINSTANCE ghInstance;

class listbox_tooltip_source {
  public:
    virtual void get_list_item_tooltip( int /*index*/, frantic::tstring& outTooltip ) = 0;
};

class listbox_with_tooltip {

    listbox_tooltip_source* m_tooltipSource;
    WNDPROC m_originalProc;
    HWND m_hwndTT;
    HWND m_hwndList;
    int m_index;
    std::vector<TCHAR> m_text;
    frantic::tstring m_textString;

    static void ffListBox_GetText( HWND hwndCtl, int index, std::vector<TCHAR>& outText ) {
        bool success = false;
        const int len = ListBox_GetTextLen( hwndCtl, index );
        if( len > 0 ) {
            outText.resize( len + 2 );
            memset( &outText[0], 0, sizeof( TCHAR ) * outText.size() );
            const int result = ListBox_GetText( hwndCtl, index, &outText[0] );
            if( result == LB_ERR ) {
                // pass
            } else {
                success = true;
            }
        }

        if( !success ) {
            outText.clear();
            outText.push_back( 0 );
        }
    }

    static HFONT get_font() { return GetCOREInterface()->GetAppHFont(); }

    static bool get_item_size( HWND hwndList, int i, SIZE& size ) {
        static std::vector<TCHAR> textBuffer;

        if( i >= 0 ) {
            HDC hdc = GetDC( hwndList );
            if( hdc ) {
                textBuffer.clear();
                ffListBox_GetText( hwndList, i, textBuffer );
                if( textBuffer.size() == 0 ) {
                    textBuffer.push_back( 0 );
                }
                HFONT hfont = get_font();
                if( HFONT hOldFont = (HFONT)SelectObject( hdc, hfont ) ) {
                    BOOL success = GetTextExtentPoint32( hdc, &textBuffer[0],
                                                         static_cast<int>( _tcsclen( &textBuffer[0] ) ), &size );
                    SelectObject( hdc, hOldFont );
                    ReleaseDC( hwndList, hdc );
                    if( success ) {
                        return true;
                    }
                }
            }
        }

        return false;
    }

    LRESULT OnGetDispInfo( HWND /*hwndList*/, NMHDR* pnm ) {
        bool success = false;
        NMTTDISPINFO* pdi = (NMTTDISPINFO*)pnm;
        if( m_index >= 0 ) {
            if( m_tooltipSource ) {
                m_tooltipSource->get_list_item_tooltip( m_index, m_textString );
                const std::size_t len = m_textString.size();
                m_text.resize( len + 1 );
                if( len > 0 ) {
                    memcpy( &m_text[0], m_textString.c_str(), len * sizeof( TCHAR ) );
                }
                m_text[len] = 0;
                success = true;
            }
        }

        if( !success ) {
            m_text.clear();
        }

        if( m_text.size() == 0 ) {
            m_text.push_back( 0 );
        }
        pdi->lpszText = &m_text[0];

        // return value is not used
        return 0;
    }

    LRESULT OnNotify( HWND hwnd, int /*idFrom*/, NMHDR* pnm ) {
        if( pnm->hwndFrom == m_hwndTT ) {
            switch( pnm->code ) {
            case TTN_GETDISPINFO:
                return OnGetDispInfo( hwnd, pnm );
            }
        }
        return 0;
    }

    int ItemHitTest( int x, int y ) {
        const int topIndex = ListBox_GetTopIndex( m_hwndList );
        const int count = ListBox_GetCount( m_hwndList );
        if( topIndex < count ) {
            int height = ListBox_GetItemHeight( m_hwndList, 0 );
            if( height > 0 ) {
                const int index = topIndex + y / height;
                if( index >= 0 ) {
                    RECT rect;
                    LRESULT result = SendMessage( m_hwndList, LB_GETITEMRECT, index, (LPARAM)( &rect ) );
                    if( result != LB_ERR ) {
                        POINT pt;
                        pt.x = x;
                        pt.y = y;
                        if( PtInRect( &rect, pt ) ) {
                            return index;
                        }
                    }
                }
            }
        }
        return -1;
    }

    void UpdateTooltip( int x, int y ) {
        int iItemOld = m_index;
        m_index = ItemHitTest( x, y );
        if( iItemOld != m_index ) {
            SendMessage( m_hwndTT, TTM_POP, 0, 0 );
        }
    }

    void RelayEvent( HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam ) {
        if( uiMsg == WM_MOUSEMOVE ) {
            UpdateTooltip( GET_X_LPARAM( lParam ), GET_Y_LPARAM( lParam ) );
        }
        MSG msg;
        msg.hwnd = hwnd;
        msg.message = uiMsg;
        msg.wParam = wParam;
        msg.lParam = lParam;
        SendMessage( m_hwndTT, TTM_RELAYEVENT, 0, (LPARAM)&msg );
    }

  public:
    listbox_with_tooltip()
        : m_tooltipSource( 0 )
        , m_originalProc( 0 )
        , m_hwndTT( 0 )
        , m_index( -1 ) {}

    void set_tooltip_source( listbox_tooltip_source* tooltipSource ) { m_tooltipSource = tooltipSource; }

    static LRESULT APIENTRY WndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
        listbox_with_tooltip* instance = (listbox_with_tooltip*)( GetWindowLongPtr( hwnd, GWLP_USERDATA ) );
        if( instance ) {
            if( ( uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST ) || uMsg == WM_NCMOUSEMOVE ) {
                instance->RelayEvent( hwnd, uMsg, wParam, lParam );
            }
            switch( uMsg ) { HANDLE_MSG( hwnd, WM_NOTIFY, instance->OnNotify ); }
            return CallWindowProc( instance->m_originalProc, hwnd, uMsg, wParam, lParam );
        }
        return 0;
    }

    void attach( HWND hwndList ) {
        if( 0 == GetWindowLongPtr( hwndList, GWLP_USERDATA ) ) {
            m_index = -1;
            m_hwndList = hwndList;

            m_hwndTT =
                CreateWindowEx( WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, TTS_NOPREFIX | TTS_NOANIMATE | TTS_ALWAYSTIP, 0, 0,
                                0, 0, hwndList, NULL, ghInstance, NULL );
            SetWindowFont( m_hwndTT, get_font(), FALSE );
            SendMessage( m_hwndTT, TTM_SETMAXTIPWIDTH, 0, 480 );
            SendMessage( m_hwndTT, TTM_SETDELAYTIME, TTDT_AUTOPOP, 32767 );

            // get rect of list box
            RECT rect;
            GetWindowRect( hwndList, &rect );
            rect.right -= rect.left;
            rect.bottom -= rect.top;
            rect.left = 0;
            rect.top = 0;

            SetWindowLongPtr( hwndList, GWLP_USERDATA, (LONG_PTR)this );

            m_originalProc =
                (WNDPROC)SetWindowLongPtr( hwndList, GWLP_WNDPROC, (LONG_PTR)&listbox_with_tooltip::WndProc );

            TOOLINFO ti = { sizeof( ti ) };
            ti.uFlags = TTF_TRANSPARENT; /* | TTF_SUBCLASS;*/
            ti.hwnd = hwndList;
            ti.uId = 0;
            ti.lpszText = LPSTR_TEXTCALLBACK;
            ti.rect = rect;
            SendMessage( m_hwndTT, TTM_ADDTOOL, 0, (LPARAM)&ti );
        }
    }

    void detach( HWND hwndList ) {
        if( hwndList == m_hwndList && m_originalProc ) {
            SetWindowLongPtr( hwndList, GWLP_USERDATA, 0 );
            SetWindowLongPtr( hwndList, GWLP_WNDPROC, (LONG_PTR)m_originalProc );
            m_originalProc = 0;
            m_hwndList = 0;
            m_index = -1;
        }
    }
};
