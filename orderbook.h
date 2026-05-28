#pragma once

#include <list>
#include <map>
#include <unordered_map>
#include <optional>
#include <algorithm>
#include <tuple>
#include <functional>
#include <assert.h>
#include <chrono>
#include <vector>
#include <iostream>
#include "packet.h"

enum Side {BUY, SELL};
enum OrderType { LIMIT, MARKET };

struct Order{
    int id;
    Side side;
    std::optional<int> price; // nullopt for market orders
    int qty;
    OrderType type;
};  

class OrderBook {
    //public:


        // struct ExecutionReport {
        //     int aggressorId;    // incoming order
        //     int restingId;      // order that was sitting in book
        //     int price;
        //     int tradedQty;
        //     std::chrono::steady_clock::time_point timestamp;
        // };

    private:
        // Linked list implementation -- bad for cache
        using OrderList = std::list<Order>;
        // BUY: highest price first
        std::map<int, OrderList, std::greater<int>> buyBook;
        // SELL: lowest price first
        std::map<int, OrderList> sellBook;
        // order_id -> (side, price, iterator into list)
        std::unordered_map<int, std::tuple<Side, int, OrderList::iterator>> orderIndex;

    public:
        OrderBook() = default;
        ~OrderBook() = default;

        OrderBook(OrderBook&& other) noexcept;
        OrderBook& operator=(OrderBook&& other) noexcept;

        OrderBook(const OrderBook&) = delete;
        OrderBook& operator=(const OrderBook&) = delete;

        void addOrder(const int id, const Side side, const std::optional<int> price, 
            const int qty, const OrderType type);
  
        void cancelOrder(const int id);
        void clear();
        // optional to replace sentinel values
        // forces caller to check if value exists
        std::optional<int> bestBid() const;
        std::optional<int> bestAsk() const;
        std::vector<OrderMsg> getAllOrders() const;

    private:
        void matchOrder(Order& incoming);

        void matchBuy(Order& incoming);

        void matchSell(Order& incoming);

        template<typename Book>
        void trade(Order& incoming, Book& book) {
            auto levelIt = book.begin();     // iterator to first element in buy or sell book
            auto& orders = levelIt->second;  // iterator to orders linked list at price level
            auto restingIt = orders.begin(); // iterator to first order in list

            const int traded = std::min(incoming.qty, restingIt->qty); 
            incoming.qty -= traded;
            restingIt->qty -= traded;

            std::cout << "TRADE aggressor= " << incoming.id
                      << " resting =" << restingIt->id
                      << " price=" << levelIt->first
                      << " qty=" << traded << "\n";
                      
            // erase empty order (no qty)
            if (restingIt->qty == 0) {
                orderIndex.erase(restingIt->id);
                orders.erase(restingIt);
            }

            // erase empty price level
            if(orders.empty()) book.erase(levelIt);
        }

        template<typename Book>
        const std::vector<OrderMsg> getSnapshot(Book& book) const {
            std::vector<OrderMsg> all_orders;

            for (auto& levelIt : book) {
                for (auto& order : levelIt.second) {
                    OrderMsg o;
                    o.price = order.price.value_or(0);
                    o.qty = order.qty;
                    o.order_type = order.type == LIMIT ? 'L' : 'M';
                    o.side = order.side == BUY ? 'B' : 'S';
                    o.order_id = order.id;
                    all_orders.push_back(o);
                }
            }
            return all_orders;
        }

        
};
