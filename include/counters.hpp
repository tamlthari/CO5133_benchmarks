#ifndef COUNTERS_HPP
#define COUNTERS_HPP

#include <atomic>
#include <mutex>
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

// 1. Mutex-protected Counter
class counter_mutex {
    long long value{0};
    std::mutex mtx;
public:
    void increment() {
        std::lock_guard<std::mutex> lock(mtx);
        ++value;
    }
    long long get() {
        std::lock_guard<std::mutex> lock(mtx);
        return value;
    }
};

// 2. Compare-And-Swap (CAS) Counter
class counter_cas {
    std::atomic<long long> value{0};
public:
    void increment() {
        long long cur = value.load(std::memory_order_relaxed);
        while (!value.compare_exchange_weak(cur, cur + 1,
                                            std::memory_order_release,
                                            std::memory_order_relaxed)) {
            CPU_PAUSE();
        }
    }
    long long get() { return value.load(std::memory_order_seq_cst); }
};

// 3. CAS Counter with Exponential Backoff
class counter_cas_backoff {
    std::atomic<long long> value{0};
    static constexpr unsigned MIN_DELAY_NS = 20;
    static constexpr unsigned MAX_DELAY_NS = 8000;
public:
    void increment() {
        long long cur = value.load(std::memory_order_relaxed);
        unsigned delay = MIN_DELAY_NS;
        while (!value.compare_exchange_weak(cur, cur + 1,
                                            std::memory_order_release,
                                            std::memory_order_relaxed)) {
            std::this_thread::sleep_for(std::chrono::nanoseconds(delay));
            if (delay < MAX_DELAY_NS) delay *= 2;
        }
    }
    long long get() { return value.load(std::memory_order_seq_cst); }
};

// 4. Fetch-And-Add (FAA) Counter (Emits ARMv8.1-A LDADDAL on M2)
class counter_faa {
    std::atomic<long long> value{0};
public:
    void increment() {
        value.fetch_add(1, std::memory_order_relaxed);
    }
    long long get() { return value.load(std::memory_order_seq_cst); }
};

#endif