#include "../include/SingletonCounter.h"
#include <atomic>
#include <mutex>

// class SingletonCounter {
// private:
//     std::atomic<int> nextId;
//     std::atomic<int> nextReceipt;

// Constructor is private to prevent direct instantiation
SingletonCounter::SingletonCounter(): nextId(0), nextReceipt(0) {}

// public:
    // Delete copy constructor and assignment operator to enforce singleton behavior
    // SingletonCounter(const SingletonCounter&) = delete;
    // SingletonCounter& operator=(const SingletonCounter&) = delete;

    // Static method to access the single instance
SingletonCounter& SingletonCounter::getInstance() {
    static SingletonCounter instance;
    return instance;
}

// Methods to get the next ID and Receipt
int SingletonCounter::getNextId() {
    return nextId.fetch_add(1);
}

int SingletonCounter::getNextReceipt() {
    return nextReceipt.fetch_add(1);
}
// };