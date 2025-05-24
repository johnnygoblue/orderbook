#pragma once

#include <algorithm>
#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <map>
#include <random>
#include <unordered_map>
#include <vector>

// Two ordered sequences:
// - Bids: from highest to lowest price
// - Asks: from lowest to highest price
enum class Side { Bid, Ask };

// Each order has an ID (uint64_t) which is unique throughout
// the trading session (day)
using OrderId = uint64_t;

// Price Level (a typical stock order book has ~1000 price levels per side)
// - Price
// - Size: sum of all orders' size at this price level
using Price = int64_t;
using Volume = int64_t;

// Helper macro for assertions
#define EXPECT(cond, msg) assert((cond) && msg)

// Common order data structure
struct Order {
    Side side;
    Price price;
    Volume volume;
};

// Base class for order book implementations
class OrderBookBase {
public:
    virtual ~OrderBookBase() = default;
    virtual void addOrder(OrderId orderId, Side side, Price price, Volume volume) = 0;
    virtual void modifyOrder(OrderId orderId, Volume newVolume) = 0;
    virtual void deleteOrder(OrderId orderId) = 0;
    virtual std::pair<Price, Price> getBestPrices() const = 0;
    virtual void clear() = 0;
};

// Implementation 1: Map-based order book
class OrderBookMap : public OrderBookBase {
public:
    void addOrder(OrderId orderId, Side side, Price price, Volume volume) override {
        auto [it, inserted] = mOrders.emplace(orderId, Order{side, price, volume});
        EXPECT(inserted, "duplicate order");

        if (side == Side::Bid) {
            auto [levelIt, levelInserted] = mBidLevels.try_emplace(price, volume);
            if (!levelInserted) {
                levelIt->second += volume;
            }
        } else {
            auto [levelIt, levelInserted] = mAskLevels.try_emplace(price, volume);
            if (!levelInserted) {
                levelIt->second += volume;
            }
        }
    }

    void modifyOrder(OrderId orderId, Volume newVolume) override {
        auto it = mOrders.find(orderId);
        EXPECT(it != mOrders.end(), "missing order");

        auto& order = it->second;
        Volume delta = newVolume - order.volume;
        order.volume = newVolume;

        if (order.side == Side::Bid) {
            auto levelIt = mBidLevels.find(order.price);
            levelIt->second += delta;
        } else {
            auto levelIt = mAskLevels.find(order.price);
            levelIt->second += delta;
        }
    }

    void deleteOrder(OrderId orderId) override {
        auto it = mOrders.find(orderId);
        EXPECT(it != mOrders.end(), "missing order");

        const auto& order = it->second;
        if (order.side == Side::Bid) {
            auto levelIt = mBidLevels.find(order.price);
            levelIt->second -= order.volume;
            if (levelIt->second <= 0) {
                mBidLevels.erase(levelIt);
            }
        } else {
            auto levelIt = mAskLevels.find(order.price);
            levelIt->second -= order.volume;
            if (levelIt->second <= 0) {
                mAskLevels.erase(levelIt);
            }
        }

        mOrders.erase(it);
    }

    std::pair<Price, Price> getBestPrices() const override {
        EXPECT(!mBidLevels.empty() && !mAskLevels.empty(), "empty levels");
        return {mBidLevels.begin()->first, mAskLevels.begin()->first};
    }

    void clear() override {
        mOrders.clear();
        mBidLevels.clear();
        mAskLevels.clear();
    }

private:
    std::unordered_map<OrderId, Order> mOrders;
    std::map<Price, Volume, std::greater<Price>> mBidLevels;
    std::map<Price, Volume, std::less<Price>> mAskLevels;
};

// Implementation 2: Vector-based order book with binary search
class OrderBookVector : public OrderBookBase {
public:
    void addOrder(OrderId orderId, Side side, Price price, Volume volume) override {
        auto [it, inserted] = mOrders.emplace(orderId, Order{side, price, volume});
        EXPECT(inserted, "duplicate order");

        auto& levels = (side == Side::Bid) ? mBidLevels : mAskLevels;

        if (side == Side::Bid) {
            auto levelIt = std::lower_bound(levels.begin(), levels.end(), price,
                [](const auto& lhs, Price p) { return lhs.first > p; });

            if (levelIt != levels.end() && levelIt->first == price) {
                levelIt->second += volume;
            } else {
                levels.insert(levelIt, {price, volume});
            }
        } else {
            auto levelIt = std::lower_bound(levels.begin(), levels.end(), price,
                [](const auto& lhs, Price p) { return lhs.first < p; });

            if (levelIt != levels.end() && levelIt->first == price) {
                levelIt->second += volume;
            } else {
                levels.insert(levelIt, {price, volume});
            }
        }
    }

    void modifyOrder(OrderId orderId, Volume newVolume) override {
        auto it = mOrders.find(orderId);
        EXPECT(it != mOrders.end(), "missing order");

        auto& order = it->second;
        Volume delta = newVolume - order.volume;
        order.volume = newVolume;

        auto& levels = (order.side == Side::Bid) ? mBidLevels : mAskLevels;

        if (order.side == Side::Bid) {
            auto levelIt = std::lower_bound(levels.begin(), levels.end(), order.price,
                [](const auto& lhs, Price p) { return lhs.first > p; });
            EXPECT(levelIt != levels.end() && levelIt->first == order.price, "price level not found");
            levelIt->second += delta;
        } else {
            auto levelIt = std::lower_bound(levels.begin(), levels.end(), order.price,
                [](const auto& lhs, Price p) { return lhs.first < p; });
            EXPECT(levelIt != levels.end() && levelIt->first == order.price, "price level not found");
            levelIt->second += delta;
        }
    }

    void deleteOrder(OrderId orderId) override {
        auto it = mOrders.find(orderId);
        EXPECT(it != mOrders.end(), "missing order");

        const auto& order = it->second;
        auto& levels = (order.side == Side::Bid) ? mBidLevels : mAskLevels;

        if (order.side == Side::Bid) {
            auto levelIt = std::lower_bound(levels.begin(), levels.end(), order.price,
                [](const auto& lhs, Price p) { return lhs.first > p; });
            EXPECT(levelIt != levels.end() && levelIt->first == order.price, "price level not found");
            levelIt->second -= order.volume;

            if (levelIt->second <= 0) {
                levels.erase(levelIt);
            }
        } else {
            auto levelIt = std::lower_bound(levels.begin(), levels.end(), order.price,
                [](const auto& lhs, Price p) { return lhs.first < p; });
            EXPECT(levelIt != levels.end() && levelIt->first == order.price, "price level not found");
            levelIt->second -= order.volume;

            if (levelIt->second <= 0) {
                levels.erase(levelIt);
            }
        }

        mOrders.erase(it);
    }

    std::pair<Price, Price> getBestPrices() const override {
        EXPECT(!mBidLevels.empty() && !mAskLevels.empty(), "empty levels");
        return {mBidLevels.front().first, mAskLevels.front().first};
    }

    void clear() override {
        mOrders.clear();
        mBidLevels.clear();
        mAskLevels.clear();
    }

private:
    std::unordered_map<OrderId, Order> mOrders;
    std::vector<std::pair<Price, Volume>> mBidLevels;  // sorted descending
    std::vector<std::pair<Price, Volume>> mAskLevels;  // sorted ascending
};

// Implementation 3: Reverse vector-based order book
class OrderBookReverseVector : public OrderBookBase {
public:
    void addOrder(OrderId orderId, Side side, Price price, Volume volume) override {
        auto [it, inserted] = mOrders.emplace(orderId, Order{side, price, volume});
        EXPECT(inserted, "duplicate order");

        auto& levels = (side == Side::Bid) ? mBidLevels : mAskLevels;

        if (side == Side::Bid) {
            auto levelIt = std::lower_bound(levels.begin(), levels.end(), price,
                [](const auto& lhs, Price p) { return lhs.first < p; });

            if (levelIt != levels.end() && levelIt->first == price) {
                levelIt->second += volume;
            } else {
                levels.insert(levelIt, {price, volume});
            }
        } else {
            auto levelIt = std::lower_bound(levels.begin(), levels.end(), price,
                [](const auto& lhs, Price p) { return lhs.first > p; });

            if (levelIt != levels.end() && levelIt->first == price) {
                levelIt->second += volume;
            } else {
                levels.insert(levelIt, {price, volume});
            }
        }
    }

    void modifyOrder(OrderId orderId, Volume newVolume) override {
        auto it = mOrders.find(orderId);
        EXPECT(it != mOrders.end(), "missing order");

        auto& order = it->second;
        Volume delta = newVolume - order.volume;
        order.volume = newVolume;

        auto& levels = (order.side == Side::Bid) ? mBidLevels : mAskLevels;

        if (order.side == Side::Bid) {
            auto levelIt = std::lower_bound(levels.begin(), levels.end(), order.price,
                [](const auto& lhs, Price p) { return lhs.first < p; });
            EXPECT(levelIt != levels.end() && levelIt->first == order.price, "price level not found");
            levelIt->second += delta;
        } else {
            auto levelIt = std::lower_bound(levels.begin(), levels.end(), order.price,
                [](const auto& lhs, Price p) { return lhs.first > p; });
            EXPECT(levelIt != levels.end() && levelIt->first == order.price, "price level not found");
            levelIt->second += delta;
        }
    }

    void deleteOrder(OrderId orderId) override {
        auto it = mOrders.find(orderId);
        EXPECT(it != mOrders.end(), "missing order");

        const auto& order = it->second;
        auto& levels = (order.side == Side::Bid) ? mBidLevels : mAskLevels;

        if (order.side == Side::Bid) {
            auto levelIt = std::lower_bound(levels.begin(), levels.end(), order.price,
                [](const auto& lhs, Price p) { return lhs.first < p; });
            EXPECT(levelIt != levels.end() && levelIt->first == order.price, "price level not found");
            levelIt->second -= order.volume;

            if (levelIt->second <= 0) {
                levels.erase(levelIt);
            }
        } else {
            auto levelIt = std::lower_bound(levels.begin(), levels.end(), order.price,
                [](const auto& lhs, Price p) { return lhs.first > p; });
            EXPECT(levelIt != levels.end() && levelIt->first == order.price, "price level not found");
            levelIt->second -= order.volume;

            if (levelIt->second <= 0) {
                levels.erase(levelIt);
            }
        }

        mOrders.erase(it);
    }

    std::pair<Price, Price> getBestPrices() const override {
        EXPECT(!mBidLevels.empty() && !mAskLevels.empty(), "empty levels");
        return {mBidLevels.back().first, mAskLevels.back().first};
    }

    void clear() override {
        mOrders.clear();
        mBidLevels.clear();
        mAskLevels.clear();
    }

private:
    std::unordered_map<OrderId, Order> mOrders;
    std::vector<std::pair<Price, Volume>> mBidLevels;  // sorted ascending (best price at end)
    std::vector<std::pair<Price, Volume>> mAskLevels;   // sorted descending (best price at end)
};

// Implementation 4: Linear search order book
class OrderBookLinear : public OrderBookBase {
public:
    void addOrder(OrderId orderId, Side side, Price price, Volume volume) override {
        auto [it, inserted] = mOrders.emplace(orderId, Order{side, price, volume});
        EXPECT(inserted, "duplicate order");

        auto& levels = (side == Side::Bid) ? mBidLevels : mAskLevels;
        if (side == Side::Bid) {
            // For bids (sorted descending), find first price <= target price
            auto levelIt = std::find_if(levels.begin(), levels.end(),
                [price](const auto& level) { return level.first <= price; });

            if (levelIt != levels.end() && levelIt->first == price) {
                // Found existing price level
                levelIt->second += volume;
            } else {
                // Insert at the found position (maintains sorted order)
                levels.insert(levelIt, {price, volume});
            }
        } else {
            // For asks (sorted ascending), find first price >= target price
            auto levelIt = std::find_if(levels.begin(), levels.end(),
                [price](const auto& level) { return level.first >= price; });

            if (levelIt != levels.end() && levelIt->first == price) {
                // Found existing price level
                levelIt->second += volume;
            } else {
                // Insert at the found position (maintains sorted order)
                levels.insert(levelIt, {price, volume});
            }
        }
    }

    void modifyOrder(OrderId orderId, Volume newVolume) override {
        auto it = mOrders.find(orderId);
        EXPECT(it != mOrders.end(), "missing order");

        auto& order = it->second;
        Volume delta = newVolume - order.volume;
        order.volume = newVolume;

        auto& levels = (order.side == Side::Bid) ? mBidLevels : mAskLevels;
        auto levelIt = std::find_if(levels.begin(), levels.end(),
            [price = order.price](const auto& level) { return level.first == price; });

        EXPECT(levelIt != levels.end(), "price level not found");
        levelIt->second += delta;
    }

    void deleteOrder(OrderId orderId) override {
        auto it = mOrders.find(orderId);
        EXPECT(it != mOrders.end(), "missing order");

        const auto& order = it->second;
        auto& levels = (order.side == Side::Bid) ? mBidLevels : mAskLevels;
        auto levelIt = std::find_if(levels.begin(), levels.end(),
            [price = order.price](const auto& level) { return level.first == price; });

        EXPECT(levelIt != levels.end(), "price level not found");
        levelIt->second -= order.volume;

        if (levelIt->second <= 0) {
            levels.erase(levelIt);
        }

        mOrders.erase(it);
    }

    std::pair<Price, Price> getBestPrices() const override {
        EXPECT(!mBidLevels.empty() && !mAskLevels.empty(), "empty levels");
        return {mBidLevels.front().first, mAskLevels.front().first};
    }

    void clear() override {
        mOrders.clear();
        mBidLevels.clear();
        mAskLevels.clear();
    }

private:
    std::unordered_map<OrderId, Order> mOrders;
    std::vector<std::pair<Price, Volume>> mBidLevels;  // sorted descending
    std::vector<std::pair<Price, Volume>> mAskLevels;  // sorted ascending
};

// Benchmark function
template <typename OrderBookT>
void benchmarkOrderBook(const std::vector<std::tuple<OrderId, Side, Price, Volume>>& orders,
                       const std::vector<std::pair<OrderId, Volume>>& modifications,
                       const std::vector<OrderId>& deletions,
                       const std::string& name) {
    OrderBookT book;

    // Warm up
    for (const auto& [id, side, price, vol] : orders) {
        book.addOrder(id, side, price, vol);
    }
    book.clear();

    // Benchmark add orders
    auto start = std::chrono::high_resolution_clock::now();
    for (const auto& [id, side, price, vol] : orders) {
        book.addOrder(id, side, price, vol);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto addTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    // Benchmark modify orders
    start = std::chrono::high_resolution_clock::now();
    for (const auto& [id, vol] : modifications) {
        book.modifyOrder(id, vol);
    }
    end = std::chrono::high_resolution_clock::now();
    auto modifyTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    // Benchmark delete orders
    start = std::chrono::high_resolution_clock::now();
    for (const auto& id : deletions) {
        book.deleteOrder(id);
    }
    end = std::chrono::high_resolution_clock::now();
    auto deleteTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    // Benchmark get best prices
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        volatile auto best = book.getBestPrices(); // volatile to prevent optimization
        (void)best;
    }
    end = std::chrono::high_resolution_clock::now();
    auto bestPriceTime = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / 1000.0;

    std::cout << "Benchmark results for " << name << ":\n";
    std::cout << "  Add " << orders.size() << " orders: " << addTime << " μs\n";
    std::cout << "  Modify " << modifications.size() << " orders: " << modifyTime << " μs\n";
    std::cout << "  Delete " << deletions.size() << " orders: " << deleteTime << " μs\n";
    std::cout << "  Get best prices (avg): " << bestPriceTime << " ns\n";
    std::cout << std::endl;
}

// Generate test data
void generateTestData(size_t numOrders,
                     std::vector<std::tuple<OrderId, Side, Price, Volume>>& orders,
                     std::vector<std::pair<OrderId, Volume>>& modifications,
                     std::vector<OrderId>& deletions) {
    orders.clear();
    modifications.clear();
    deletions.clear();

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<Price> priceDist(1000, 2000);
    std::uniform_int_distribution<Volume> volumeDist(1, 100);
    std::bernoulli_distribution sideDist(0.5);

    // Generate orders
    for (OrderId id = 1; id <= numOrders; ++id) {
        Side side = sideDist(gen) ? Side::Bid : Side::Ask;
        Price price = priceDist(gen);
        Volume volume = volumeDist(gen);
        orders.emplace_back(id, side, price, volume);
    }

    // Generate modifications (modify half of the orders)
    for (OrderId id = 1; id <= numOrders/2; ++id) {
        Volume newVolume = volumeDist(gen);
        modifications.emplace_back(id, newVolume);
    }

    // Generate deletions (delete 1/3 of the orders)
    for (OrderId id = 1; id <= numOrders/3; ++id) {
        deletions.push_back(id);
    }
}
