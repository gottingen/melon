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



#ifndef  MELON_RPC_HTTP_HPACK_H_
#define  MELON_RPC_HTTP_HPACK_H_

#include <melon/utility/iobuf.h>                             // mutil::IOBuf
#include <melon/utility/strings/string_piece.h>              // mutil::StringPiece
#include <melon/rpc/http/http2.h>
#include <melon/rpc/describable.h>

namespace melon {

    enum HeaderIndexPolicy {
        // Append this header, alerting the decoder dynamic table
        //  - If the given header matches one of the indexed header, this header
        //    is replaced by the index.
        //  - If not, append this header into the decoder dynamic table
        HPACK_INDEX_HEADER = 0,

        // Append this header, without alerting the decoder dynamic table
        //  - If the given header matches one of the indexed header, this header
        //    is replaced by the index.
        //  - If not, append this header directly *WITHOUT* any modification on the
        //    decoder dynamic table
        HPACK_NOT_INDEX_HEADER = 1,

        // Append this header which will never replaced by a index
        HPACK_NEVER_INDEX_HEADER = 2,
    };

    // Options to encode a header
    struct HPackOptions {

        // How to index this header field.
        // Default: HPACK_INDEX_HEADER
        HeaderIndexPolicy index_policy;

        // If true, the name string would be encoded with huffman encoding
        // Default: false
        bool encode_name;

        // If true, the value string would be encoded with huffman encoding
        // Default: false
        bool encode_value;

        // Construct default options
        HPackOptions();
    };

    inline HPackOptions::HPackOptions()
            : index_policy(HPACK_INDEX_HEADER), encode_name(false), encode_value(false) {}

    class IndexTable;

    // HPACK - Header compression algorithm for http2 (rfc7541)
    // http://httpwg.org/specs/rfc7541.html
    // Note: Name of header is assumed to be in *lowercase* acoording to
    // https://tools.ietf.org/html/rfc7540#section-8.1.2
    //      Just as in HTTP/1.x, header field names are strings of ASCII
    //      characters that are compared in a case-insensitive fashion.  However,
    //      header field names *MUST* be converted to lowercase prior to their
    //      encoding in HTTP/2.  A request or response containing uppercase
    //      header field names MUST be treated as malformed
    // Not supported methods:
    //  - Resize dynamic table.
    class HPacker : public Describable {
    public:
        struct Header {
            std::string name;
            std::string value;

            Header() {}

            explicit Header(const std::string &name2) : name(name2) {}

            Header(const std::string &name2, const std::string &value2)
                    : name(name2), value(value2) {}
        };

        HPacker();

        ~HPacker();

        // Initialize the instance.
        // Returns 0 on success, -1 otherwise.
        int Init(size_t max_table_size = H2Settings::DEFAULT_HEADER_TABLE_SIZE);

        // Encode header and append the encoded buffer to |out|
        // Returns true on success.
        void Encode(mutil::IOBufAppender *out, const Header &header,
                    const HPackOptions &options);

        void Encode(mutil::IOBufAppender *out, const Header &header) { return Encode(out, header, HPackOptions()); }

        // Try to decode at most one Header from source and erase corresponding
        // buffer.
        // Returns:
        //  * $size of decoded buffer when a header is succesfully decoded
        //  * 0 when the source is incompleted
        //  * -1 when the source is malformed
        ssize_t Decode(mutil::IOBuf *source, Header *h);

        // Like the previous function, except that the source is from
        // IOBufBytesIterator.
        ssize_t Decode(mutil::IOBufBytesIterator &source, Header *h);

        void Describe(std::ostream &os, const DescribeOptions &) const;

    private:
        DISALLOW_COPY_AND_ASSIGN(HPacker);

        int FindHeaderFromIndexTable(const Header &h) const;

        int FindNameFromIndexTable(const std::string &name) const;

        const Header *HeaderAt(int index) const;

        ssize_t DecodeWithKnownPrefix(
                mutil::IOBufBytesIterator &iter, Header *h, uint8_t prefix_size) const;

        IndexTable *_encode_table;
        IndexTable *_decode_table;
    };

// Lowercase the input string, a fast implementation.
    void tolower(std::string *s);

    inline ssize_t HPacker::Decode(mutil::IOBuf *source, Header *h) {
        mutil::IOBufBytesIterator iter(*source);
        const ssize_t nc = Decode(iter, h);
        if (nc > 0) {
            source->pop_front(nc);
        }
        return nc;
    }

} // namespace melon


#endif  // MELON_RPC_HTTP_HPACK_H_
