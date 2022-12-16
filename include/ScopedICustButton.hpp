// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
class ScopedICustButton {
  public:
    ScopedICustButton( HWND hCtrl );
    ~ScopedICustButton();

    ICustButton* operator->();

  private:
    ICustButton* m_button;
};
