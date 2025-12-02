#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <cstdlib>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

// Path to your implementation binary
const std::string IMPLEMENTATION_BIN = "../implementation/bin/main.o";
const std::string GENERATOR_BIN = "./bin/generator";
const std::string TEMP_DATA_FILE = "temp_bench_data.txt";
const std::string RESULTS_FILE = "benchmark_results.csv";

struct TestCase {
    std::string algorithm; // "exact" or "approx"
    int sizeG;
    int sizeH;
    double density;
    int iterations;
};

void run_benchmark(const TestCase& tc, std::ofstream& csv) {
    double totalTime = 0;
    
    for(int i=0; i<tc.iterations; ++i) {
        // 1. Generate Data
        std::string genCmd = GENERATOR_BIN + " " + 
                             std::to_string(tc.sizeG) + " " + 
                             std::to_string(tc.sizeH) + " " + 
                             std::to_string(tc.density) + " 3 " + // max multiplicity 3
                             TEMP_DATA_FILE;
        
        if (system(genCmd.c_str()) != 0) {
            std::cerr << "Failed to generate data.\n";
            return;
        }

        // 2. Run Implementation & Measure Time
        // We pipe output to /dev/null to avoid cluttering console, 
        // unless you want to parse the cost (which requires popen).
        // For this benchmark, we focus on execution time.
        std::string runCmd = IMPLEMENTATION_BIN + " " + tc.algorithm + " " + TEMP_DATA_FILE + " 1 > /dev/null 2>&1";

        auto start = std::chrono::high_resolution_clock::now();
        int ret = system(runCmd.c_str());
        auto end = std::chrono::high_resolution_clock::now();

        if (ret != 0) {
            std::cerr << "Implementation crashed or returned error.\n";
        }

        std::chrono::duration<double, std::milli> duration = end - start;
        totalTime += duration.count();
    }

    double avgTime = totalTime / tc.iterations;

    // Output to console
    std::cout << "Benchmarked " << tc.algorithm 
              << " | G=" << tc.sizeG << " H=" << tc.sizeH 
              << " | Avg Time: " << avgTime << " ms" << std::endl;

    // Output to CSV
    csv << tc.algorithm << "," << tc.sizeG << "," << tc.sizeH << "," 
        << tc.density << "," << avgTime << "\n";
}

int main() {
    // Ensure bin directory exists
    if (!fs::exists("bin")) fs::create_directory("bin");

    std::ofstream csv(RESULTS_FILE);
    csv << "Algorithm,SizeG,SizeH,Density,TimeMS\n";

    std::cout << "Starting Benchmark Suite..." << std::endl;

    // --- EXACT ALGORITHM TESTS ---
    // Small graphs because exact is exponential
    std::vector<int> exact_sizes = {3, 4, 5, 6, 7, 8}; 
    for (int n : exact_sizes) {
        run_benchmark({"exact", n, n+2, 0.5, 5}, csv);
    }

    // --- APPROX ALGORITHM TESTS ---
    // Larger graphs
    std::vector<int> approx_sizes = {5, 10, 15, 20, 30, 50};
    for (int n : approx_sizes) {
        run_benchmark({"approx", n, n+5, 0.5, 3}, csv);
    }

    csv.close();
    std::cout << "Benchmark complete. Results saved to " << RESULTS_FILE << std::endl;
    
    // Clean up
    fs::remove(TEMP_DATA_FILE);

    return 0;
}
