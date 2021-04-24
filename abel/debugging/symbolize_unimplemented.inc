// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com
//

#include <cstdint>
#include "abel/log/logging.h"

namespace abel {


namespace debugging_internal {

int InstallSymbolDecorator(SymbolDecorator, void *) { return -1; }

bool RemoveSymbolDecorator(int) { return false; }

bool RemoveAllSymbolDecorators(void) { return false; }

bool RegisterFileMappingHint(const void *, const void *, uint64_t, const char *) {
    return false;
}

bool GetFileMappingHint(const void **, const void **, uint64_t *, const char **) {
    return false;
}

}  // namespace debugging_internal

void initialize_symbolizer(const char *) {}

bool symbolize(const void *, char *, int) { return false; }


}  // namespace abel
