//

#include "abel/random/seed_sequences.h"

#include "abel/random/engine/pool_urbg.h"

namespace abel {


seed_seq make_seed_seq() {
    seed_seq::result_type seed_material[8];
    random_internal::randen_pool<uint32_t>::fill(abel::make_span(seed_material));
    return seed_seq(std::begin(seed_material), std::end(seed_material));
}


}  // namespace abel
