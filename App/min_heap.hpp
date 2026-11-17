#pragma once
#include <vector>
#include "node.hpp"

struct HeapEntry {
    float fCost;
    float hCost;
    Node* node;
};

class MinHeap {
private:
    std::vector<HeapEntry> heap;

    bool comesFirst(const HeapEntry& a, const HeapEntry& b) const;
    void heapifyUp(int index);
    void heapifyDown(int index);

public:
    void push(Node* node);
    Node* pop();
    bool empty() const;
    void clear();
};
