#include "../include/Hungarian.hpp"
#include <algorithm>
#include <queue>
#include <cassert>

bool Assignment::operator<(Assignment const& o) const { return cost > o.cost; }
bool Node::operator<(Node const& o) const { return mapping_cost > o.mapping_cost; }

constexpr bool ckmin(Cost& a, const Cost& b) { 
    return b < a ? a = b, true : false; 
}

Assignment hungarian(const CostMatrix& C) {
    const int M = static_cast<int>(C.size());
    const int N = static_cast<int>(C[0].size());
    const int N1 = N+1;
    assert(M <= N);

    Rows y(N1, NoRow);
    Costs potential(N);
    Cost current = 0;

    for (int i = 0; i < M; ++i) {
        int j = N;
        y[j] = i;
        Costs dist(N1, INF);
        dist[N] = 0;
        std::vector<bool> vis(N + 1);
        Columns pred(N1, -1);

        while (y[j] != NoRow) {
            Cost min_dist = INF;
            vis[j] = true;
            int j_next = NoColumn;

            for (int j1 = 0; j1 < N; ++j1) {
                if (!vis[j1]) {
                    Cost edge = C[y[j]][j1] - potential[j1];
                    if (j != N) {
                        edge -= C[y[j]][j] - potential[j];
                        assert(edge >= 0);
                    }
                    if (dist[j] + edge < dist[j1]) {
                        dist[j1] = dist[j] + edge;
                        pred[j1] = j;
                    }
                    if (dist[j1]<min_dist) {
                        min_dist = dist[j1];
                        j_next = j1;
                    }
                }
            }
            j = j_next;
        }
        for (int w = 0; w < N; ++w) {
            ckmin(dist[w], dist[j]);
            potential[w] += dist[w];
        }
        current += potential[j];

        for (int w; j != N; j = w) 
            y[j] = y[w = pred[j]];
    }

    std::vector<int> jobMapping(M);
    for (int w = 0; w < N; ++w) {
        if (y[w] != NoRow) {
            jobMapping[y[w]] = w;
        }
    }

    return { jobMapping, current };
}


std::vector<Assignment> murty(const std::vector<std::vector<double>>& costMat, int K) {
    int rows = costMat.size();
    int cols = (rows>0 ? costMat[0].size() : 0);
    // Get initial mapping
    auto [mapping0, cost0] = hungarian(costMat);

    // Priority queue based on the mapping costs
    std::priority_queue<Node> pq;
    Node root;
    // fixed[i] = -1 => ith column is currently unmapped
    // fixed[i] = k > -1 => ith column currently has a fixed mapping to kth row
    root.fixed.assign(rows, -1);
    // Initially, each mapping is allowed/NOT banned
    root.banned.assign(rows*cols, false);
    // Current mapping
    root.mapping = mapping0;
    // Current mapping's cost
    root.mapping_cost = cost0;

    pq.push(root);

    // Result accumulator
    std::vector<Assignment> results;
    
    // While we can and want to add more results 
    while (!pq.empty() && (int)results.size() < K) {
        // Fetch the current best mapping node
        Node node = pq.top(); 
        pq.pop();
        // Convert it to an assignment
        Assignment a;
        a.mapping = node.mapping;
        a.cost = node.mapping_cost;
        results.push_back(a);
        
        // For each row 
        for (int row=0;row<rows;row++) {
            // check if row is already fixed
            if (node.fixed[row] != -1) {
                continue;
            }

            // Find the column mapped to the row in order to ban it
            int colToBan = node.mapping[row];
            // Unless the row is unassigned - then we skip it
            if (colToBan == -1) {
                continue;
            } 

            // Create a child node
            Node child = node;
            // Fix the mapping for all previous rows in child
            for (int r=0;r<row;r++) {
                if (child.fixed[r]==-1) {
                    child.fixed[r] = node.mapping[r];
                }
            }
            // And ban the current mapping
            child.banned[row*cols + colToBan] = true;

            // Modify cost matrix
            std::vector<std::vector<double>> mod = costMat;
            // For banned mappings, set their cost to infinity
            for (int r=0;r<rows;r++) {
                for (int c=0;c<cols;c++) {
                    if (child.banned[r*cols + c]) mod[r][c] = INF/4;
                }
            }
            // And for fixed mappings, set all OTHER costs to infinity to force them
            for (int r=0;r<rows;r++) {
                if (child.fixed[r] != -1) {
                    for (int c=0;c<cols;c++) {
                        if (c != child.fixed[r]) {
                            mod[r][c] = INF/4;
                        }
                    }
                }
            }
            
            // Find the new assignment
            auto [mapping1, cost1] = hungarian(mod);
            if (cost1 >= INF/8) continue; // infeasible
            child.mapping = mapping1;
            child.mapping_cost = 0;
            // Compute true cost considering only valid assigned pairs
            for (int r=0;r<rows;r++) {
                if (mapping1[r] != -1) {
                    child.mapping_cost += costMat[r][mapping1[r]];
                }
            }
            // Push the child per row
            pq.push(child);
        }
    }
    // Return gathered results
    return results;
}

std::ostream& operator<<(std::ostream& os, const Mapping& m) {
    os << '[';
    for (std::size_t i = 0; i < m.size(); ++i) {
        os << m[i];
        if (i != m.size() - 1) {
            os << ", ";
        }
    }
    os << ']';
    return os;
}

std::ostream& operator<<(std::ostream& os, const Assignment& a) {
    os << a.mapping << " / " << a.cost;
    return os ;
}