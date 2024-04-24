// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//



#ifndef MELON_RPC_TRACKME_H_
#define MELON_RPC_TRACKME_H_

// [Internal] RPC users are not supposed to call functions below. 

#include "melon/utility/endpoint.h"


namespace melon {

// Set the server address for reporting.
// Currently only the first address will be saved.
void SetTrackMeAddress(mutil::EndPoint pt);

// Call this function every second (or every several seconds) to send
// TrackMeRequest to -trackme_server every TRACKME_INTERVAL seconds.
void TrackMe();

} // namespace melon


#endif // MELON_RPC_TRACKME_H_
