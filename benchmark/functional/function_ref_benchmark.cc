//

#include <abel/functional/function_ref.h>

#include <memory>

#include <benchmark/benchmark.h>
#include <abel/base/profile.h>

namespace abel {

namespace {

int dummy = 0;

void FreeFunction() { benchmark::DoNotOptimize(dummy); }

struct TrivialFunctor {
  void operator()() const { benchmark::DoNotOptimize(dummy); }
};

struct LargeFunctor {
  void operator()() const { benchmark::DoNotOptimize(this); }
  std::string a, b, c;
};

template <typename Function, typename... Args>
void ABEL_NO_INLINE CallFunction(Function f, Args&&... args) {
  f(std::forward<Args>(args)...);
}

template <typename Function, typename Callable, typename... Args>
void ConstructAndCallFunctionBenchmark(benchmark::State& state,
                                       const Callable& c, Args&&... args) {
  for (auto _ : state) {
    CallFunction<Function>(c, std::forward<Args>(args)...);
  }
}

void BM_TrivialStdFunction(benchmark::State& state) {
  ConstructAndCallFunctionBenchmark<std::function<void()>>(state,
                                                           TrivialFunctor{});
}
BENCHMARK(BM_TrivialStdFunction);

void BM_TrivialFunctionRef(benchmark::State& state) {
  ConstructAndCallFunctionBenchmark<FunctionRef<void()>>(state,
                                                         TrivialFunctor{});
}
BENCHMARK(BM_TrivialFunctionRef);

void BM_LargeStdFunction(benchmark::State& state) {
  ConstructAndCallFunctionBenchmark<std::function<void()>>(state,
                                                           LargeFunctor{});
}
BENCHMARK(BM_LargeStdFunction);

void BM_LargeFunctionRef(benchmark::State& state) {
  ConstructAndCallFunctionBenchmark<FunctionRef<void()>>(state, LargeFunctor{});
}
BENCHMARK(BM_LargeFunctionRef);

void BM_FunPtrStdFunction(benchmark::State& state) {
  ConstructAndCallFunctionBenchmark<std::function<void()>>(state, FreeFunction);
}
BENCHMARK(BM_FunPtrStdFunction);

void BM_FunPtrFunctionRef(benchmark::State& state) {
  ConstructAndCallFunctionBenchmark<FunctionRef<void()>>(state, FreeFunction);
}
BENCHMARK(BM_FunPtrFunctionRef);

// Doesn't include construction or copy overhead in the loop.
template <typename Function, typename Callable, typename... Args>
void CallFunctionBenchmark(benchmark::State& state, const Callable& c,
                           Args... args) {
  Function f = c;
  for (auto _ : state) {
    benchmark::DoNotOptimize(&f);
    f(args...);
  }
}

struct FunctorWithTrivialArgs {
  void operator()(int a, int b, int c) const {
    benchmark::DoNotOptimize(a);
    benchmark::DoNotOptimize(b);
    benchmark::DoNotOptimize(c);
  }
};

void BM_TrivialArgsStdFunction(benchmark::State& state) {
  CallFunctionBenchmark<std::function<void(int, int, int)>>(
      state, FunctorWithTrivialArgs{}, 1, 2, 3);
}
BENCHMARK(BM_TrivialArgsStdFunction);

void BM_TrivialArgsFunctionRef(benchmark::State& state) {
  CallFunctionBenchmark<FunctionRef<void(int, int, int)>>(
      state, FunctorWithTrivialArgs{}, 1, 2, 3);
}
BENCHMARK(BM_TrivialArgsFunctionRef);

struct FunctorWithNonTrivialArgs {
  void operator()(std::string a, std::string b, std::string c) const {
    benchmark::DoNotOptimize(&a);
    benchmark::DoNotOptimize(&b);
    benchmark::DoNotOptimize(&c);
  }
};

void BM_NonTrivialArgsStdFunction(benchmark::State& state) {
  std::string a, b, c;
  CallFunctionBenchmark<
      std::function<void(std::string, std::string, std::string)>>(
      state, FunctorWithNonTrivialArgs{}, a, b, c);
}
BENCHMARK(BM_NonTrivialArgsStdFunction);

void BM_NonTrivialArgsFunctionRef(benchmark::State& state) {
  std::string a, b, c;
  CallFunctionBenchmark<
      FunctionRef<void(std::string, std::string, std::string)>>(
      state, FunctorWithNonTrivialArgs{}, a, b, c);
}
BENCHMARK(BM_NonTrivialArgsFunctionRef);

}  // namespace

}  // namespace abel
