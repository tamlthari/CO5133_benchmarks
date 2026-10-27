#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <mutex>
#include <cassert>
#include "locks.hpp"

using namespace std::chrono;

// Assignment-specified primality test workload
int is_prime(int p) {
    for (int d = 2; d < p; ++d) {
        if (p % d == 0) return 0;
    }
    return 1;
}

// Correctness verification: Invariant validation under heavy concurrency
template <typename LockType>
void verify_lock_correctness(int num_threads, int iters_per_thread) {
    LockType lock;
    long long shared_counter = 0;
    long long inv_a = 0;
    long long inv_b = 0;
    std::vector<std::thread> threads;

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&]() {
            for (int i = 0; i < iters_per_thread; ++i) {
                lock.acquire();
                ++shared_counter;
                ++inv_a;
                ++inv_b;
                // If mutual exclusion is violated, inv_a != inv_b triggers an assertion
                assert(inv_a == inv_b && "Mutual exclusion invariant broken!");
                lock.release();
            }
        });
    }
    for (auto& th : threads) th.join();
    assert(shared_counter == (long long)num_threads * iters_per_thread);
}

// Assignment-specified thread entry benchmark
template <typename LockType>
double benchmark_lock(int num_threads, int num_iters, int outside_work, int prime_n) {
    LockType lock;
    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    auto start = high_resolution_clock::now();
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&]() {
            for (auto i = 0; i < num_iters; ++i) {
                lock.acquire();
                is_prime(prime_n);
                lock.release();

                for (auto j = 0; j < outside_work; ++j) {
                    is_prime(prime_n);
                }
            }
        });
    }
    for (auto& th : threads) th.join();
    auto end = high_resolution_clock::now();

    return duration<double, std::milli>(end - start).count();
}

struct std_mutex_wrapper {
    std::mutex mtx;
    void acquire() { mtx.lock(); }
    void release() { mtx.unlock(); }
};

int main(int argc, char* argv[]) {
    int num_threads = (argc > 1) ? std::stoi(argv[1]) : 8;
    int num_iters = (argc > 2) ? std::stoi(argv[2]) : 5000;
    int outside_work = (argc > 3) ? std::stoi(argv[3]) : 0;
    int prime_n = (argc > 4) ? std::stoi(argv[4]) : 5003;

    std::cout << "--> Running Lock Correctness Assertions (" << num_threads << " threads)..." << std::endl;
    verify_lock_correctness<lock_tas>(num_threads, 5000);
    verify_lock_correctness<lock_ttas>(num_threads, 5000);
    verify_lock_correctness<lock_ttas_backoff>(num_threads, 5000);
    verify_lock_correctness<lock_mcs>(num_threads, 5000);
    verify_lock_correctness<std_mutex_wrapper>(num_threads, 5000);
    std::cout << "All Correctness Checks Passed!\n" << std::endl;

    std::cout << "=== Lock Benchmark (Threads: " << num_threads << ", OUTSIDE_WORK: " << outside_work << ") ===" << std::endl;
    std::cout << "TAS:          " << benchmark_lock<lock_tas>(num_threads, num_iters, outside_work, prime_n) << " ms" << std::endl;
    std::cout << "TTAS:         " << benchmark_lock<lock_ttas>(num_threads, num_iters, outside_work, prime_n) << " ms" << std::endl;
    std::cout << "TTAS Backoff: " << benchmark_lock<lock_ttas_backoff>(num_threads, num_iters, outside_work, prime_n) << " ms" << std::endl;
    std::cout << "MCS:          " << benchmark_lock<lock_mcs>(num_threads, num_iters, outside_work, prime_n) << " ms" << std::endl;
    std::cout << "std::mutex:   " << benchmark_lock<std_mutex_wrapper>(num_threads, num_iters, outside_work, prime_n) << " ms" << std::endl;

    return 0;
}