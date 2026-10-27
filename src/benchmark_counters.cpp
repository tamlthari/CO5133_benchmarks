#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <cassert>
#include "counters.hpp"

using namespace std::chrono;

template <typename CounterType>
void verify_counter_correctness(int num_threads, int iters_per_thread) {
    CounterType counter;
    std::vector<std::thread> threads;
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&]() {
            for (int i = 0; i < iters_per_thread; ++i) {
                counter.increment();
            }
        });
    }
    for (auto& th : threads) th.join();
    assert(counter.get() == (long long)num_threads * iters_per_thread && "Counter lost atomic updates!");
}

template <typename CounterType>
double benchmark_counter(int num_threads, int iters_per_thread) {
    CounterType counter;
    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    auto start = high_resolution_clock::now();
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&]() {
            for (int i = 0; i < iters_per_thread; ++i) {
                counter.increment();
            }
        });
    }
    for (auto& th : threads) th.join();
    auto end = high_resolution_clock::now();

    return duration<double, std::milli>(end - start).count();
}

int main(int argc, char* argv[]) {
    int num_threads = (argc > 1) ? std::stoi(argv[1]) : 8;
    int iters_per_thread = (argc > 2) ? std::stoi(argv[2]) : 200000;

    std::cout << "--> Running Counter Correctness Verification..." << std::endl;
    verify_counter_correctness<counter_mutex>(num_threads, 50000);
    verify_counter_correctness<counter_cas>(num_threads, 50000);
    verify_counter_correctness<counter_cas_backoff>(num_threads, 50000);
    verify_counter_correctness<counter_faa>(num_threads, 50000);
    std::cout << "All Counter Tests Passed!\n" << std::endl;

    std::cout << "=== Counter Execution Time Benchmark ===" << std::endl;
    std::cout << "Mutex Counter:       " << benchmark_counter<counter_mutex>(num_threads, iters_per_thread) << " ms" << std::endl;
    std::cout << "CAS Counter:         " << benchmark_counter<counter_cas>(num_threads, iters_per_thread) << " ms" << std::endl;
    std::cout << "CAS Backoff Counter: " << benchmark_counter<counter_cas_backoff>(num_threads, iters_per_thread) << " ms" << std::endl;
    std::cout << "FAA Counter:         " << benchmark_counter<counter_faa>(num_threads, iters_per_thread) << " ms" << std::endl;

    return 0;
}