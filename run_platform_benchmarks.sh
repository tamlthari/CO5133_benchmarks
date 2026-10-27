#!/bin/zsh

# Auto-detect chip architecture
ARCH=$(uname -m)
if [[ "$ARCH" == "arm64" ]]; then
    CHIP_NAME=$(sysctl -n machdep.cpu.brand_string 2>/dev/null || echo "Apple_Silicon")
else
    CHIP_NAME="Apple_Intel_x86"
fi

# Clean label for filenames (e.g., M1, M2, Intel)
LABEL=$(echo "$CHIP_NAME" | tr ' ' '_' | tr -cd '[:alnum:]_')
echo "=== Detected Platform: $CHIP_NAME (Prefix: $LABEL) ==="

# Compile with optimization
clang++ -O3 -std=c++20 -Iinclude src/benchmark_locks.cpp -o bench_locks
clang++ -O3 -std=c++20 -Iinclude src/benchmark_barrier.cpp -o bench_barrier
clang++ -O3 -std=c++20 -Iinclude src/benchmark_counters.cpp -o bench_counters
mpicxx -O3 -std=c++20 -Iinclude src/mpi_distributed.cpp -o bench_mpi

# 1. Mutex Locks
echo "threads,outside_work,tas_ms,ttas_ms,ttas_backoff_ms,mcs_ms,std_mutex_ms" > "${LABEL}_locks.csv"
for ow in 0 1 4; do
    for t in 1 2 4 6 8 12 16; do
        echo "[$LABEL Locks] Threads=$t | OW=$ow"
        out=$(./bench_locks $t 5000 $ow 5003)
        tas=$(echo "$out" | grep -E "^TAS:" | awk '{print $2}')
        ttas=$(echo "$out" | grep -E "^TTAS:" | awk '{print $2}')
        ttas_bo=$(echo "$out" | grep -E "^TTAS Backoff:" | awk '{print $3}')
        mcs=$(echo "$out" | grep -E "^MCS:" | awk '{print $2}')
        std_mtx=$(echo "$out" | grep -E "^std::mutex:" | awk '{print $2}')
        echo "$t,$ow,$tas,$ttas,$ttas_bo,$mcs,$std_mtx" >> "${LABEL}_locks.csv"
    done
done

# 2. Barriers
echo "threads,sense_barrier_episodes_sec,sense_backoff_episodes_sec,std_barrier_episodes_sec" > "${LABEL}_barrier.csv"
for t in 1 2 4 6 8 12 16; do
    echo "[$LABEL Barrier] Threads=$t"
    out=$(./bench_barrier $t 50000)
    sense=$(echo "$out" | grep -E "^Sense-Reversing Barrier:" | awk '{print $3}')
    sense_bo=$(echo "$out" | grep -E "^Sense-Reversing Barrier Backoff:" | awk '{print $4}')
    std_b=$(echo "$out" | grep -E "^std::barrier" | awk '{print $3}')
    echo "$t,$sense,$sense_bo,$std_b" >> "${LABEL}_barrier.csv"
done

# 3. Counters
echo "threads,mutex_ms,cas_ms,cas_backoff_ms,faa_ms" > "${LABEL}_counters.csv"
for t in 1 2 4 6 8 12 16; do
    echo "[$LABEL Counters] Threads=$t"
    out=$(./bench_counters $t 200000)
    mtx=$(echo "$out" | grep -E "^Mutex Counter:" | awk '{print $3}')
    cas=$(echo "$out" | grep -E "^CAS Counter:" | awk '{print $3}')
    cas_bo=$(echo "$out" | grep -E "^CAS Backoff Counter:" | awk '{print $4}')
    faa=$(echo "$out" | grep -E "^FAA Counter:" | awk '{print $3}')
    echo "$t,$mtx,$cas,$cas_bo,$faa" >> "${LABEL}_counters.csv"
done

# 4. MPI
echo "processes,counter_ms,lock_ms,barrier_episodes_sec" > "${LABEL}_mpi.csv"
for p in 1 2 4 8; do
    echo "[$LABEL MPI] np=$p"
    out=$(mpirun -np $p ./bench_mpi)
    cnt=$(echo "$out" | grep "\[MPI Counter\]" | awk '{print $(NF-1)}')
    lck=$(echo "$out" | grep "\[MPI Lock\]" | awk '{print $(NF-1)}')
    bar=$(echo "$out" | grep "\[MPI Barrier\]" | awk '{print $3}')
    echo "$p,$cnt,$lck,$bar" >> "${LABEL}_mpi.csv"
done
out=$(mpirun --oversubscribe -np 16 ./bench_mpi)
cnt=$(echo "$out" | grep "\[MPI Counter\]" | awk '{print $(NF-1)}')
lck=$(echo "$out" | grep "\[MPI Lock\]" | awk '{print $(NF-1)}')
bar=$(echo "$out" | grep "\[MPI Barrier\]" | awk '{print $3}')
echo "16,$cnt,$lck,$bar" >> "${LABEL}_mpi.csv"

echo "Platform run completed: generated ${LABEL}_*.csv files."