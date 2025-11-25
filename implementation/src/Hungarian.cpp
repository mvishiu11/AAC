#include "../include/Hungarian.hpp"
#include <algorithm>

bool Assignment::operator<(Assignment const& o) const { return cost > o.cost; }
bool Node::operator<(Node const& o) const { return mapping_cost > o.mapping_cost; }

// Based on:
// https://cyberlab.engr.uconn.edu/wp-content/uploads/sites/2576/2018/09/Lecture_8.pdf
// (Actually, it's the JVC algorithm which is in a sense a variant of the hungarian method)
Assignment hungarian(const CostMatrix& a) {
    int n = a.size(); 
    if (n==0) {
        return {{},0};
    }
    int m = a[0].size();
    int N = std::max(n,m);
    // For rectangular matrix (|V(G)|<|V(H)|), we pad it with INFs
    CostMatrix cost(N, std::vector<Cost>(N, 0));
    for (int row=0;row<N;row++)
        for (int col=0;col<N;col++) 
            cost[row][col] = (row<n && col<m) ? a[row][col] : INF; // Padding

    // Potentials of each column
    std::vector<Cost> u(N+1), v(N+1);

    // mapped[i] = k => ith column matched with kth row
    std::vector<int> mapped(N+1);
    // way[i] = k => kth column currently precedes ith column 
    std::vector<int> way(N+1);
    // Final mapping
    std::vector<int> answer(N, -1);
    for (int row=1;row<=N;row++) {
        mapped[0] = row;
        int col0 = 0;
        // minv[i] = x => current minimal cost to reach ith column
        std::vector<double> minv(N+1, INF);
        // used[i] = true => column already mapped
        std::vector<bool> used(N+1, false);

        // Repeat until the column is mapped
        do {
            used[col0] = true;
            int row0 = mapped[col0], col1 = 0;
            // Minimum cost to map another column
            double delta = INF;

            // For every reduced column
            for (int col=1;col<=N;col++) {
                if (!used[col]) {
                    // Reduce cost
                    double cur = cost[row0-1][col-1] - u[row0] - v[col];
                    // If minimum, remember it and its path
                    if (cur < minv[col]) { 
                        minv[col] = cur; way[col] = col0; 
                    }
                    // Update delta if we found a smaller reduction
                    if (minv[col] < delta) { 
                        delta = minv[col]; col1 = col; 
                    }
                }
            }
            // Update potentials of each column
            for (int col=0;col<=N;col++) {
                if (used[col]) { 
                    u[mapped[col]] += delta; v[col] -= delta; 
                }
                else minv[col] -= delta;
            }
            col0 = col1;
        } while (mapped[col0] != 0);
        // Reconstruct the path 
        do {
            int col1 = way[col0];
            mapped[col0] = mapped[col1];
            col0 = col1;
        } while (col0);
    }
    
    // Get the mapped values
    for (int j=1;j<=N;j++) {
        if (mapped[j] <= N) {
            answer[mapped[j]-1] = j-1;
        }
    }
    // Calculate total cost
    double total = 0;
    Mapping mapping(n, -1);
    for (int i=0;i<n;i++) {
        if (answer[i] < m) {
            mapping[i] = answer[i];
            total += a[i][answer[i]];
        } else mapping[i] = -1;
    }
    return {mapping, total};
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
