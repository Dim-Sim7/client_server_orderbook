# CS 3410: Data Structures and Algorithms
## Programming Assignment 4: Limit Order Book Implementation

**Assigned:** Week 8  
**Due Date:** Week 11, Friday 11:59 PM  
**Points:** 100  
**Submission:** Via course portal (GitHub Classroom repository)

---

## Learning Objectives

Upon completion of this assignment, you will be able to:
- Implement and manipulate advanced data structures for real-time applications
- Design efficient algorithms for order matching in financial systems
- Analyze time complexity of operations in a performance-critical system
- Apply object-oriented programming principles to model complex domain logic
- Write robust, well-tested C++ code following modern best practices

---

## Background

A **limit order book** is a fundamental data structure used in electronic trading systems to match buy and sell orders for financial instruments (stocks, cryptocurrencies, commodities, etc.). Understanding and implementing an order book provides excellent practice with data structures, algorithm design, and performance optimization.

### Order Types

1. **Limit Orders**: Orders to buy or sell at a specific price or better
   - Buy orders execute at the limit price or lower
   - Sell orders execute at the limit price or higher
   - Unmatched orders remain in the book until filled or cancelled

2. **Market Orders** (Optional Extension): Orders to buy or sell immediately at the best available price

### Order Book Structure

The order book maintains two sides:
- **Bid side**: Buy orders, sorted by price (highest first) and time (earliest first for same price)
- **Ask side**: Sell orders, sorted by price (lowest first) and time (earliest first for same price)

### Order Matching Rules

Orders are matched using **price-time priority**:
1. Best price has priority (highest bid, lowest ask)
2. For orders at the same price, earlier orders have priority (FIFO)

---

## Assignment Requirements

### Part 1: Core Implementation (70 points)

Implement a `LimitOrderBook` class that supports the following operations:

#### Required Data Structures
You must design appropriate data structures to efficiently support all operations. Consider:
- How to quickly find the best bid and best ask
- How to efficiently match incoming orders
- How to maintain price-time priority
- How to quickly cancel specific orders

#### Required Methods

```cpp
class Order {
public:
    enum class Side { BUY, SELL };
    enum class Type { LIMIT, MARKET };
    
    uint64_t orderId;
    Side side;
    Type type;
    double price;      // 0 for market orders
    uint32_t quantity;
    uint64_t timestamp; // For time priority
    
    // Add constructor and any helper methods
};

class LimitOrderBook {
public:
    // Add a new limit order to the book
    // Returns: vector of trades executed (may be empty)
    std::vector<Trade> addOrder(const Order& order);
    
    // Cancel an existing order
    // Returns: true if order was found and cancelled, false otherwise
    bool cancelOrder(uint64_t orderId);
    
    // Get the best bid price (highest buy price)
    // Returns: best bid price, or 0.0 if no bids exist
    double getBestBid() const;
    
    // Get the best ask price (lowest sell price)
    // Returns: best ask price, or 0.0 if no asks exist
    double getBestAsk() const;
    
    // Get total volume at a specific price level
    uint32_t getVolumeAtPrice(Order::Side side, double price) const;
    
    // Print the order book state (for debugging)
    void printBook(int depth = 5) const;
    
private:
    // Design your internal data structures here
};

struct Trade {
    uint64_t buyOrderId;
    uint64_t sellOrderId;
    double price;
    uint32_t quantity;
    uint64_t timestamp;
};
```

#### Matching Algorithm (25 points)

When a new order arrives:
1. Check if it can match with existing orders on the opposite side
2. Execute trades in price-time priority order
3. If the order is partially filled, add the remaining quantity to the book
4. If fully filled, do not add to the book
5. Return all executed trades

**Example:**
```
Order Book State:
BIDS:              ASKS:
100 @ $99.50      100 @ $100.50
200 @ $99.00      150 @ $101.00

New Order: SELL 250 @ $99.00

Execution:
- Match 100 @ $99.50 (best bid)
- Match 150 @ $99.00 (next best bid)
- Add remaining 0 to book (fully filled)

Resulting Book:
BIDS:              ASKS:
50 @ $99.00       100 @ $100.50
                  150 @ $101.00
```

#### Performance Requirements (15 points)

Your implementation should achieve the following time complexities:
- `addOrder()`: O(log n + m) where n is the number of price levels and m is the number of orders matched
- `cancelOrder()`: O(log n) where n is the number of price levels
- `getBestBid()` / `getBestAsk()`: O(1)
- `getVolumeAtPrice()`: O(log n) where n is the number of price levels

You must document the actual time complexity of your implementation in comments.

#### Code Quality (15 points)
- Proper use of C++ features (const correctness, RAII, move semantics where appropriate)
- Clear class design and encapsulation
- Meaningful variable and function names
- Comprehensive comments explaining complex logic
- No memory leaks (use smart pointers or proper manual memory management)

#### Correctness (15 points)
- All methods work correctly for edge cases
- Order matching follows price-time priority correctly
- Thread-safety is not required, but your code should be deterministic

---

### Part 2: Testing (20 points)

Write comprehensive unit tests using a testing framework of your choice (Google Test recommended, but Catch2 or others are acceptable).

Your test suite must include:

1. **Basic Operations Tests** (8 points)
   - Adding orders to an empty book
   - Cancelling orders
   - Querying best bid/ask
   - Volume queries

2. **Matching Logic Tests** (8 points)
   - Full order fills
   - Partial order fills
   - Multiple orders at same price level (time priority)
   - Orders that don't match (added to book)
   - Price improvement scenarios

3. **Edge Cases** (4 points)
   - Empty order book operations
   - Cancelling non-existent orders
   - Orders with zero quantity (should be rejected)
   - Very large order quantities
   - Minimum price tick considerations

---

### Part 3: Analysis and Documentation (10 points)

Submit a written report (PDF format, 2-4 pages) that includes:

1. **Design Decisions** (4 points)
   - Explanation of your data structure choices
   - Trade-offs you considered
   - Why your design meets the performance requirements

2. **Complexity Analysis** (3 points)
   - Detailed time and space complexity analysis for each operation
   - Justification with reference to your implementation

3. **Testing Strategy** (2 points)
   - Overview of your testing approach
   - Any interesting bugs you discovered during testing

4. **Reflection** (1 point)
   - What you learned
   - What you would improve given more time

---

## Bonus Extensions (Optional, +15 points max)

Choose one or more extensions to implement for extra credit:

1. **Market Orders** (+5 points)
   - Implement market order support
   - Market orders execute immediately at best available price(s)

2. **Order Book Snapshot** (+5 points)
   - Implement serialization/deserialization of the order book state
   - Write to/read from a file or binary format

3. **Performance Benchmarking** (+5 points)
   - Create a realistic load testing scenario
   - Measure and report throughput (orders/second)
   - Compare performance with different data structure choices

4. **Order Modification** (+5 points)
   - Implement order modification (price or quantity changes)
   - Maintain correct priority (modification loses time priority)

5. **Stop Orders** (+10 points)
   - Implement stop-loss and stop-limit orders
   - Trigger based on market price movements

---

## Submission Requirements

Submit via your GitHub Classroom repository:

```
assignment4-orderbook/
├── src/
│   ├── Order.h
│   ├── Order.cpp
│   ├── LimitOrderBook.h
│   ├── LimitOrderBook.cpp
│   └── main.cpp (demonstration program)
├── tests/
│   └── orderbook_tests.cpp
├── docs/
│   └── report.pdf
├── CMakeLists.txt or Makefile
└── README.md (build and run instructions)
```

### Compilation Requirements
- Code must compile with C++17 or later
- Must compile with: `g++ -std=c++17 -Wall -Wextra -O2`
- No compiler warnings allowed
- Include a build script or CMake configuration

---

## Grading Rubric

| Component | Points | Criteria |
|-----------|--------|----------|
| Matching Algorithm | 25 | Correct implementation of price-time priority |
| Performance | 15 | Meets time complexity requirements |
| Code Quality | 15 | Clean, well-documented, modern C++ |
| Correctness | 15 | All operations work correctly |
| Testing | 20 | Comprehensive test coverage |
| Documentation | 10 | Clear design explanations and analysis |
| **Total** | **100** | |
| Bonus | +15 | Optional extensions |

---

## Getting Started

### Recommended Approach

1. **Week 8**: Design your data structures on paper, consider different options
2. **Week 9**: Implement the Order class and basic OrderBook structure
3. **Week 9-10**: Implement order matching algorithm
4. **Week 10**: Write comprehensive tests
5. **Week 11**: Write documentation, polish code, submit

### Suggested Data Structures

Consider using combinations of:
- `std::map` or `std::multimap` for price levels
- `std::queue` or `std::list` for FIFO ordering at each price
- `std::unordered_map` for fast order lookup by ID
- Custom structures for optimal performance

### Tips for Success

- Start simple: get basic add/cancel working before tackling matching
- Write tests early and run them frequently
- Use a debugger and print statements to visualize the book state
- Consider drawing diagrams of your data structures
- Don't optimize prematurely; correctness first, performance second

---

## Academic Integrity

This is an individual assignment. You may:
- Discuss high-level design concepts with classmates
- Use online resources for C++ syntax and STL documentation
- Ask teaching assistants for clarification

You may NOT:
- Share code with other students
- Copy implementations from online sources
- Use AI tools to generate significant portions of your code

Any violations will result in a zero for the assignment and be reported to the department.

---

## Resources

- [C++ Reference](https://en.cppreference.com/)
- [Google Test Documentation](https://google.github.io/googletest/)
- [How Order Books Work](https://www.investopedia.com/terms/o/order-book.asp)
- Course textbook: Chapter 14 (Priority Queues), Chapter 18 (Hash Tables)

---

## Questions?

- Office Hours: Tuesday/Thursday 2-4 PM, CS Building Room 301
- Discussion Forum: Use the "Assignment 4" category
- Email: Please use subject line "[CS3410-A4] Your Question"

Good luck, and happy coding!

---

*Last updated: Week 8, Spring 2025*
