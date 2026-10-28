## On Apple Intel

Chip: Detected Windows Processor: Intel(R) Core(TM) i5-5257U CPU @ 2.70GHz
Intel_Core_i55257U_CPU__270GHz

### Install portable GCC on Windows

#### Download from https://winlibs.com/.

To the Release versions table. Locate the section for GCC 14.x.x (or GCC 13.x.x).

Look across the columns/rows in that version block to find the exact combination:

    Architecture: 64-bit (x86_64)

    C runtime: UCRT (Universal C Runtime)

    Thread technology: POSIX threads (needed for standard C++ <thread>, <mutex>, <barrier>)

    LLVM/Clang: Choose "without LLVM/Clang/LLD" for a smaller download, or "with LLVM/Clang/LLD" if you also want Clang on Windows.

In that specific row, click either:

    Zip archive (.zip): Download this if you want native Windows extraction without installing 3rd-party archivers.
#### Install and set up GCC
Once downloaded, extract the archive contents directly to a clean root path (e.g., C:\mingw64), ensuring that the compiler binaries reside in C:\mingw64\bin.

Add the compiler to your Windows environment variables:

    Press Win + R, type sysdm.cpl, and press Enter.

    Go to the Advanced tab and click Environment Variables....

    Under System variables (or User variables), select Path and click Edit....

    Click New, add C:\mingw64\bin, and click OK to close all dialogs.

Extract the Archive:
Extract the folder so the binary path is: C:\Users\<YourUsername>\mingw64\bin


Add to User Environment Variables (No Admin Rights Needed):

    Press Win + R, type sysdm.cpl, and press Enter.

    If Windows prompts for an administrator password, cancel that dialog and instead search the Start Menu for "Edit environment variables for your account" and press Enter.

    In the top section labeled User variables for <YourUsername>, select Path and click Edit... (or click New... if Path doesn't exist).

    Click New, paste:    C:\Users\<YourUsername>\mingw64\bin

### Install MPI on Windows
download and run msmpisetup.exe and msmpisdk.msi directly from the official Microsoft Download Center).This installs:The runtime launcher mpiexec.exe to C:\Program Files\Microsoft MPI\Bin\.   The SDK headers (mpi.h) to C:\Program Files (x86)\Microsoft SDKs\MPI\Include\.   The 64-bit library (msmpi.lib) to C:\Program Files (x86)\Microsoft SDKs\MPI\Lib\x64\.   Restart PowerShell after installation so the newly created environment paths ($env:MSMPI_INC and $env:MSMPI_LIB64) load into your session.

### Build executables and run sample commands
```powershell
# 1. Mutex Locks (Problem 1)
g++ -O3 -std=c++20 -Iinclude src/benchmark_locks.cpp -o bench_locks.exe

# 2. Barrier Synchronization (Problem 2)
g++ -O3 -std=c++20 -Iinclude src/benchmark_barrier.cpp -o bench_barrier.exe

# 3. Concurrent Counters (Problem 3)
g++ -O3 -std=c++20 -Iinclude src/benchmark_counters.cpp -o bench_counters.exe

g++ -O3 -std=c++20 -Iinclude -I"C:\Program Files (x86)\Microsoft SDKs\MPI\Include" src/mpi_distributed.cpp -o bench_mpi.exe -L"C:\Program Files (x86)\Microsoft SDKs\MPI\Lib\x64" -lmsmpi
```
#### Run sample commands
```powershell
# Test Locks (4 threads, 5000 iters, OW=0, N=5003)
.\bench_locks.exe 4 5000 0 5003

# Test Barriers (4 threads, 50000 episodes)
.\bench_barrier.exe 4 50000

# Test Counters (4 threads, 50000 increments)
.\bench_counters.exe 4 50000

# Run with 2 MPI processes
mpiexec.exe -n 2 .\bench_mpi.exe

# Run with 4 MPI processes
mpiexec.exe -n 4 .\bench_mpi.exe
```

### Run automated sweeps
```powershell
Set-ExecutionPolicy -Scope Process -Execu
tionPolicy Bypass
.\run_windows_benchmarks.ps1
```