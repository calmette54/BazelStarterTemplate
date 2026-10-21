#include <benchmark/benchmark.h>

#include "lib/greeter/include/greeter.h"

static void BM_greeter(benchmark::State& state) {
  const greeter::Greeter greeter("Tests");
  for (auto _ : state) {  // NOLINT(clang-analyzer-deadcode.DeadStores)
    benchmark::DoNotOptimize(greeter.greet(greeter::LanguageCode::EN));
  }
}

// Register the function as a benchmark. main() comes from benchmark_main.
BENCHMARK(BM_greeter);
