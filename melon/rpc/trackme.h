//
// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//



#ifndef MELON_RPC_TRACKME_H_
#define MELON_RPC_TRACKME_H_

// [Internal] RPC users are not supposed to call functions below. 

#include <melon/base/endpoint.h>


namespace melon {

// Set the server address for reporting.
// Currently only the first address will be saved.
void SetTrackMeAddress(mutil::EndPoint pt);

// Call this function every second (or every several seconds) to send
// TrackMeRequest to -trackme_server every TRACKME_INTERVAL seconds.
void TrackMe();

} // namespace melon


#endif // MELON_RPC_TRACKME_H_
