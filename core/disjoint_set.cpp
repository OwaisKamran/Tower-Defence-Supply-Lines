#include "disjoint_set.hpp"
#include <utility>

DisjointSet::DisjointSet(int n) : parent(n), rank(n, 0) {
    for (int i = 0; i < n; i++)
        parent[i] = i;
}

int DisjointSet::find(int x) {
    steps++;
    if (parent[x] != x)
        parent[x] = find(parent[x]);
    return parent[x];
}

bool DisjointSet::unite(int a, int b) {
    int rootA = find(a);
    int rootB = find(b);
    if (rootA == rootB) return false;

    if (rank[rootA] < rank[rootB]) std::swap(rootA, rootB);
    parent[rootB] = rootA;
    if (rank[rootA] == rank[rootB]) rank[rootA]++;
    return true;
}
