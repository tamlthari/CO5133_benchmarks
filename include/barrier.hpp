#ifndef BARRIER_HPP
#define BARRIER_HPP

#include <atomic>
#include <thread>
#include <chrono>

#if defined(__aarch64__) || defined(__arm64__)
#define CPU_PAUSE() asm volatile("yield" ::: "memory")
#elif defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#define CPU_PAUSE() _mm_pause()
#else
#define CPU_PAUSE() std::this_thread::yield()
#endif

// 1. Sense-Reversing Barrier with Adaptive OS Yield
class sense_reversing_barrier {
    const unsigned int threshold;
    std::atomic<unsigned int> count;
    std::atomic<bool> sense;

public:
    explicit sense_reversing_barrier(unsigned int num_threads)
        : threshold(num_threads), count(num_threads), sense(false) {}

    void arrive_and_wait(bool& thread_sense) {
        thread_sense = !thread_sense;
        if (count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            count.store(threshold, std::memory_order_release);
            sense.store(thread_sense, std::memory_order_release);
        } else {
            int spins = 0;
            while (sense.load(std::memory_order_acquire) != thread_sense) {
                if (++spins < 64) {
                    CPU_PAUSE();
                } else {
                    std::this_thread::yield();
                }
            }
        }
    }
};

// 2. Sense-Reversing Barrier with Exponential Backoff and Adaptive Yield
class sense_reversing_barrier_backoff {
    const unsigned int threshold;
    std::atomic<unsigned int> count;
    std::atomic<bool> sense;

public:
    explicit sense_reversing_barrier_backoff(unsigned int num_threads)
        : threshold(num_threads), count(num_threads), sense(false) {}

    void arrive_and_wait(bool& thread_sense) {
        thread_sense = !thread_sense;
        if (count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            count.store(threshold, std::memory_order_release);
            sense.store(thread_sense, std::memory_order_release);
        } else {
            unsigned spins = 1;
            int yield_counter = 0;
            while (sense.load(std::memory_order_acquire) != thread_sense) {
                for (unsigned i = 0; i < spins; ++i) {
                    CPU_PAUSE();
                }
                if (spins < 512) spins *= 2;
                if (++yield_counter > 32) {
                    std::this_thread::yield();
                }
            }
        }
    }
};

#endif