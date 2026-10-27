## On Apple M2

Chip: Apple M2
Total Number of Cores: 8 (4 Performance and 4 Efficiency)

### Build executables and run sample commands
```bash
# 1. Compile Shared-Memory Benchmarks
clang++ -O3 -std=c++20 -Iinclude src/benchmark_locks.cpp -o bench_locks
clang++ -O3 -std=c++20 -Iinclude src/benchmark_barrier.cpp -o bench_barrier
clang++ -O3 -std=c++20 -Iinclude src/benchmark_counters.cpp -o bench_counters


# 2. Setup and Compile MPI (Problem 4)
brew install open-mpi
mpicxx -O3 -std=c++20 -Iinclude src/mpi_distributed.cpp -o bench_mpi
```
#### Run sample commands
```bash
./bench_barrier 4 50000
./bench_barrier 4 200000

./bench_counters 4 50000
./bench_counters 8 200000

mpirun -np 2 ./bench_mpi
mpirun -np 4 ./bench_mpi
mpirun -np 8 ./bench_mpi
mpirun --oversubscribe -np 16 ./bench_mpi
```

### Run automated sweeps
```bash
chmod +x run_platform_benchmarks.sh
./run_platform_benchmarks.sh
```