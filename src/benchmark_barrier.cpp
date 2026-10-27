#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <cassert>
#include <barrier>
#include "barrier.hpp"

using namespace std::chrono;

void verify_barrier_correctness(int num_threads, int num_episodes) {
    sense_reversing_barrier barrier(num_threads);
    std::vector<int> step_tracker(num_threads, 0);
    std::vector<std::thread> threads;

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            bool local_sense = false;
            for (int ep = 1; ep <= num_episodes; ++ep) {
                step_tracker[t] = ep;
                barrier.arrive_and_wait(local_sense);

                for (int i = 0; i < num_threads; ++i) {
                    assert(step_tracker[i] >= ep && "Thread escaped barrier prematurely!");
                }
                barrier.arrive_and_wait(local_sense);
            }
        });
    }
    for (auto& th : threads) th.join();
}

template <typename BarrierType>
double benchmark_custom_barrier(int num_threads, int num_episodes) {
    BarrierType barrier(num_threads);
    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    auto start = high_resolution_clock::now();
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&]() {
            bool local_sense = false;
            for (int ep = 0; ep < num_episodes; ++ep) {
                barrier.arrive_and_wait(local_sense);
            }
        });
    }
    for (auto& th : threads) th.join();
    auto end = high_resolution_clock::now();

    double elapsed_sec = duration<double>(end - start).count();
    return (double)num_episodes / elapsed_sec;
}

double benchmark_std_barrier(int num_threads, int num_episodes) {
    std::barrier barrier(num_threads);
    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    auto start = high_resolution_clock::now();
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&]() {
            for (int ep = 0; ep < num_episodes; ++ep) {
                barrier.arrive_and_wait();
            }
        });
    }
    for (auto& th : threads) th.join();
    auto end = high_resolution_clock::now();

    double elapsed_sec = duration<double>(end - start).count();
    return (double)num_episodes / elapsed_sec;
}

int main(int argc, char* argv[]) {
    int num_threads = (argc > 1) ? std::stoi(argv[1]) : 4;
    
    // Automatically throttle episodes when oversubscribing physical cores
    int default_episodes = (num_threads > 8) ? 1000 : 50000;
    int num_episodes = (argc > 2) ? std::stoi(argv[2]) : default_episodes;

    std::cout << "--> Running Barrier Correctness Verification..." << std::endl;
    verify_barrier_correctness(num_threads, 50);
    std::cout << "Barrier Verification Passed!\n" << std::endl;

    std::cout << "=== Barrier Scalability (Episodes per Second) ===" << std::endl;
    std::cout << "Threads: " << num_threads << " | Episodes: " << num_episodes << "\n" << std::endl;

    double rate_sense = benchmark_custom_barrier<sense_reversing_barrier>(num_threads, num_episodes);
    std::cout << "Sense-Reversing Barrier:         " << rate_sense << " ep/s" << std::endl;

    double rate_backoff = benchmark_custom_barrier<sense_reversing_barrier_backoff>(num_threads, num_episodes);
    std::cout << "Sense-Reversing Barrier Backoff: " << rate_backoff << " ep/s" << std::endl;

    double rate_std = benchmark_std_barrier(num_threads, num_episodes);
    std::cout << "std::barrier (C++20):            " << rate_std << " ep/s" << std::endl;

    return 0;
}