# run_windows_benchmarks.ps1
$ErrorActionPreference = "Continue"

# Detect Windows CPU name to automatically prefix output CSV files
$RAW_CPU = (Get-CimInstance Win32_Processor).Name.Trim()
$CPU_LABEL = ($RAW_CPU -replace '\(R\)','' -replace '\(TM\)','' -replace '\s+','_' -replace '[^a-zA-Z0-9_]','').Trim('_')
Write-Host "=== Detected Windows Processor: $RAW_CPU ===" -ForegroundColor Cyan
Write-Host "=== File Prefix: $CPU_LABEL ===" -ForegroundColor Cyan

# 0. Compile all 4 problems
Write-Host ""
Write-Host "=== Compiling Benchmarks with g++ ===" -ForegroundColor Yellow
g++ -O3 -std=c++20 -Iinclude src/benchmark_locks.cpp -o bench_locks.exe
g++ -O3 -std=c++20 -Iinclude src/benchmark_barrier.cpp -o bench_barrier.exe
g++ -O3 -std=c++20 -Iinclude src/benchmark_counters.cpp -o bench_counters.exe
g++ -O3 -std=c++20 -Iinclude -I"C:\Program Files (x86)\Microsoft SDKs\MPI\Include" src/mpi_distributed.cpp -o bench_mpi.exe -L"C:\Program Files (x86)\Microsoft SDKs\MPI\Lib\x64" -lmsmpi

# 1. Problem 1: Mutex Locks Sweep
$locks_file = "${CPU_LABEL}_locks.csv"
"threads,outside_work,tas_ms,ttas_ms,ttas_backoff_ms,mcs_ms,std_mutex_ms" | Out-File -FilePath $locks_file -Encoding utf8
Write-Host ""
Write-Host "=== 1. Running Mutex Locks Benchmarks ===" -ForegroundColor Green
foreach ($ow in 0, 1, 4) {
    foreach ($t in 1, 2, 4, 6, 8, 12, 16) {
        Write-Host "[$CPU_LABEL Locks] Threads=$t | OUTSIDE_WORK=$ow"
        $out = .\bench_locks.exe $t 5000 $ow 5003
        
        $tas    = (($out | Select-String "^TAS:").ToString() -split '\s+')[1]
        $ttas   = (($out | Select-String "^TTAS:").ToString() -split '\s+')[1]
        $ttas_bo= (($out | Select-String "^TTAS Backoff:").ToString() -split '\s+')[2]
        $mcs    = (($out | Select-String "^MCS:").ToString() -split '\s+')[1]
        $std_mtx= (($out | Select-String "^std::mutex:").ToString() -split '\s+')[1]

        "$t,$ow,$tas,$ttas,$ttas_bo,$mcs,$std_mtx" | Out-File -FilePath $locks_file -Append -Encoding utf8
    }
}

# 2. Problem 2: Barrier Synchronization Sweep
$barrier_file = "${CPU_LABEL}_barrier.csv"
"threads,sense_barrier_episodes_sec,sense_backoff_episodes_sec,std_barrier_episodes_sec" | Out-File -FilePath $barrier_file -Encoding utf8
Write-Host ""
Write-Host "=== 2. Running Barrier Benchmarks ===" -ForegroundColor Green
foreach ($t in 1, 2, 4, 6, 8, 12, 16) {
    Write-Host "[$CPU_LABEL Barrier] Threads=$t"
    $out = .\bench_barrier.exe $t 5000
    
    $sense   = (($out | Select-String "^Sense-Reversing Barrier:").ToString() -split '\s+')[2]
    $sense_bo= (($out | Select-String "^Sense-Reversing Barrier Backoff:").ToString() -split '\s+')[3]
    $std_bar = (($out | Select-String "^std::barrier").ToString() -split '\s+')[2]

    "$t,$sense,$sense_bo,$std_bar" | Out-File -FilePath $barrier_file -Append -Encoding utf8
}

# 3. Problem 3: Concurrent Counters Sweep
$counters_file = "${CPU_LABEL}_counters.csv"
"threads,mutex_ms,cas_ms,cas_backoff_ms,faa_ms" | Out-File -FilePath $counters_file -Encoding utf8
Write-Host ""
Write-Host "=== 3. Running Concurrent Counters Benchmarks ===" -ForegroundColor Green
foreach ($t in 1, 2, 4, 6, 8, 12, 16) {
    Write-Host "[$CPU_LABEL Counters] Threads=$t"
    $out = .\bench_counters.exe $t 200000
    
    $mtx    = (($out | Select-String "^Mutex Counter:").ToString() -split '\s+')[2]
    $cas    = (($out | Select-String "^CAS Counter:").ToString() -split '\s+')[2]
    $cas_bo = (($out | Select-String "^CAS Backoff Counter:").ToString() -split '\s+')[3]
    $faa    = (($out | Select-String "^FAA Counter:").ToString() -split '\s+')[2]

    "$t,$mtx,$cas,$cas_bo,$faa" | Out-File -FilePath $counters_file -Append -Encoding utf8
}

# 4. Problem 4: Distributed MPI Sweep
$mpi_file = "${CPU_LABEL}_mpi.csv"
"processes,counter_ms,lock_ms,barrier_episodes_sec" | Out-File -FilePath $mpi_file -Encoding utf8
Write-Host ""
Write-Host "=== 4. Running Distributed MS-MPI Benchmarks ===" -ForegroundColor Green
foreach ($p in 1, 2, 4, 8, 16) {
    Write-Host "[$CPU_LABEL MPI] np=$p"
    $out = mpiexec.exe -n $p .\bench_mpi.exe
    
    $cnt = (($out | Select-String "\[MPI Counter\]").ToString() -split '\s+')[-2]
    $lck = (($out | Select-String "\[MPI Lock\]").ToString() -split '\s+')[-2]
    $bar = (($out | Select-String "\[MPI Barrier\]").ToString() -split '\s+')[2]

    "$p,$cnt,$lck,$bar" | Out-File -FilePath $mpi_file -Append -Encoding utf8
}

Write-Host ""
Write-Host "[OK] All 4 benchmarks complete!" -ForegroundColor Yellow
Write-Host "Generated: $locks_file, $barrier_file, $counters_file, $mpi_file" -ForegroundColor Yellow