file(REMOVE_RECURSE
  "../output/lib/libmelon.a"
  "../output/lib/libmelon.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang C CXX)
  include(CMakeFiles/melon-static.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
