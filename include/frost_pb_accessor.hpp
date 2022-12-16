// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
class frost_pb_accessor : public PBAccessor {
  public:
    // void  Get( PB2Value & v, ReferenceMaker *owner, ParamID id, int tabIndex, TimeValue t, Interval & valid );
    void Set( PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t );
    void TabChanged( tab_changes changeCode, Tab<PB2Value>* tab, ReferenceMaker* owner, ParamID id, int tabIndex,
                     int count );
};

frost_pb_accessor* GetFrostPBAccessor();
