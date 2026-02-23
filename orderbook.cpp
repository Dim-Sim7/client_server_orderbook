#pragma once

#include "orderbook.h"



OrderBook::OrderBook() {}
OrderBook::~OrderBook() {}

OrderBook::OrderBook(OrderBook&& other) : 
            buyBook(std::move(other.buyBook)),
            sellBook(std::move(other.sellBook)),
            orderIndex(std::move(other.orderIndex))        
{}

OrderBook& OrderBook::operator=(OrderBook&& other) {
    if (this != &other) {
        buyBook    = std::move(other.buyBook);
        sellBook   = std::move(other.sellBook);
        orderIndex = std::move(other.orderIndex);
    }

    return *this;
}



void OrderBook::addOrder(const int id, const Side side, const std::optional<int> price, const int qty, const OrderType type) {
    Order incoming{id, side, price, qty, type};
    matchOrder(incoming);
    if (incoming.qty == 0) return;
    if (type == MARKET) return; // fill or kill
    assert(incoming.price.has_value());

    auto insert = [&](auto& book) {          // only limit orders are inserted
        auto& level = book[price.value()];   // order(type) linked list at price
        level.push_back(incoming);           // insert incoming order
        const auto it = std::prev(level.end());   // iterator to newly inserted element

        // add to orderIndex
        // defines what side, price level and position in list an order id is for O(1) access
        orderIndex[id] = {side, price.value(), it}; 
    };
    if (side == BUY) insert(buyBook);
    else             insert(sellBook);
}

void OrderBook::cancelOrder(const int id) {
    // iterator to orderIndex
    const auto it = orderIndex.find(id);
    if (it == orderIndex.end()) return;
    
    // extract tuple values
    auto [side, price, orderIt] = it->second;

    // lambda to remove order from orderBook
    auto remove = [&](auto& book) {
        // level linked list
        auto& level = book[price];
        // remove order from level using interator
        level.erase(orderIt);
        // remove empty price level
        if (level.empty())   book.erase(price);
        orderIndex.erase(it);
    };
    
    if (side == BUY) remove(buyBook);
    else             remove(sellBook);
}

// optional to replace sentinel values
// forces caller to check if value exists
std::optional<int> OrderBook::bestBid() const {
    if (buyBook.empty()) return std::nullopt;
    return buyBook.begin()->first;
}

std::optional<int> OrderBook::bestAsk() const {
    if (sellBook.empty()) return std::nullopt;
    return sellBook.begin()->first;
}

void OrderBook::matchOrder(Order& incoming) {
    if (incoming.side == BUY) {
        matchBuy(incoming);
    } else {
        matchSell(incoming);
    }
}

void OrderBook::matchBuy(Order& incoming) {
    while (incoming.qty > 0 && !sellBook.empty()) {
        // if sell price is greater than incoming buy price and is limit order
        if (incoming.type == LIMIT && sellBook.begin()->first > incoming.price.value()) break;
        trade(incoming, sellBook);
    }
}

void OrderBook::matchSell(Order& incoming) {
    while (incoming.qty > 0 && !buyBook.empty()) {
        // if buy price is less than incoming sell price and is limit order
        if (incoming.type == LIMIT && buyBook.begin()->first < incoming.price.value()) break;
        trade(incoming, buyBook);
    }
}

//returns flat vector of current orderbook
std::vector<SnapshotOrder> OrderBook::getAllOrders() const {
    auto all_orders = getSnapshot(buyBook); //compiler infers template type
    auto sell_orders = getSnapshot(sellBook);

    // merge sell into all_orders
    all_orders.insert(all_orders.end(), sell_orders.begin(), sell_orders.end());

    return all_orders;
}
