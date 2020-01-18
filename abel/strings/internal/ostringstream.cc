//

#include <abel/strings/internal/ostringstream.h>

namespace abel {
ABEL_NAMESPACE_BEGIN
namespace strings_internal {

OStringStream::Buf::int_type OStringStream::overflow(int c) {
  assert(s_);
  if (!Buf::traits_type::eq_int_type(c, Buf::traits_type::eof()))
    s_->push_back(static_cast<char>(c));
  return 1;
}

std::streamsize OStringStream::xsputn(const char* s, std::streamsize n) {
  assert(s_);
  s_->append(s, n);
  return n;
}

}  // namespace strings_internal
ABEL_NAMESPACE_END
}  // namespace abel
