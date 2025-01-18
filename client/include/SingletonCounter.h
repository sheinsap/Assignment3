#ifndef SINGLETON_COUNTER_H
#define SINGLETON_COUNTER_H

#include <atomic>
#include <mutex>

class SingletonCounter {
private:
    std::atomic<int> nextId;        // Counter for unique IDs
    std::atomic<int> nextReceipt;   // Counter for unique receipts

    // Private constructor to prevent direct instantiation
    SingletonCounter();

public:
    // Delete copy constructor and assignment operator to enforce singleton behavior
    SingletonCounter(const SingletonCounter&) = delete;
    SingletonCounter& operator=(const SingletonCounter&) = delete;

    // Static method to access the single instance
    static SingletonCounter& getInstance();

    // Methods to get the next ID and Receipt
    int getNextId();
    int getNextReceipt();
};

#endif // SINGLETON_COUNTER_H