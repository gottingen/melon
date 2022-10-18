file(REMOVE_RECURSE
  "../output/lib/libmelon.dylib"
  "../output/lib/libmelon.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang C CXX)
  include(CMakeFiles/melon-shared.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
