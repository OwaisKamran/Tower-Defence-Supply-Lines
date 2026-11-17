#pragma once
#include <vector>

class DisjointSet {
public:
    explicit DisjointSet(int n);

    int find(int x);
    bool unite(int a, int b);

    long long getSteps() const { return steps; }

private:
    std::vector<int> parent;
    std::vector<int> rank;
    long long steps = 0;
};
