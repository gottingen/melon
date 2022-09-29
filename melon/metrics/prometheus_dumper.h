
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef MELON_PROMETHUES_DUMPER_H_
#define MELON_PROMETHUES_DUMPER_H_

#include <ostream>
#include "melon/metrics/variable_base.h"
#include "melon/metrics/dumper.h"
#include "melon/metrics/cache_metric.h"
#include "melon/log/logging.h"

namespace melon {

    class prometheus_dumper : public metrics_dumper {
    public:
        explicit prometheus_dumper(std::ostream *buf) : _buf(buf) {
            MELON_CHECK(_buf);
        }

        bool dump(const cache_metrics &metric, const melon::time_point *tp) override;

        static std::string dump_to_string(const cache_metrics &metric, const melon::time_point *tp);

    private:

        std::ostream *_buf;
    };
}  // namespace melon

#endif  // MELON_PROMETHUES_DUMPER_H_
