//

#ifndef ABEL_BASE_INTERNAL_IDENTITY_H_
#define ABEL_BASE_INTERNAL_IDENTITY_H_

#include <abel/base/profile.h>

namespace abel {
ABEL_NAMESPACE_BEGIN
namespace internal {

template <typename T>
struct identity {
  typedef T type;
};

template <typename T>
using identity_t = typename identity<T>::type;

}  // namespace internal
ABEL_NAMESPACE_END
}  // namespace abel

#endif  // ABEL_BASE_INTERNAL_IDENTITY_H_
