#include "min_heap.hpp"
#include <algorithm>

bool MinHeap::comesFirst(const HeapEntry& a, const HeapEntry& b) const {
    if (a.fCost != b.fCost) return a.fCost < b.fCost;
    return a.hCost < b.hCost;
}

void MinHeap::heapifyUp(int index) {
    while (index > 0) {
        int parent = (index - 1) / 2;
        if (comesFirst(heap[index], heap[parent])) {
            std::swap(heap[index], heap[parent]);
            index = parent;
        } else break;
    }
}

void MinHeap::heapifyDown(int index) {
    int size = (int)heap.size();
    while (true) {
        int left = 2 * index + 1;
        int right = 2 * index + 2;
        int smallest = index;

        if (left < size && comesFirst(heap[left], heap[smallest])) smallest = left;
        if (right < size && comesFirst(heap[right], heap[smallest])) smallest = right;

        if (smallest != index) {
            std::swap(heap[index], heap[smallest]);
            index = smallest;
        } else break;
    }
}

void MinHeap::push(Node* node) {
    heap.push_back({node->getFCost(), node->hCost, node});
    heapifyUp((int)heap.size() - 1);
}

Node* MinHeap::pop() {
    if (heap.empty()) return nullptr;

    Node* top = heap.front().node;
    heap[0] = heap.back();
    heap.pop_back();
    if (!heap.empty()) heapifyDown(0);

    return top;
}

bool MinHeap::empty() const {
    return heap.empty();
}

void MinHeap::clear() {
    heap.clear();
}
