#include <mpi.h>
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>

// 1. Distributed Counter using MPI-3 RMA Fetch-and-Add
class DistributedCounter {
    MPI_Win win;
    int rank;
    long long counter_storage = 0;

public:
    DistributedCounter(int rank) : rank(rank) {
        if (rank == 0) {
            MPI_Win_create(&counter_storage, sizeof(long long), sizeof(long long), MPI_INFO_NULL, MPI_COMM_WORLD, &win);
        } else {
            MPI_Win_create(nullptr, 0, sizeof(long long), MPI_INFO_NULL, MPI_COMM_WORLD, &win);
        }
    }

    ~DistributedCounter() {
        MPI_Win_free(&win);
    }

    void increment() {
        long long incr = 1;
        long long res = 0;
        MPI_Win_lock(MPI_LOCK_SHARED, 0, 0, win);
        MPI_Fetch_and_op(&incr, &res, MPI_LONG_LONG, 0, 0, MPI_SUM, win);
        MPI_Win_unlock(0, win);
    }

    long long get() {
        long long res = 0;
        MPI_Win_lock(MPI_LOCK_SHARED, 0, 0, win);
        MPI_Get(&res, 1, MPI_LONG_LONG, 0, 0, 1, MPI_LONG_LONG, win);
        MPI_Win_unlock(0, win);
        return res;
    }
};

// 2. Distributed TAS Spinlock using MPI_Compare_and_swap
class DistributedTASLock {
    MPI_Win win;
    int rank;
    int lock_storage = 0; // 0: unlocked, 1: locked

public:
    DistributedTASLock(int rank) : rank(rank) {
        if (rank == 0) {
            MPI_Win_create(&lock_storage, sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &win);
        } else {
            MPI_Win_create(nullptr, 0, sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &win);
        }
    }

    ~DistributedTASLock() {
        MPI_Win_free(&win);
    }

    void acquire() {
        int desired = 1;
        int expected = 0;
        int result = 1;

        while (result != 0) {
            MPI_Win_lock(MPI_LOCK_SHARED, 0, 0, win);
            MPI_Compare_and_swap(&desired, &expected, &result, MPI_INT, 0, 0, win);
            MPI_Win_unlock(0, win);

            if (result != 0) {
                std::this_thread::sleep_for(std::chrono::microseconds(25));
            }
        }
    }

    void release() {
        int zero = 0;
        MPI_Win_lock(MPI_LOCK_SHARED, 0, 0, win);
        MPI_Accumulate(&zero, 1, MPI_INT, 0, 0, 1, MPI_INT, MPI_REPLACE, win);
        MPI_Win_unlock(0, win);
    }
};

// 3. Distributed Sense-Reversing Barrier via RMA
class DistributedBarrier {
    MPI_Win win;
    int rank;
    int world_size;
    struct State {
        int count;
        int sense;
    } state;

public:
    DistributedBarrier(int rank, int world_size) : rank(rank), world_size(world_size) {
        if (rank == 0) {
            state.count = world_size;
            state.sense = 0;
            MPI_Win_create(&state, sizeof(State), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &win);
        } else {
            MPI_Win_create(nullptr, 0, sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &win);
        }
    }

    ~DistributedBarrier() {
        MPI_Win_free(&win);
    }

    void arrive_and_wait(int& local_sense) {
        local_sense = !local_sense;
        int decrement = -1;
        int prior = 0;

        MPI_Win_lock(MPI_LOCK_SHARED, 0, 0, win);
        MPI_Fetch_and_op(&decrement, &prior, MPI_INT, 0, 0, MPI_SUM, win);
        MPI_Win_unlock(0, win);

        if (prior == 1) {
            int reset = world_size;
            MPI_Win_lock(MPI_LOCK_SHARED, 0, 0, win);
            MPI_Accumulate(&reset, 1, MPI_INT, 0, 0, 1, MPI_INT, MPI_REPLACE, win);
            MPI_Accumulate(&local_sense, 1, MPI_INT, 0, 1, 1, MPI_INT, MPI_REPLACE, win);
            MPI_Win_unlock(0, win);
        } else {
            int observed_sense = !local_sense;
            while (observed_sense != local_sense) {
                MPI_Win_lock(MPI_LOCK_SHARED, 0, 0, win);
                MPI_Get(&observed_sense, 1, MPI_INT, 0, 1, 1, MPI_INT, win);
                MPI_Win_unlock(0, win);
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        }
    }
};

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Benchmark Distributed Counter
    {
        DistributedCounter counter(rank);
        MPI_Barrier(MPI_COMM_WORLD);
        double t0 = MPI_Wtime();
        const int iters = 2000;
        for (int i = 0; i < iters; ++i) counter.increment();
        MPI_Barrier(MPI_COMM_WORLD);
        double t1 = MPI_Wtime();

        if (rank == 0) {
            std::cout << "[MPI Counter] Value: " << counter.get() 
                      << " (Expected: " << size * iters << ") in " 
                      << (t1 - t0) * 1000.0 << " ms" << std::endl;
        }
    }

    // Benchmark Distributed Lock
    {
        DistributedTASLock lock(rank);
        MPI_Barrier(MPI_COMM_WORLD);
        double t0 = MPI_Wtime();
        const int iters = 200;
        for (int i = 0; i < iters; ++i) {
            lock.acquire();
            lock.release();
        }
        MPI_Barrier(MPI_COMM_WORLD);
        double t1 = MPI_Wtime();

        if (rank == 0) {
            std::cout << "[MPI Lock] Completed in " << (t1 - t0) * 1000.0 << " ms" << std::endl;
        }
    }

    // Benchmark Distributed Barrier
    {
        DistributedBarrier barrier(rank, size);
        int local_sense = 0;
        MPI_Barrier(MPI_COMM_WORLD);
        double t0 = MPI_Wtime();
        const int episodes = 500;
        for (int i = 0; i < episodes; ++i) {
            barrier.arrive_and_wait(local_sense);
        }
        double t1 = MPI_Wtime();

        if (rank == 0) {
            std::cout << "[MPI Barrier] " << episodes / (t1 - t0) << " episodes/sec" << std::endl;
        }
    }

    MPI_Finalize();
    return 0;
}