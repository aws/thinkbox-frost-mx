// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#if defined(PHONEIX_SDK_AVAILABLE)
#include <frantic/particles/streams/particle_istream.hpp>

class INode;
typedef int TimeValue;

std::pair<bool, int> IsPhoenixObject( INode* pNode, ObjectState& os, TimeValue t );

boost::shared_ptr<frantic::particles::streams::particle_istream>
GetPhoenixParticleIstream( INode* pNode, TimeValue t, const frantic::channels::channel_map& pcm );
#endif
