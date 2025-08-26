# Real-Time Programming with Linux (Raspberry Pi 5)

## Project Overview

This project demonstrates real-time programming capabilities on Linux using POSIX real-time extensions and thread scheduling policies. The implementation explores the impact of CPU affinity, scheduling policies, and workload distribution on real-time and non-real-time thread performance.

## Project Goals

- Enable real-time capabilities through kernel patching with PREEMPT_RT
- Create and manage real-time (RT) and non-real-time (NRT) threads using POSIX pthread APIs
- Compare performance under different scheduling policies (SCHED_FIFO, SCHED_RR)
- Analyze the impact of CPU affinity on thread execution times
- Measure and report execution performance across various experimental configurations

## Core Components

### 1. Real-Time Thread Classes

#### ThreadRT Class
- Manages real-time threads with configurable scheduling policies
- Supports SCHED_FIFO and SCHED_RR scheduling
- Configurable priority levels (default: 80)
- Memory locking capabilities for deterministic performance
- CPU affinity support

#### ThreadNRT Class  
- Manages non-real-time threads
- Uses default CFS (Completely Fair Scheduler) policy
- Provides baseline performance comparison

### 2. Application Types

#### AppTypeX (Real-Time)
- Inherits from ThreadRT
- Executes compute-intensive tasks (BusyCal)
- Demonstrates real-time scheduling behavior

#### AppTypeY (Non-Real-Time)
- Inherits from ThreadNRT
- Executes the same compute workload as AppTypeX
- Shows standard Linux scheduling behavior

### 3. Core Functions

#### BusyCal()
```cpp
void BusyCal() {
    long abc = 1e9;
    while (abc--) {
        long tmp = 10000 * 10000;
    }
}
```
- Simulates CPU-intensive computation
- Provides consistent workload for performance comparison

#### CannyP3()
- Implements Canny edge detection on video frames
- Processes video input ("ground_crew_480p.mp4")
- Demonstrates real-world computer vision workload
- Configurable parameters: sigma=1, tlow=0.2, thigh=0.6

## Experimental Configurations

The project includes 5 different experimental setups:

| Experiment | Description | CPU Binding | RT Threads | NRT Threads | Scheduling |
|------------|-------------|-------------|------------|-------------|------------|
| 0 | One RT + Two NRT | CPU=1 | 1 (SCHED_FIFO) | 2 | Bound |
| 1 | Same as 0 | Free | 1 (SCHED_FIFO) | 2 | Free |
| 2 | Two RT + One NRT | CPU=1 | 2 (SCHED_FIFO) | 1 | Bound |
| 3 | Two RT + One NRT | CPU=1 | 2 (SCHED_RR) | 1 | Bound |
| 4 | Same as 2 | Free | 2 (SCHED_FIFO) | 1 | Free |

## Key Features

### Memory Management
- `mlockall()` prevents memory swapping for deterministic performance
- Stack size configuration for RT threads (1MB)
- Explicit scheduling inheritance control

### CPU Affinity Control
```cpp
void setCPU(int cpu_id = 1) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}
```
- Controllable via `#define SET_CPU` flag
- Allows comparison between bound and free CPU allocation

### Performance Measurement
- High-resolution timing using `gettimeofday()`
- Thread-specific runtime tracking
- CPU usage reporting via `sched_getcpu()`

## Build and Execution

### Compilation
```bash
g++ -o p3 p3.cpp p3_util.cpp -lpthread `pkg-config --cflags --libs opencv4`
```

### Execution
```bash
# Run specific experiment (0-4)
./p3 <experiment_id>

# Example: Run experiment 0
./p3 0
```

### Sample Output
```
Experiment 1: One CannyP3 APP (RT) and Two any-type APPs (NRT), All running on CPU=1
[RT thread #140234567890] running on CPU #1
[RT thread #140234567890] Scheduling policy: SCHED_FIFO with priority 80
Running App #1...
[NRT thread #140234567891] running on CPU #1
Running App #2...
App #1 runtime: 2.320000 seconds
App #2 runtime: 6.810000 seconds
App #3 runtime: 6.800000 seconds
```

## Key Findings

### CPU Affinity Impact
- **Bound to CPU=1**: RT threads experience delays when multiple threads compete for the same CPU
- **Free allocation**: Significant performance improvement (~65% reduction in NRT thread runtime)

### Scheduling Policy Comparison
- **SCHED_FIFO**: Provides predictable, deterministic behavior for RT threads
- **SCHED_RR**: Offers fairness but introduces slight overhead (~2% increase in runtime)

### Performance Results Summary

| Configuration | RT Thread Time | NRT Thread Time | Performance Impact |
|---------------|----------------|-----------------|-------------------|
| Single CPU Bound | 2.32s | 6.81s | High contention |
| Multiple CPU Free | 2.32s | 2.31s | Optimal performance |
| Multiple RT (FIFO) | 2.37s / 4.73s | 6.80s | RT serialization |
| Multiple RT (RR) | 4.66s / 4.73s | 6.80s | Fair sharing overhead |

## Conclusions

1. **CPU Affinity**: Free CPU allocation significantly improves overall system performance
2. **Scheduling Policies**: SCHED_FIFO provides better determinism than SCHED_RR for single-threaded RT applications
3. **Resource Contention**: Multiple RT threads on the same CPU lead to serialized execution
4. **Load Balancing**: Modern multi-core systems benefit from allowing the scheduler to distribute threads across available CPUs
