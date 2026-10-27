#!/bin/zsh

# 1. Compile all benchmarks from root
clang++ -O3 -std=c++20 -Iinclude src/benchmark_locks.cpp -o bench_locks
clang++ -O3 -std=c++20 -Iinclude src/benchmark_barrier.cpp -o bench_barrier
clang++ -O3 -std=c++20 -Iinclude src/benchmark_counters.cpp -o bench_counters
mpicxx -O3 -std=c++20 -Iinclude src/mpi_distributed.cpp -o bench_mpi

# 2. Benchmark Locks (Problem 1)
echo "threads,outside_work,tas_ms,ttas_ms,ttas_backoff_ms,mcs_ms,std_mutex_ms" > results_locks.csv
echo "=== Running Mutex Sweeps ==="
for ow in 0 1 4; do
    for t in 1 2 4 6 8 12 16; do
        echo "Running Locks: Threads=$t | OUTSIDE_WORK=$ow"
        out=$(./bench_locks $t 5000 $ow 5003)
        tas=$(echo "$out" | grep -E "^TAS:" | awk '{print $2}')
        ttas=$(echo "$out" | grep -E "^TTAS:" | awk '{print $2}')
        ttas_bo=$(echo "$out" | grep -E "^TTAS Backoff:" | awk '{print $3}')
        mcs=$(echo "$out" | grep -E "^MCS:" | awk '{print $2}')
        std_mtx=$(echo "$out" | grep -E "^std::mutex:" | awk '{print $2}')
        echo "$t,$ow,$tas,$ttas,$ttas_bo,$mcs,$std_mtx" >> results_locks.csv
    done
done

# 3. Benchmark Barriers (Problem 2)
echo "threads,sense_barrier_episodes_sec,sense_backoff_episodes_sec,std_barrier_episodes_sec" > results_barrier.csv
echo "=== Running Barrier Sweeps ==="
for t in 1 2 4 6 8 12 16; do
    echo "Running Barrier: Threads=$t"
    out=$(./bench_barrier $t 50000)
    sense=$(echo "$out" | grep -E "^Sense-Reversing Barrier:" | awk '{print $3}')
    sense_bo=$(echo "$out" | grep -E "^Sense-Reversing Barrier Backoff:" | awk '{print $4}')
    std_b=$(echo "$out" | grep -E "^std::barrier" | awk '{print $3}')
    echo "$t,$sense,$sense_bo,$std_b" >> results_barrier.csv
done

# 4. Benchmark Counters (Problem 3)
echo "threads,mutex_ms,cas_ms,cas_backoff_ms,faa_ms" > results_counters.csv
echo "=== Running Counter Sweeps ==="
for t in 1 2 4 6 8 12 16; do
    echo "Running Counters: Threads=$t"
    out=$(./bench_counters $t 200000)
    mtx=$(echo "$out" | grep -E "^Mutex Counter:" | awk '{print $3}')
    cas=$(echo "$out" | grep -E "^CAS Counter:" | awk '{print $3}')
    cas_bo=$(echo "$out" | grep -E "^CAS Backoff Counter:" | awk '{print $4}')
    faa=$(echo "$out" | grep -E "^FAA Counter:" | awk '{print $3}')
    echo "$t,$mtx,$cas,$cas_bo,$faa" >> results_counters.csv
done

# 5. Distributed MPI Benchmarks (Problem 4)
echo "=== Running Distributed MPI Sweeps ==="
for p in 1 2 4 8; do
    echo "--- MPI World Size: $p ---"
    mpirun -np $p ./bench_mpi
done
echo "--- MPI Oversubscribed: 16 ---"
mpirun --oversubscribe -np 16 ./bench_mpi

echo "All sweeps complete. Clean CSV files ready for plotting."