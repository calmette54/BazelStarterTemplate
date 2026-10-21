#include <benchmark/benchmark.h>

#include "lib/greeter/include/greeter.h"

static const greeter::Greeter kGreeter("Tests");

static void BM_greeter(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(kGreeter.greet(greeter::LanguageCode::EN));
  }
}

// Register the function as a benchmark. main() comes from benchmark_main.
BENCHMARK(BM_greeter);
