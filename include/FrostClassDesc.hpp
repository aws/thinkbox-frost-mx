// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "Frost.hpp"

extern HINSTANCE ghInstance;

class FrostClassDesc : public ClassDesc2 {
  public:
    int IsPublic() { return TRUE; }
    void* Create( BOOL loading ) { return new Frost( loading == TRUE, loading ? FALSE : TRUE ); }
    const TCHAR* ClassName() { return Frost_CLASS_NAME; }
#if MAX_VERSION_MAJOR >= 24
    const TCHAR* NonLocalizedClassName() { return Frost_CLASS_NAME; }
#endif
    SClass_ID SuperClassID() { return GEOMOBJECT_CLASS_ID; }
    Class_ID ClassID() { return Frost_CLASS_ID; }
    const TCHAR* Category() { return Frost_CATEGORY; }

    const TCHAR* InternalName() { return Frost_CLASS_NAME; } // returns fixed parsable name (scripter-visible name)
    HINSTANCE HInstance() { return ghInstance; }             // returns owning module handle
};

ClassDesc2* GetFrostClassDesc();
