// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "stdafx.h"

#include <tbb/task_scheduler_init.h>

extern ClassDesc2* GetFrostClassDesc();

extern void InitializeFrostLogging();

HINSTANCE ghInstance = 0;

namespace {
// Put all the classdesc's in a vector, to make adding & removing plugins easier
std::vector<ClassDesc*> classDescList;
} // anonymous namespace

BOOL FrostInitialize() {
    classDescList.clear();
    classDescList.push_back( GetFrostClassDesc() );
    return true;
}

BOOL APIENTRY DllMain( HANDLE hModule, DWORD ul_reason_for_call, LPVOID /*lpReserved*/ ) {
    if( ul_reason_for_call == DLL_PROCESS_ATTACH ) {
        ghInstance = (HINSTANCE)hModule;
        return FrostInitialize();
    }

    return true;
}

TCHAR* GetString( int id ) {
    static TCHAR buf[256];

    if( ghInstance )
        return LoadString( ghInstance, id, buf, sizeof( buf ) / sizeof( TCHAR ) ) ? buf : NULL;
    return NULL;
}

static tbb::task_scheduler_init g_tbbScheduler( tbb::task_scheduler_init::deferred );

extern "C" {
//------------------------------------------------------
// This is the interface to 3DSMax
//------------------------------------------------------
__declspec( dllexport ) const TCHAR* LibDescription() {
    return _T("Frost - Thinkbox Software - www.thinkboxsoftware.com");
}

__declspec( dllexport ) int LibNumberClasses() { return (int)classDescList.size(); }

__declspec( dllexport ) ClassDesc* LibClassDesc( int i ) { return classDescList[i]; }

// Return version so can detect obsolete DLLs
__declspec( dllexport ) ULONG LibVersion() { return VERSION_3DSMAX; }

// Let the plug-in register itself for deferred loading
__declspec( dllexport ) ULONG CanAutoDefer() {
#if MAX_VERSION_MAJOR >= 14
    return 1;
#else
    return 0;
#endif
}

__declspec( dllexport ) int LibInitialize() {
    try {
        g_tbbScheduler.initialize();
        // BEWARE!!  InitializeFrostLogging() schedules a callback from 3ds Max.
        // If LibInitialize returns FALSE after this callback is registered,
        // then 3ds Max will crash.
        InitializeFrostLogging();
        return TRUE;
    } catch( std::exception& e ) {
        Interface* coreInterface = GetCOREInterface();
        if( coreInterface ) {
            const frantic::tstring errmsg =
                _T("Unable to initialize Frost: ") + frantic::strings::to_tstring( e.what() );
            LogSys* log = GetCOREInterface()->Log();
            if( log ) {
                log->LogEntry( SYSLOG_ERROR, DISPLAY_DIALOG, _T("Frost"), _T("%s"), errmsg.c_str() );
            }
        }
    }
    return FALSE;
}

__declspec( dllexport ) int LibShutdown() {
    // IMPORTANT! This is in LibShutdown since the destructor on g_tbbScheduler was hanging
    //  under certain conditions.
    g_tbbScheduler.terminate();
    return TRUE;
}

} // extern "C"
