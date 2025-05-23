// benchmark.cpp
#include "orderbook.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

// Configuration
constexpr size_t WARMUP_RUNS = 3;
constexpr size_t MEASURED_RUNS = 10;
constexpr size_t NUM_ORDERS = 10000;

struct BenchmarkStats {
    double mean;
    double median;
    double min;
    double max;
    double stddev;
};

template <typename T>
BenchmarkStats calculateStats(const std::vector<T>& values) {
    BenchmarkStats stats{};
    if (values.empty()) return stats;

    // Calculate mean
    double sum = std::accumulate(values.begin(), values.end(), 0.0);
    stats.mean = sum / values.size();

    // Calculate median
    std::vector<T> sorted = values;
    std::sort(sorted.begin(), sorted.end());
    stats.median = sorted[sorted.size() / 2];
    if (sorted.size() % 2 == 0) {
        stats.median = (stats.median + sorted[sorted.size() / 2 - 1]) / 2;
    }

    // Min/max
    stats.min = *std::min_element(values.begin(), values.end());
    stats.max = *std::max_element(values.begin(), values.end());

    // Standard deviation
    double sq_sum = std::inner_product(values.begin(), values.end(), values.begin(), 0.0);
    stats.stddev = std::sqrt(sq_sum / values.size() - stats.mean * stats.mean);

    return stats;
}

void writeCSV(const std::string& filename,
              const std::vector<std::string>& implementations,
              const std::vector<BenchmarkStats>& addStats,
              const std::vector<BenchmarkStats>& modifyStats,
              const std::vector<BenchmarkStats>& deleteStats,
              const std::vector<BenchmarkStats>& bestPriceStats) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open " << filename << " for writing\n";
        return;
    }

    // Header
    file << "Implementation,Add Mean,Add Median,Add StdDev,Add Min,Add Max,"
         << "Modify Mean,Modify Median,Modify StdDev,Modify Min,Modify Max,"
         << "Delete Mean,Delete Median,Delete StdDev,Delete Min,Delete Max,"
         << "BestPrice Mean,BestPrice Median,BestPrice StdDev,BestPrice Min,BestPrice Max\n";

    // Data
    for (size_t i = 0; i < implementations.size(); ++i) {
        file << implementations[i] << ","
             << addStats[i].mean << "," << addStats[i].median << "," << addStats[i].stddev << ","
             << addStats[i].min << "," << addStats[i].max << ","
             << modifyStats[i].mean << "," << modifyStats[i].median << "," << modifyStats[i].stddev << ","
             << modifyStats[i].min << "," << modifyStats[i].max << ","
             << deleteStats[i].mean << "," << deleteStats[i].median << "," << deleteStats[i].stddev << ","
             << deleteStats[i].min << "," << deleteStats[i].max << ","
             << bestPriceStats[i].mean << "," << bestPriceStats[i].median << "," << bestPriceStats[i].stddev << ","
             << bestPriceStats[i].min << "," << bestPriceStats[i].max << "\n";
    }
}

void plotResults(const std::string& csvFilename) {
    // This would typically call Python or gnuplot, but for completeness:
    std::cout << "\nTo generate plots, either execute 'plot.py'";
    std::cout << " or run the following scripts:\n\n";
    std::cout << "import pandas as pd\n";
    std::cout << "import matplotlib.pyplot as plt\n\n";
    std::cout << "df = pd.read_csv('" << csvFilename << "')\n";
    std::cout << "fig, axes = plt.subplots(2, 2, figsize=(12, 10))\n\n";
    std::cout << "# Add Orders\n";
    std::cout << "df.plot.bar(x='Implementation', y='Add Mean', yerr='Add StdDev', capsize=4, ax=axes[0,0], title='Add Orders (μs)')\n";
    std::cout << "# Modify Orders\n";
    std::cout << "df.plot.bar(x='Implementation', y='Modify Mean', yerr='Modify StdDev', capsize=4, ax=axes[0,1], title='Modify Orders (μs)')\n";
    std::cout << "# Delete Orders\n";
    std::cout << "df.plot.bar(x='Implementation', y='Delete Mean', yerr='Delete StdDev', capsize=4, ax=axes[1,0], title='Delete Orders (μs)')\n";
    std::cout << "# Best Price\n";
    std::cout << "df.plot.bar(x='Implementation', y='BestPrice Mean', yerr='BestPrice StdDev', capsize=4, ax=axes[1,1], title='Best Price Lookup (ns)')\n\n";
    std::cout << "plt.tight_layout()\n";
    std::cout << "plt.savefig('orderbook_benchmark.png')\n";
    std::cout << "plt.show()\n";
}

template <typename OrderBookT>
void runBenchmark(const std::vector<std::tuple<OrderId, Side, Price, Volume>>& orders,
                  const std::vector<std::pair<OrderId, Volume>>& modifications,
                  const std::vector<OrderId>& deletions,
                  std::vector<long>& addTimes,
                  std::vector<long>& modifyTimes,
                  std::vector<long>& deleteTimes,
                  std::vector<double>& bestPriceTimes) {
    OrderBookT book;

    // Warm up
    for (size_t i = 0; i < WARMUP_RUNS; ++i) {
        for (const auto& [id, side, price, vol] : orders) {
            book.addOrder(id, side, price, vol);
        }
        for (const auto& [id, vol] : modifications) {
            book.modifyOrder(id, vol);
        }
        for (const auto& id : deletions) {
            book.deleteOrder(id);
        }
        book.clear();
    }

    // Measured runs
    for (size_t i = 0; i < MEASURED_RUNS; ++i) {
        // Benchmark add orders
        auto start = std::chrono::high_resolution_clock::now();
        for (const auto& [id, side, price, vol] : orders) {
            book.addOrder(id, side, price, vol);
        }
        auto end = std::chrono::high_resolution_clock::now();
        addTimes.push_back(std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());

        // Benchmark modify orders
        start = std::chrono::high_resolution_clock::now();
        for (const auto& [id, vol] : modifications) {
            book.modifyOrder(id, vol);
        }
        end = std::chrono::high_resolution_clock::now();
        modifyTimes.push_back(std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());

        // Benchmark delete orders
        start = std::chrono::high_resolution_clock::now();
        for (const auto& id : deletions) {
            book.deleteOrder(id);
        }
        end = std::chrono::high_resolution_clock::now();
        deleteTimes.push_back(std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());

        // Benchmark get best prices
        start = std::chrono::high_resolution_clock::now();
        for (int j = 0; j < 1000; ++j) {
            volatile auto best = book.getBestPrices();
            (void)best;
        }
        end = std::chrono::high_resolution_clock::now();
        bestPriceTimes.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / 1000.0);

        book.clear();
    }
}

void printStats(const std::string& name, const BenchmarkStats& stats, const std::string& unit) {
    std::cout << std::left << std::setw(30) << name
              << " Mean: " << std::setw(8) << stats.mean
              << " Median: " << std::setw(8) << stats.median
              << " StdDev: " << std::setw(8) << stats.stddev
              << " Min: " << std::setw(8) << stats.min
              << " Max: " << std::setw(8) << stats.max
              << " " << unit << "\n";
}

int main() {
    // Generate test data
    std::vector<std::tuple<OrderId, Side, Price, Volume>> orders;
    std::vector<std::pair<OrderId, Volume>> modifications;
    std::vector<OrderId> deletions;

    generateTestData(NUM_ORDERS, orders, modifications, deletions);

    // Implementations to test
    std::vector<std::string> implementations = {
        "Map-based",
        "Vector (binary search)",
        "Reverse vector",
        "Linear search"
    };

    // Storage for all results
    std::vector<std::vector<long>> allAddTimes(implementations.size());
    std::vector<std::vector<long>> allModifyTimes(implementations.size());
    std::vector<std::vector<long>> allDeleteTimes(implementations.size());
    std::vector<std::vector<double>> allBestPriceTimes(implementations.size());

    // Run benchmarks for each implementation
    runBenchmark<OrderBookMap>(orders, modifications, deletions,
                              allAddTimes[0], allModifyTimes[0], allDeleteTimes[0], allBestPriceTimes[0]);
    runBenchmark<OrderBookVector>(orders, modifications, deletions,
                                allAddTimes[1], allModifyTimes[1], allDeleteTimes[1], allBestPriceTimes[1]);
    runBenchmark<OrderBookReverseVector>(orders, modifications, deletions,
                                       allAddTimes[2], allModifyTimes[2], allDeleteTimes[2], allBestPriceTimes[2]);
    runBenchmark<OrderBookLinear>(orders, modifications, deletions,
                                allAddTimes[3], allModifyTimes[3], allDeleteTimes[3], allBestPriceTimes[3]);

    // Calculate statistics
    std::vector<BenchmarkStats> addStats, modifyStats, deleteStats, bestPriceStats;
    for (size_t i = 0; i < implementations.size(); ++i) {
        addStats.push_back(calculateStats(allAddTimes[i]));
        modifyStats.push_back(calculateStats(allModifyTimes[i]));
        deleteStats.push_back(calculateStats(allDeleteTimes[i]));
        bestPriceStats.push_back(calculateStats(allBestPriceTimes[i]));
    }

    // Print results
    std::cout << "\nBenchmark Results (over " << MEASURED_RUNS << " runs)\n";
    std::cout << "=============================================\n";
    for (size_t i = 0; i < implementations.size(); ++i) {
        std::cout << "\nImplementation: " << implementations[i] << "\n";
        printStats("  Add " + std::to_string(NUM_ORDERS) + " orders", addStats[i], "μs");
        printStats("  Modify " + std::to_string(modifications.size()) + " orders", modifyStats[i], "μs");
        printStats("  Delete " + std::to_string(deletions.size()) + " orders", deleteStats[i], "μs");
        printStats("  Get best prices", bestPriceStats[i], "ns");
    }

    // Save to CSV and generate plots
    const std::string csvFilename = "orderbook_benchmark.csv";
    writeCSV(csvFilename, implementations, addStats, modifyStats, deleteStats, bestPriceStats);
    plotResults(csvFilename);

    return 0;
}
