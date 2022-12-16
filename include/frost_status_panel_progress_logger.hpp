// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <boost/assign/list_of.hpp>
#include <boost/foreach.hpp>

#include <frantic/max3d/logging/status_panel_progress_logger.hpp>

#include <maxversion.h>

using namespace boost::assign;

// TODO: improve how this is handled..
struct repost_message_state {
    std::set<UINT> lookForMessage;
    std::vector<MSG> repostMessages;
    std::map<UINT, UINT> upToDownEventMap;

    repost_message_state() {
        boost::array<UINT, 10> messages = { WM_LBUTTONDOWN, WM_LBUTTONUP, WM_MBUTTONDOWN, WM_MBUTTONUP,
                                            WM_RBUTTONDOWN, WM_RBUTTONUP, WM_XBUTTONDOWN, WM_XBUTTONUP,
                                            WM_KEYDOWN,     WM_KEYUP };

        lookForMessage.insert( messages.begin(), messages.end() );
    }

    bool looking_for( const MSG& msg ) { return ( lookForMessage.find( msg.message ) != lookForMessage.end() ); }
    void stop_looking_for( UINT message ) { lookForMessage.erase( message ); }
    void stop_looking_for( const MSG& msg ) { stop_looking_for( msg.message ); }
    void repost_message( const MSG& msg ) { repostMessages.push_back( msg ); }
    UINT get_down_message( const MSG& msg ) {
        switch( msg.message ) {
        case WM_LBUTTONUP:
            return WM_LBUTTONDOWN;
        case WM_RBUTTONUP:
            return WM_RBUTTONDOWN;
        case WM_MBUTTONUP:
            return WM_MBUTTONDOWN;
        case WM_XBUTTONUP:
            return WM_XBUTTONDOWN;
        case WM_KEYUP:
            return WM_KEYDOWN;
        default:
            return 0;
        }
    }
    bool has_down_message( const MSG& msg ) { return get_down_message( msg ) != 0; }

    void process_message( const MSG& msg ) {
        switch( msg.message ) {
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_XBUTTONDOWN:
        case WM_KEYDOWN:
            if( looking_for( msg ) ) {
                stop_looking_for( msg );
                repost_message( msg );
            }
            break;
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        case WM_XBUTTONUP:
        case WM_KEYUP:
            if( looking_for( msg ) ) {
                stop_looking_for( msg );
                repost_message( msg );
            }

            if( has_down_message( msg ) ) {
                stop_looking_for( get_down_message( msg ) );
            }
            break;
        }
    }

    void post_messages() {
        BOOST_FOREACH( MSG& msg, repostMessages ) {
            PostMessage( msg.hwnd, msg.message, msg.wParam, msg.lParam );
        }
    }
};

#if MAX_VERSION_MAJOR < 12

class frost_status_panel_progress_logger : public frantic::max3d::logging::status_panel_progress_logger {
    repost_message_state m_repostMessageState;

  protected:
    bool is_esc_pressed() {
        bool pressed = false;

        MSG msg;
        UINT wMsgFilterMin = WM_KEYFIRST;
        UINT wMsgFilterMax = WM_MOUSELAST;
        UINT wRemoveMsg = PM_NOYIELD /*PM_QS_INPUT*/ | PM_REMOVE;

        // checks if there are any new keyboard or mouse messages
        // if( GetInputState() ){
        // check for esc key presses
        while( PeekMessage( &msg, NULL, wMsgFilterMin, wMsgFilterMax, wRemoveMsg ) ) {
            m_repostMessageState.process_message( msg );

            switch( msg.message ) {
            case WM_KEYDOWN:
            case WM_KEYUP:
                if( msg.wParam == VK_ESCAPE ) {
                    pressed = true;
                }
                break;
            case WM_QUIT:
                PostQuitMessage( static_cast<int>( msg.wParam ) );
                pressed = true;
                break;
            }
        }

        m_repostMessageState.post_messages();
        //}

        return pressed;
    }

  public:
    frost_status_panel_progress_logger( float progressStart = 0, float progressEnd = 100, int delay = 0,
                                        std::string msgStart = "Progress: " )
        : frantic::max3d::logging::status_panel_progress_logger( progressStart, progressEnd, delay, msgStart ) {}
};

#else // MAX_VERSION_MAJOR >= 12

#include <WindowsMessageFilter.h>

// TODO : I don't like this but I'm not sure how to handle it..
// If we don't somehow pass on the LBUTTONUP message, then spinners can
// become stuck.  What other controls can be affected ?
// It seems like we should not be in edit/modify anyways..
class StatusPanelWindowsMessageFilter : public MaxSDK::WindowsMessageFilter {
    repost_message_state* m_repostMessageState;

  public:
    StatusPanelWindowsMessageFilter( repost_message_state* repostMessageState = 0 )
        : m_repostMessageState( repostMessageState ) {}
    bool IsAcceptableMessage( const MSG& msg ) const {
        if( m_repostMessageState ) {
            m_repostMessageState->process_message( msg );
        }
        return MaxSDK::WindowsMessageFilter::IsAcceptableMessage( msg );
    }
};

class frost_status_panel_progress_logger : public frantic::max3d::logging::status_panel_progress_logger {
    repost_message_state m_repostMessageState;

  protected:
    bool is_esc_pressed() {
        bool pressed = false;

        StatusPanelWindowsMessageFilter messageFilter( &m_repostMessageState );

        messageFilter.RunNonBlockingMessageLoop();

        if( messageFilter.Aborted() ) {
            pressed = true;
        }

        m_repostMessageState.post_messages();

        return pressed;
    }

  public:
    frost_status_panel_progress_logger( float progressStart = 0, float progressEnd = 100, int delay = 0,
                                        const frantic::tstring& msgStart = _T("Progress: ") )
        : frantic::max3d::logging::status_panel_progress_logger( progressStart, progressEnd, delay, msgStart ) {}
};

#endif
