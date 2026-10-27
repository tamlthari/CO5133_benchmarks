#ifndef LOCKS_HPP
#define LOCKS_HPP

#include <atomic>
#include <thread>
#include <chrono>

// Hardware spin-pause calibrated for Apple Silicon M2 ARM64
#if defined(__aarch64__) || defined(__arm64__)
#define CPU_PAUSE() asm volatile("yield" ::: "memory")
#elif defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#define CPU_PAUSE() _mm_pause()
#else
#define CPU_PAUSE() std::this_thread::yield()
#endif

// 1. Test-And-Set (TAS) Spinlock
class lock_tas {
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
public:
    void acquire() {
        while (flag.test_and_set(std::memory_order_acquire)) {
            CPU_PAUSE();
        }
    }
    void release() {
        flag.clear(std::memory_order_release);
    }
};

// 2. Test-and-Test-And-Set (TTAS) Spinlock
class lock_ttas {
    std::atomic<bool> flag{false};
public:
    void acquire() {
        while (true) {
            while (flag.load(std::memory_order_relaxed)) {
                CPU_PAUSE();
            }
            if (!flag.exchange(true, std::memory_order_acquire)) {
                return;
            }
        }
    }
    void release() {
        flag.store(false, std::memory_order_release);
    }
};

// 3. TTAS Spinlock with Exponential Backoff
class lock_ttas_backoff {
    std::atomic<bool> flag{false};
    static constexpr unsigned MIN_DELAY_NS = 20;
    static constexpr unsigned MAX_DELAY_NS = 16000;
public:
    void acquire() {
        unsigned delay = MIN_DELAY_NS;
        while (true) {
            while (flag.load(std::memory_order_relaxed)) {
                CPU_PAUSE();
            }
            if (!flag.exchange(true, std::memory_order_acquire)) {
                return;
            }
            std::this_thread::sleep_for(std::chrono::nanoseconds(delay));
            if (delay < MAX_DELAY_NS) {
                delay *= 2;
            }
        }
    }
    void release() {
        flag.store(false, std::memory_order_release);
    }
};

// 4. Mellor-Crummey Scott (MCS) Queue Lock
struct mcs_qnode {
    std::atomic<mcs_qnode*> next;
    std::atomic<bool> locked;
    mcs_qnode() : next(nullptr), locked(false) {}
};

class lock_mcs {
private:
    std::atomic<mcs_qnode*> tail{nullptr};
    static inline thread_local mcs_qnode node;

public:
    void acquire() {
        node.next.store(nullptr, std::memory_order_relaxed);
        node.locked.store(true, std::memory_order_relaxed);

        mcs_qnode* prev = tail.exchange(&node, std::memory_order_acq_rel);
        if (prev != nullptr) {
            prev->next.store(&node, std::memory_order_release);
            while (node.locked.load(std::memory_order_acquire)) {
                CPU_PAUSE();
            }
        }
    }

    void release() {
        mcs_qnode* succ = node.next.load(std::memory_order_acquire);
        if (succ == nullptr) {
            mcs_qnode* expected = &node;
            if (tail.compare_exchange_strong(expected, nullptr, std::memory_order_acq_rel)) {
                return;
            }
            while ((succ = node.next.load(std::memory_order_acquire)) == nullptr) {
                CPU_PAUSE();
            }
        }
        succ->locked.store(false, std::memory_order_release);
    }
};

#endif