
#include "orderbook.h"

OrderBook::OrderBook(OrderBook&& other) noexcept : 
            buyBook(std::move(other.buyBook)),
            sellBook(std::move(other.sellBook)),
            orderIndex(std::move(other.orderIndex))        
{}

OrderBook& OrderBook::operator=(OrderBook&& other) noexcept {
    if (this != &other) {
        buyBook    = std::move(other.buyBook);
        sellBook   = std::move(other.sellBook);
        orderIndex = std::move(other.orderIndex);
    }
    return *this;
}

void OrderBook::addOrder(const uint64_t id, const Side side, const std::optional<int32_t> price, 
                const uint32_t qty, const OrderType type) {
    Order incoming{id, qty, price, side,  type};
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

void OrderBook::cancelOrder(const uint64_t id) {
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

void OrderBook::clear() {
    orderIndex.clear();
    buyBook.clear();
    sellBook.clear();
}

void OrderBook::print() {

    std::cout << "\033[2J\033[H";
    std::cout << "================ ORDER BOOK ================\n\n";
    std::cout << "BUY BOOK"
              << std::setw(35) << " "
              << "SELL BOOK\n";
    std::cout << "------------------------------------------------------------\n";

    auto buyIt = buyBook.begin();
    auto sellIt = sellBook.begin();

    while (buyIt != buyBook.end() || sellIt != sellBook.end()) {
        std::stringstream buySS;
        std::stringstream sellSS;
        // BUY SIDE
        if (buyIt != buyBook.end()) {
            buySS << std::left
                << std::setw(8)
                << buyIt->first;

            for (const auto& order : buyIt->second) {
                buySS << "[" << order.id << ":" << order.qty << "] ";
            }
            ++buyIt;
        }
        // ---- SELL SIDE ----
        if (sellIt != sellBook.end()) {

            sellSS << std::left
                << std::setw(8)
                << sellIt->first;

            for (const auto& order : sellIt->second) {
                sellSS << "[" << order.id << ":" << order.qty << "] ";
            }

            ++sellIt;
        }

        // fixed-width left column
        std::cout << std::left << std::setw(35) << buySS.str() << sellSS.str() << "\n";
    }

    // PRINT TRADES ONCE
    std::cout << "\n================ TRADES ================\n";
    for (const auto& t : tradeLog_) {
        std::cout << t;
    }

    std::cout.flush();
}

// optional<> to replace sentinel values
// forces caller to check if value exists
// returns either nullopt or the bestBid price
std::optional<int32_t> OrderBook::bestBid() const {
    if (buyBook.empty()) return std::nullopt;
    return buyBook.begin()->first;
}

std::optional<int32_t> OrderBook::bestAsk() const {
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

