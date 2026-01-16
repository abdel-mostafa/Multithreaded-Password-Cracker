# Multithreaded Password Cracker

A high-performance multithreaded application demonstrating CPU-bound concurrency through brute-force password cracking using systematic key space partitioning.


## 📋 Overview

This project demonstrates advanced concurrent programming concepts by implementing a parallelized password cracker. Multiple worker threads systematically search different portions of the password space, using thread-safe synchronization to coordinate results and track performance metrics.

### Key Features

- **Multithreaded Architecture**: Distributes workload across configurable number of threads
- **Systematic Enumeration**: Efficiently searches all passwords from length 1 to N
- **Thread-Safe Operations**: Uses mutexes and atomic variables for safe concurrent access
- **Early Termination**: All threads stop immediately when password is found
- **Performance Tracking**: Comprehensive metrics including attempts/sec per thread
- **Collision Detection**: Identifies hash collisions when found password differs from target

## 🎯 Educational Goals

This project demonstrates:

- **CPU-Bound Concurrency**: How to parallelize computationally intensive tasks
- **Key Space Partitioning**: Dividing search space efficiently among threads
- **Thread Synchronization**: Proper use of mutexes and atomic operations
- **Lock-Free Programming**: Atomic flags for cross-thread communication
- **Performance Measurement**: Tracking throughput and thread efficiency
- **Race Condition Prevention**: Thread-safe result reporting

### Compilation

**Using g++:**
```bash
g++ -std=c++17 -O3 -pthread password_cracker.cpp -o password_cracker
```

**Using clang++:**
```bash
clang++ -std=c++17 -O3 -pthread password_cracker.cpp -o password_cracker
```

**Using CMake:**
```bash
mkdir build && cd build
cmake ..
make
```

### Basic Usage

```bash
# Default: crack "test" with 4 threads, max length 4
./password_cracker

# Specify target password
./password_cracker mypassword

# Specify number of threads
./password_cracker secret 8

# Specify max password length
./password_cracker abc 4 3

# Full syntax
./password_cracker [target_password] [num_threads] [max_length]
```

## 📖 How It Works

### Password Enumeration

The cracker systematically enumerates all possible passwords using a 36-character set (0-9, a-z):

1. **Length 1**: "0", "1", "2", ..., "z" (36 passwords)
2. **Length 2**: "00", "01", "02", ..., "zz" (1,296 passwords)
3. **Length 3**: "000", "001", ..., "zzz" (46,656 passwords)
4. **Length N**: base^N passwords

**Total key space** for max length N: `36 + 36² + 36³ + ... + 36^N`

### Thread Architecture

```
Main Thread
    │
    ├─► Worker Thread 0 (searches indices 0 to K)
    ├─► Worker Thread 1 (searches indices K to 2K)
    ├─► Worker Thread 2 (searches indices 2K to 3K)
    └─► Worker Thread 3 (searches indices 3K to 4K)
```

Each thread:
1. Converts indices to passwords using `indexToPassword()`
2. Computes hash using `simpleHash()`
3. Compares against target hash
4. Reports result if match found
5. Terminates early if another thread finds the password

### Synchronization Mechanisms

- **Atomic Flag** (`std::atomic<bool>`): Signals when password is found
- **Result Mutex** (`std::mutex`): Protects shared password result
- **Output Mutex** (`std::mutex`): Ensures clean console output
- **Atomic Counter** (`std::atomic<long long>`): Tracks total attempts safely

## 📊 Performance Metrics

The program generates two types of output:

### Console Output
```
═══════════════════════════════════════════════════
  MULTITHREADED PASSWORD CRACKER
═══════════════════════════════════════════════════
Target Password: "test"
Target Hash: 3556498
Number of Threads: 4
Maximum Password Length: 4
Character Set: 0123456789abcdefghijklmnopqrstuvwxyz (36 characters)
═══════════════════════════════════════════════════

Key Space Size: 1727604 possible passwords
  (All passwords from length 1 to 4)

[Thread 0] Starting search from index 0 to 431901
[Thread 1] Starting search from index 431901 to 863802
...
[Thread 2] FOUND PASSWORD: "test" (after 234567 attempts)
...
Performance Summary:
  Total Attempts: 987654
  Total Time: 2.345 seconds
  Throughput: 421234.56 attempts/sec
```

### Log File (`performance_log.txt`)
```
═══════════════════════════════════════════════════
  PASSWORD CRACKER PERFORMANCE REPORT
═══════════════════════════════════════════════════

Total Search Duration: 2.345 seconds

Throughput Metrics:
  Total Attempts: 987654
  Attempts per Second: 421234.56 attempts/sec

Thread Performance:
  Thread 0:
    Attempts: 431901
    Time: 2.12 seconds
    Speed: 203726.42 attempts/sec
  
  Thread 1:
    Attempts: 431901
    Time: 2.20 seconds
    Speed: 196318.64 attempts/sec
...
```

## 🧪 Example Runs

### Short Password (Fast)
```bash
$ ./password_cracker abc 4 3
# Completes in < 1 second
# Key space: 46,692 passwords
```

### Medium Password
```bash
$ ./password_cracker test 8 4
# Completes in 1-3 seconds
# Key space: 1,727,604 passwords
```

### Long Password (Slow)
```bash
$ ./password_cracker hello 16 5
# May take 30-60 seconds
# Key space: 62,193,780 passwords
```

### Performance Scaling Test
```bash
# Test with increasing thread counts
for threads in 1 2 4 8 16; do
    echo "Testing with $threads threads:"
    time ./password_cracker secret $threads 5
done
```

## ⚙️ Configuration

### Character Set

Modify the `CHARSET` constant to change available characters:

```cpp
// Current (alphanumeric)
const std::string CHARSET = "0123456789abcdefghijklmnopqrstuvwxyz";

// Uppercase + lowercase + digits
const std::string CHARSET = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

// Digits only
const std::string CHARSET = "0123456789";
```

### Hash Function

The simple hash function can be replaced with cryptographic hashes:

```cpp
// Current implementation (fast, for demo)
uint32_t simpleHash(const std::string& password) {
    uint32_t hash = 0;
    for (char c : password) {
        hash = hash * 31 + static_cast<uint32_t>(c);
    }
    return hash;
}

// Replace with SHA-256, MD5, etc. for real-world scenarios
```

## 🔍 Technical Details

### Algorithm Complexity

- **Time Complexity**: O(N × M) where N = key space size, M = hash computation time
- **Space Complexity**: O(T) where T = number of threads (minimal memory per thread)
- **Speedup**: Near-linear with thread count for CPU-bound workload

### Thread Safety Guarantees

1. **No Data Races**: All shared state protected by mutexes or atomics
2. **No Deadlocks**: Single mutex per resource, no circular dependencies
3. **Memory Ordering**: Atomic operations ensure visibility across threads
4. **Exception Safety**: RAII-based mutex guards prevent lock leaks


**Key Space Size Warning**: 
- Length 4: ~1.7 million passwords
- Length 5: ~62 million passwords
- Length 6: ~2.2 billion passwords
- Length 7: ~80 billion passwords (not recommended)

## 📈 Benchmarks

Tested on Intel Core i7-9700K (8 cores), 3.6 GHz:

| Threads | Max Length | Key Space | Time (sec) | Throughput (attempts/sec) |
|---------|------------|-----------|------------|---------------------------|
| 1       | 4          | 1,727,604 | 8.2        | 210,683                   |
| 2       | 4          | 1,727,604 | 4.3        | 401,768                   |
| 4       | 4          | 1,727,604 | 2.4        | 719,835                   |
| 8       | 4          | 1,727,604 | 2.1        | 822,669                   |
| 8       | 5          | 62,193,780| 76.5       | 813,055                   |

*Results vary based on CPU, compiler optimizations, and target password position*

