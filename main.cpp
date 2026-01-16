
#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <mutex>
#include <atomic>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <algorithm>

// Simple hash function - converts password string to a hash value
uint32_t simpleHash(const std::string& password) {
    uint32_t hash = 0;
    for (char c : password) {
        hash = hash * 31 + static_cast<uint32_t>(c);
    }
    return hash;
}

// Global shared state
struct SearchState {
    uint32_t targetHash;
    std::string foundPassword;
    std::atomic<bool> passwordFound{false};
    std::atomic<long long> totalAttempts{0};
    std::mutex resultMutex;
    std::chrono::steady_clock::time_point startTime;
} searchState;

// Performance metrics
struct PerformanceMetrics {
    std::vector<long long> attemptsPerThread;
    std::vector<double> threadTimes;
    std::mutex logMutex;
    std::mutex outputMutex;
} perfMetrics;

// Character set for password generation (digits and lowercase letters)
const std::string CHARSET = "0123456789abcdefghijklmnopqrstuvwxyz";

std::string indexToPassword(long long index, int maxLength) {
    int base = CHARSET.length();
    
    // Find which length tier this index falls into
    long long cumulative = 0;
    int len = 1;
    long long tier_size = base;
    
    while (len <= maxLength && index >= cumulative + tier_size) {
        cumulative += tier_size;
        tier_size *= base;
        len++;
    }
    
    if (len > maxLength) {
        return ""; // Index out of bounds
    }
    
    // Get position within this length tier
    long long index_in_tier = index - cumulative;
    
    // Convert to password of length 'len'
    std::string password(len, CHARSET[0]);
    
    for (int i = len - 1; i >= 0; i--) {
        password[i] = CHARSET[index_in_tier % base];
        index_in_tier /= base;
    }
    
    return password;
}

/**
 * Calculate total key space size for passwords up to maxLength
 * 
 * @param maxLength Maximum password length
 * @return Total number of possible passwords
 */
long long calculateKeySpace(int maxLength) {
    int base = CHARSET.length();
    long long total = 0;
    long long power = 1;
    
    for (int i = 1; i <= maxLength; i++) {
        power *= base;
        total += power;
    }
    
    return total;
}


void crackerWorker(int threadId, long long startIndex, long long endIndex, int maxLength) {
    auto threadStartTime = std::chrono::steady_clock::now();
    long long attempts = 0;
    long long localBatchCount = 0;
    
    {
        std::lock_guard<std::mutex> lock(perfMetrics.outputMutex);
        std::cout << "[Thread " << threadId << "] Starting search from index " 
                  << startIndex << " to " << endIndex << std::endl;
    }
    
    // Search through assigned portion of key space
    for (long long i = startIndex; i < endIndex && !searchState.passwordFound.load(); ++i) {
        // Generate password candidate
        std::string candidate = indexToPassword(i, maxLength);
        
        if (candidate.empty()) {
            break; // Out of valid range
        }
        
        // Compute hash
        uint32_t hash = simpleHash(candidate);
        attempts++;
        localBatchCount++;
        
        // Check if hash matches target
        if (hash == searchState.targetHash) {
            // Found the password!
            std::lock_guard<std::mutex> lock(searchState.resultMutex);
            
            if (!searchState.passwordFound.load()) {
                searchState.passwordFound.store(true);
                searchState.foundPassword = candidate;
                searchState.totalAttempts.fetch_add(localBatchCount);
                localBatchCount = 0;
                
                {
                    std::lock_guard<std::mutex> outputLock(perfMetrics.outputMutex);
                    std::cout << "\n[Thread " << threadId << "] FOUND PASSWORD: \"" 
                              << candidate << "\" (after " << attempts << " attempts)" << std::endl;
                }
            }
            break;
        }
        
        // Periodic progress update (every 50000 attempts)
        if (localBatchCount >= 50000) {
            searchState.totalAttempts.fetch_add(localBatchCount);
            localBatchCount = 0;
        }
    }
    
    // Final attempt count update
    if (localBatchCount > 0) {
        searchState.totalAttempts.fetch_add(localBatchCount);
    }
    
    auto threadEndTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        threadEndTime - threadStartTime).count();
    
    // Record performance metrics
    {
        std::lock_guard<std::mutex> lock(perfMetrics.logMutex);
        perfMetrics.attemptsPerThread.push_back(attempts);
        perfMetrics.threadTimes.push_back(duration / 1000.0);
    }
    
    {
        std::lock_guard<std::mutex> lock(perfMetrics.outputMutex);
        std::cout << "[Thread " << threadId << "] Completed. Attempted " 
                  << attempts << " passwords in " 
                  << std::fixed << std::setprecision(2) << (duration / 1000.0) 
                  << " seconds" << std::endl;
    }
}

/**
 * Performance logging function
 * 
 * Writes performance metrics to a log file for analysis.
 */
void logPerformanceMetrics() {
    std::ofstream logFile("performance_log.txt");
    if (!logFile.is_open()) {
        std::cerr << "Warning: Could not open performance_log.txt for writing.\n";
        return;
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - searchState.startTime).count();
    
    logFile << "═══════════════════════════════════════════════════\n";
    logFile << "  PASSWORD CRACKER PERFORMANCE REPORT\n";
    logFile << "═══════════════════════════════════════════════════\n\n";
    
    logFile << "Total Search Duration: " << std::fixed << std::setprecision(3) 
            << (duration / 1000.0) << " seconds\n\n";
    
    logFile << "Throughput Metrics:\n";
    logFile << "  Total Attempts: " << searchState.totalAttempts.load() << "\n";
    
    if (duration > 0) {
        logFile << "  Attempts per Second: " << std::fixed << std::setprecision(2)
                << (searchState.totalAttempts.load() * 1000.0 / duration) 
                << " attempts/sec\n\n";
    } else {
        logFile << "  Attempts per Second: N/A (duration too short)\n\n";
    }
    
    logFile << "Thread Performance:\n";
    for (size_t i = 0; i < perfMetrics.attemptsPerThread.size(); ++i) {
        logFile << "  Thread " << i << ":\n";
        logFile << "    Attempts: " << perfMetrics.attemptsPerThread[i] << "\n";
        logFile << "    Time: " << std::fixed << std::setprecision(2) 
                << perfMetrics.threadTimes[i] << " seconds\n";
        
        if (perfMetrics.threadTimes[i] > 0) {
            logFile << "    Speed: " << std::fixed << std::setprecision(2)
                    << (perfMetrics.attemptsPerThread[i] / perfMetrics.threadTimes[i])
                    << " attempts/sec\n";
        }
        logFile << "\n";
    }
    
    logFile << "═══════════════════════════════════════════════════\n";
    logFile.close();
}

int main(int argc, char* argv[]) {
    // Configuration
    std::string targetPassword = "test";
    int numThreads = 4;
    int maxLength = 4;
    
    // Parse command line arguments
    if (argc >= 2) {
        targetPassword = argv[1];
    }
    if (argc >= 3) {
        numThreads = std::stoi(argv[2]);
        if (numThreads < 1) numThreads = 1;
    }
    if (argc >= 4) {
        maxLength = std::stoi(argv[3]);
        if (maxLength < 1) maxLength = 1;
        if (maxLength > 8) {
            std::cout << "Warning: maxLength > 8 may take very long. Limiting to 8.\n";
            maxLength = 8;
        }
    }
    
    // Calculate target hash
    searchState.targetHash = simpleHash(targetPassword);
    
    std::cout << "═══════════════════════════════════════════════════\n";
    std::cout << "  MULTITHREADED PASSWORD CRACKER\n";
    std::cout << "═══════════════════════════════════════════════════\n";
    std::cout << "Target Password: \"" << targetPassword << "\"\n";
    std::cout << "Target Hash: " << searchState.targetHash << "\n";
    std::cout << "Number of Threads: " << numThreads << "\n";
    std::cout << "Maximum Password Length: " << maxLength << "\n";
    std::cout << "Character Set: " << CHARSET << " (" << CHARSET.length() 
              << " characters)\n";
    std::cout << "═══════════════════════════════════════════════════\n\n";
    
    // Calculate key space size
    long long keySpaceSize = calculateKeySpace(maxLength);
    
    std::cout << "Key Space Size: " << keySpaceSize << " possible passwords\n";
    std::cout << "  (All passwords from length 1 to " << maxLength << ")\n\n";
    
    // Partition key space among threads
    long long rangePerThread = keySpaceSize / numThreads;
    long long remainder = keySpaceSize % numThreads;
    
    std::cout << "Key Space Partitioning:\n";
    for (int i = 0; i < numThreads; ++i) {
        long long start = i * rangePerThread;
        long long end = (i + 1) * rangePerThread;
        if (i == numThreads - 1) {
            end += remainder;
        }
        std::cout << "  Thread " << i << ": indices " << start << " to " << end 
                  << " (" << (end - start) << " passwords)\n";
    }
    std::cout << "\n";
    
    searchState.startTime = std::chrono::steady_clock::now();
    
    // Start worker threads
    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i) {
        long long start = i * rangePerThread;
        long long end = (i + 1) * rangePerThread;
        if (i == numThreads - 1) {
            end += remainder;
        }
        
        threads.emplace_back(crackerWorker, i, start, end, maxLength);
    }
    
    // Wait for all threads to complete
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - searchState.startTime).count();
    
    // Display results
    std::cout << "\n═══════════════════════════════════════════════════\n";
    std::cout << "  RESULTS\n";
    std::cout << "═══════════════════════════════════════════════════\n";
    
    if (searchState.passwordFound.load()) {
        std::cout << "✓ Password FOUND: \"" << searchState.foundPassword << "\"\n";
        std::cout << "  Expected: \"" << targetPassword << "\"\n";
        
        if (searchState.foundPassword == targetPassword) {
            std::cout << "  Status: EXACT MATCH ✓\n";
        } else {
            std::cout << "  Status: Hash collision (different password, same hash)\n";
        }
    } else {
        std::cout << "✗ Password NOT FOUND in searched key space\n";
        std::cout << "  (Password may be longer than maxLength=" << maxLength << ")\n";
    }
    
    std::cout << "\nPerformance Summary:\n";
    std::cout << "  Total Attempts: " << searchState.totalAttempts.load() << "\n";
    std::cout << "  Total Time: " << std::fixed << std::setprecision(3) 
              << (duration / 1000.0) << " seconds\n";
    
    if (duration > 0) {
        std::cout << "  Throughput: " << std::fixed << std::setprecision(2)
                  << (searchState.totalAttempts.load() * 1000.0 / duration) 
                  << " attempts/sec\n";
    }
    
    // Log performance metrics
    logPerformanceMetrics();
    
    std::cout << "\nDetailed metrics saved to: performance_log.txt\n";
    std::cout << "═══════════════════════════════════════════════════\n";
    
    return 0;
}
