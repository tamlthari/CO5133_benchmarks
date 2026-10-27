#!/usr/bin/env python3
import glob
import os
import matplotlib.pyplot as plt
import pandas as pd

plt.rcParams.update({
    "font.size": 11,
    "axes.labelsize": 12,
    "axes.titlesize": 13,
    "legend.fontsize": 10,
    "xtick.labelsize": 10,
    "ytick.labelsize": 10,
    "lines.linewidth": 2,
    "lines.markersize": 6
})

os.makedirs("plots", exist_ok=True)

def find_file(pattern, fallback):
    matches = glob.glob(pattern)
    if matches:
        return matches[0]
    if os.path.exists(fallback):
        return fallback
    return None

locks_file = find_file("*_locks.csv", "results_locks.csv")
barrier_file = find_file("*_barrier.csv", "results_barrier.csv")
counters_file = find_file("*_counters.csv", "results_counters.csv")
mpi_file = find_file("*_mpi.csv", "results_mpi.csv")

# 1. Plot Problem 1: Mutex Locks
if locks_file:
    df_locks = pd.read_csv(locks_file)
    for ow in sorted(df_locks["outside_work"].unique()):
        sub = df_locks[df_locks["outside_work"] == ow].sort_values("threads")
        plt.figure(figsize=(9, 5))
        plt.plot(sub["threads"], sub["tas_ms"], marker="o", label="TAS")
        plt.plot(sub["threads"], sub["ttas_ms"], marker="s", label="TTAS")
        plt.plot(sub["threads"], sub["ttas_backoff_ms"], marker="^", label="TTAS Backoff")
        plt.plot(sub["threads"], sub["mcs_ms"], marker="D", label="MCS")
        plt.plot(sub["threads"], sub["std_mutex_ms"], marker="x", linestyle="--", label="std::mutex")

        plt.axvline(x=4, color="gray", linestyle=":", label="P-Core Boundary (4)")
        plt.axvline(x=8, color="red", linestyle=":", label="Physical Cores (8)")

        plt.yscale("log")
        plt.xlabel("Thread Count")
        plt.ylabel("Execution Time (ms) [Log Scale]")
        plt.title(f"Mutex Locks Scalability (OUTSIDE_WORK = {ow})")
        plt.grid(True, which="both", linestyle="--", alpha=0.5)
        plt.legend()
        plt.tight_layout()
        plt.savefig(f"plots/locks_ow_{ow}.png", dpi=300)
        plt.close()
    print(f"[+] Plotted Locks from {locks_file}")

# 2. Plot Problem 2: Barrier Throughput
if barrier_file:
    df_bar = pd.read_csv(barrier_file).sort_values("threads")
    plt.figure(figsize=(9, 5))
    plt.plot(df_bar["threads"], df_bar["sense_barrier_episodes_sec"], marker="o", label="Sense Barrier")
    if "sense_backoff_episodes_sec" in df_bar.columns:
        plt.plot(df_bar["threads"], df_bar["sense_backoff_episodes_sec"], marker="^", label="Sense + Backoff")
    plt.plot(df_bar["threads"], df_bar["std_barrier_episodes_sec"], marker="s", linestyle="--", label="std::barrier (C++20)")

    plt.axvline(x=4, color="gray", linestyle=":", label="P-Core Boundary (4)")
    plt.axvline(x=8, color="red", linestyle=":", label="Physical Cores (8)")

    plt.yscale("log")
    plt.xlabel("Thread Count")
    plt.ylabel("Episodes / Second [Log Scale]")
    plt.title("Barrier Synchronization Throughput")
    plt.grid(True, which="both", linestyle="--", alpha=0.5)
    plt.legend()
    plt.tight_layout()
    plt.savefig("plots/barrier_scaling.png", dpi=300)
    plt.close()
    print(f"[+] Plotted Barriers from {barrier_file}")

# 3. Plot Problem 3: Concurrent Counters
if counters_file:
    df_cnt = pd.read_csv(counters_file).sort_values("threads")
    plt.figure(figsize=(9, 5))
    plt.plot(df_cnt["threads"], df_cnt["mutex_ms"], marker="o", label="Mutex Counter")
    plt.plot(df_cnt["threads"], df_cnt["cas_ms"], marker="s", label="CAS Counter")
    plt.plot(df_cnt["threads"], df_cnt["cas_backoff_ms"], marker="^", label="CAS Backoff Counter")
    plt.plot(df_cnt["threads"], df_cnt["faa_ms"], marker="D", label="FAA (ARMv8.1 LDADDAL)")

    plt.axvline(x=4, color="gray", linestyle=":", label="P-Core Boundary (4)")
    plt.axvline(x=8, color="red", linestyle=":", label="Physical Cores (8)")

    plt.xlabel("Thread Count")
    plt.ylabel("Execution Time (ms)")
    plt.title("Concurrent Counters Execution Time")
    plt.grid(True, linestyle="--", alpha=0.5)
    plt.legend()
    plt.tight_layout()
    plt.savefig("plots/counters_scaling.png", dpi=300)
    plt.close()
    print(f"[+] Plotted Counters from {counters_file}")

# 4. Plot Problem 4: Distributed MPI RMA
if mpi_file:
    df_mpi = pd.read_csv(mpi_file).sort_values("processes")
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))

    # Subplot A: Latency of Distributed Counter & Distributed TAS Lock
    ax1.plot(df_mpi["processes"], df_mpi["counter_ms"], marker="o", color="#1f77b4", label="MPI Counter (Fetch_and_op)")
    ax1.plot(df_mpi["processes"], df_mpi["lock_ms"], marker="s", color="#d62728", label="MPI TAS Lock (CAS)")
    ax1.set_xlabel("MPI Processes (np)")
    ax1.set_ylabel("Execution Time (ms)")
    ax1.set_title("MPI-3 RMA Counter & Lock Latency")
    ax1.grid(True, linestyle="--", alpha=0.5)
    ax1.legend()

    # Subplot B: Distributed Barrier Throughput
    ax2.plot(df_mpi["processes"], df_mpi["barrier_episodes_sec"], marker="^", color="#2ca02c", label="MPI Sense Barrier")
    ax2.set_yscale("log")
    ax2.set_xlabel("MPI Processes (np)")
    ax2.set_ylabel("Episodes / Second [Log Scale]")
    ax2.set_title("MPI-3 RMA Barrier Throughput")
    ax2.grid(True, which="both", linestyle="--", alpha=0.5)
    ax2.legend()

    plt.tight_layout()
    plt.savefig("plots/mpi_scaling.png", dpi=300)
    plt.close()
    print(f"[+] Plotted Distributed MPI from {mpi_file}")
else:
    print("[!] No MPI CSV found. Run `run_platform_benchmarks.sh` to generate results_mpi.csv.")