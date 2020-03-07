//

#include <cstdlib>

#include <abel/hash/hash.h>

// Prints the hash of argv[1].
int main(int argc, char **argv) {
    if (argc < 2) return 1;
    printf("%zu\n", abel::hash<int>{}(std::atoi(argv[1])));  // NOLINT
}
