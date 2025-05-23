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

| Implementation        | Operation          | Mean (μs) | Median (μs) | StdDev  | Min  | Max  |
|-----------------------|-------------------|----------|------------|--------|------|------|
| Map-based             | Add 10000 orders  | 1268.6   | 1361       | 441.64 | 720  | 2293 |
|                       | Modify 5000 orders| 997      | 544.5      | 1366.9 | 279  | 4978 |
|                       | Delete 3333 orders| 1022.6   | 453        | 1809.1 | 223  | 6432 |
|                       | Best price lookup | 0.48 ns  | 0.49 ns    | 0.14   | 0.27 | 0.74 |
| Vector (binary search)| Add 10000 orders  | 729.1    | 704.5      | 64.58  | 656  | 856  |
|                       | Modify 5000 orders| 285.2    | 249        | 71.32  | 240  | 471  |
|                       | Delete 3333 orders| 230.3    | 201.5      | 50.66  | 196  | 356  |
|                       | Best price lookup | 0.25 ns  | 0.25 ns    | 0.005  | 0.24 | 0.26 |
| Reverse vector        | Add 10000 orders  | 707.5    | 697.5      | 49.98  | 667  | 846  |
|                       | Modify 5000 orders| 281.4    | 255.5      | 57.16  | 240  | 436  |
|                       | Delete 3333 orders| 207.1    | 199.5      | 17.39  | 191  | 253  |
|                       | Best price lookup | 0.26 ns  | 0.25 ns    | 0.02   | 0.25 | 0.33 |
| Linear search         | Add 10000 orders  | 1220.5   | 1130.5     | 193.31 | 1051 | 1664 |
|                       | Modify 5000 orders| 452.8    | 435        | 53.44  | 423  | 609  |
|                       | Delete 3333 orders| 325.4    | 314        | 24.04  | 310  | 392  |
|                       | Best price lookup | 0.30 ns  | 0.31 ns    | 0.03   | 0.24 | 0.34 |