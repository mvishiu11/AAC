#ifndef H_HUNGARIAN
#define H_HUNGARIAN
#include <bits/stdc++.h>
#include <limits>
#include <ostream>
#include <vector>

using Cost = double;
using CostMatrix = std::vector<std::vector<Cost>>;
using Mapping = std::vector<int>;

const double INF = std::numeric_limits<double>::max();

struct Assignment {
    Mapping mapping; // -1 for unassigned
    Cost cost;
    bool operator<(Assignment const& o) const; // for min-heap with greater
};

// Hungarian method for rectangular cost matrix assignment cost minimization.
// size = rows x cols
Assignment hungarian(const std::vector<std::vector<double>>& a);

// Murty: partition node representation
struct Node {
    // size = rows
    // -1 means free; otherwise fixed column
    std::vector<int> fixed; 

    // size = rows*cols
    // index[row][col] = row*cols+col
    // denotes which mappings had already been chosen
    std::vector<bool> banned; 

    Cost mapping_cost;
    Mapping mapping; // assignment for free rows (size rows)
    bool operator<(Node const& o) const; // for min-heap with
};

std::vector<Assignment> murty(const CostMatrix& costMat, int K);

std::ostream& operator<<(std::ostream&, const Mapping&);
#endif