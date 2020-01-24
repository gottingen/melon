//

#include <cstdint>

#include <abel/log/raw_logging.h>

namespace abel {


namespace debugging_internal {

int InstallSymbolDecorator(SymbolDecorator, void*) { return -1; }
bool RemoveSymbolDecorator(int) { return false; }
bool RemoveAllSymbolDecorators(void) { return false; }
bool RegisterFileMappingHint(const void *, const void *, uint64_t, const char *) {
  return false;
}
bool GetFileMappingHint(const void **, const void **, uint64_t *, const char **) {
  return false;
}

}  // namespace debugging_internal

void InitializeSymbolizer(const char*) {}
bool Symbolize(const void *, char *, int) { return false; }


}  // namespace abel
