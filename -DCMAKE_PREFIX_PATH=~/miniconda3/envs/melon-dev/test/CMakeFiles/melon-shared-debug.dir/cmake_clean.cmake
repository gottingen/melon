file(REMOVE_RECURSE
  "libmelon-shared-debug.dylib"
  "libmelon-shared-debug.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang C CXX)
  include(CMakeFiles/melon-shared-debug.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
