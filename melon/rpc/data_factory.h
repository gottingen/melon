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



#pragma once


namespace melon {

    // ---- thread safety ----
    // Method implementations of this interface should be thread-safe
    class DataFactory {
    public:
        virtual ~DataFactory() {}

        // Implement this method to create a piece of data
        // Returns the data, NULL on error.
        virtual void *CreateData() const = 0;

        // Implement this method to destroy data created by Create().
        virtual void DestroyData(void *) const = 0;

        // Overwrite this method to reset the data before reuse. Nothing done by default.
        // Returns
        //   true:  the data can be kept for future reuse
        //   false: the data is improper to be reused and should be sent to
        //          DestroyData() immediately after calling this method
        virtual bool ResetData(void *) const { return true; }
    };

} // namespace melon
