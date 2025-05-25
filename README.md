# Order Book Benchmarking Project

## Project Overview

This project benchmarks different implementations of an order book data structure, comparing their performance characteristics for financial trading applications. The implementations range from theoretically optimal approaches to empirically fast solutions, as discussed in CppCon 2024's talk on ultra-fast trading systems.

## Implementations Compared

1. **Map-based Order Book**
   - **Data Structures**: `std::map` for price levels, `std::unordered_map` for order tracking
   - **Complexity**:
     - AddOrder: O(log N)
     - ModifyOrder: O(1) amortized (using stable iterators)
     - DeleteOrder: O(1) amortized
   - **Design Choice**: Theoretical optimal for modification operations

2. **Vector-based with Binary Search**
   - **Data Structures**: Sorted `std::vector` for price levels
   - **Complexity**:
     - AddOrder: O(log N) for search + O(N) for insertion
     - ModifyOrder: O(log N)
     - DeleteOrder: O(log N) for search + O(N) for deletion
   - **Design Choice**: Better cache locality than maps

3. **Reverse Vector-based**
   - **Data Structures**: Reverse-sorted `std::vector` (best prices at end)
   - **Complexity**: Same as vector-based but optimized for common case
   - **Design Choice**: Minimizes shifts when adding/deleting top-of-book orders

4. **Linear Search**
   - **Data Structures**: `std::vector` with linear search
   - **Complexity**:
     - AddOrder: O(N)
     - ModifyOrder: O(N)
     - DeleteOrder: O(N)
   - **Design Choice**: Better cache behavior despite worse complexity

## Building the Project

### Prerequisites
- C++17 compatible compiler (GCC, Clang, or MSVC)
- CMake (optional, but recommended)

### Build Instructions

**Basic build:**
```bash
g++ -std=c++17 -O3 -march=native benchmark.cpp -o orderbook_benchmark
```

**Plotting:**
```python
python3 plot.py
```

## Sample Output

The benchmark produces comprehensive statistics for each implementation. Here's a condensed example of the output format (11th Gen I7 CPU Ubuntu Linux):

| Implementation        | Operation          | Mean (μs) | Median (μs) | StdDev   | Min  | Max  |
|-----------------------|-------------------|----------|------------|---------|------|------|
| Map-based             | Add 10000 orders  | 1437.9   | 723.5      | 1730.73 | 681  | 6465 |
|                       | Modify 5000 orders| 308      | 297.5      | 27.83   | 280  | 364  |
|                       | Delete 3333 orders| 241.9    | 233.5      | 24.02   | 216  | 297  |
|                       | Best price lookup | 0.33 ns  | 0.33 ns    | 0.009   | 0.32 | 0.35 |
| Vector (binary search)| Add 10000 orders  | 699.9    | 688        | 27.98   | 667  | 755  |
|                       | Modify 5000 orders| 266.7    | 255.5      | 25.39   | 243  | 319  |
|                       | Delete 3333 orders| 205.8    | 201        | 18.64   | 189  | 255  |
|                       | Best price lookup | 0.25 ns  | 0.25 ns    | 0.010   | 0.23 | 0.27 |
| Reverse vector        | Add 10000 orders  | 686.3    | 686.5      | 12.39   | 663  | 708  |
|                       | Modify 5000 orders| 250.7    | 249        | 6.02    | 244  | 262  |
|                       | Delete 3333 orders| 201      | 201        | 3.26    | 196  | 208  |
|                       | Best price lookup | 0.25 ns  | 0.26 ns    | 0.014   | 0.23 | 0.28 |
| Linear search         | Add 10000 orders  | 938.5    | 933.5      | 13.51   | 925  | 969  |
|                       | Modify 5000 orders| 446.3    | 432.5      | 25.08   | 428  | 499  |
|                       | Delete 3333 orders| 317.8    | 313        | 9.09    | 310  | 341  |
|                       | Best price lookup | 0.33 ns  | 0.34 ns    | 0.023   | 0.27 | 0.35 |

Key observations from this benchmark:
1. **Reverse vector** shows the most consistent performance (lowest StdDev) across all operations
2. **Map-based** implementation has high variance in add operations (StdDev 1730μs)
3. **Binary search** variants outperform linear search by 25-30% for order modifications
4. **Best price lookup** is fastest with vector-based implementations (~0.25ns)

Note: Results may vary based on CPU architecture, background processes, and memory configuration.