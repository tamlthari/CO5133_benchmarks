## On Apple M2

Chip: Apple M2
Total Number of Cores: 8 (4 Performance and 4 Efficiency)

```bash
# 1. Compile Shared-Memory Benchmarks
clang++ -O3 -std=c++20 -Iinclude src/benchmark_locks.cpp -o bench_locks
clang++ -O3 -std=c++20 -Iinclude src/benchmark_barrier.cpp -o bench_barrier
clang++ -O3 -std=c++20 -Iinclude src/benchmark_counters.cpp -o bench_counters


# 2. Setup and Compile MPI (Problem 4)
brew install open-mpi
mpicxx -O3 -std=c++20 -Iinclude src/mpi_distributed.cpp -o bench_mpi
```