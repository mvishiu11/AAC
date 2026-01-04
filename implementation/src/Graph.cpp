#include "../include/Graph.hpp"
#include "../include/Hungarian.hpp"
#include <iostream>
#include <algorithm>
#include <functional>
#include <set>
#include <vector>
#include <cmath>
#include <climits>

using namespace std;

int Graph::getSize() const
{
    int size = 0;
    for (int i = 0; i < nodes; i++)
    {
        for (int j = 0; j < nodes; j++)
        {
            if (edges[i][j] != 0)
            {
                size += edges[i][j];
            }
        }
    }
    return size + nodes;
}

bool Graph::hasNSubgraphs(Graph &G, int N, bool verbose)
{
    std::cout << "### N SUBGRAPH ISOMORPHISMS CHECK ###" << endl; 

    Graph H = *this;
    if (G.getVerticesCount() == 0 || G.getVerticesCount() > H.getVerticesCount())
    {
        return true;
    }
    int Hn = H.getVerticesCount();
    int Gn = G.getVerticesCount();

    int **M = (int **)malloc(sizeof(int *) * Hn);
    if (M==NULL) {
        std::cout << "memory allocation error\n";
        exit(1);
    }
    for (int i = 0; i < Hn; i++)
    {
        M[i] = (int *)malloc(sizeof(int) * Gn);
        if (M[i]==NULL) {
            std::cout << "memory allocation error\n";
            exit(1);
        }
    }

    for (int i = 0; i < Hn; i++)
    {
        for (int j = 0; j < Gn; j++)
        {

            if (H.getOutDegree(i) >= G.getOutDegree(j) && H.getInDegree(i) >= G.getInDegree(j))
            {
                M[i][j] = 1;
            }
            else
            {
                M[i][j] = 0;
            }
        }
    }
    // Iterative neighbourhood refinement (prune until fixpoint)
    bool changed = true;
    while (changed)
    {
        changed = false;
        for (int i = 0; i < Hn; ++i)
        {
            for (int j = 0; j < Gn; ++j)
            {
                if (M[i][j] == 0)
                    continue;

                // Check outgoing neighbours of pattern-vertex j
                bool ok_out = true;
                for (int u = 0; u < Gn; ++u)
                {
                    int mult_pattern = G.getMultiplicity(j, u); // multiplicity of (j->u) in G
                    if (mult_pattern <= 0)
                        continue;

                    bool found = false;
                    for (int v = 0; v < Hn; ++v)
                    {
                        int mult_host = H.getMultiplicity(i, v); // multiplicity of (i->v) in H
                        if (mult_host >= mult_pattern && M[v][u] == 1)
                        {
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                    {
                        ok_out = false;
                        break;
                    }
                }

                if (!ok_out)
                {
                    M[i][j] = 0;
                    changed = true;
                    continue;
                }

                // Check incoming neighbours of pattern-vertex j
                bool ok_in = true;
                for (int u = 0; u < Gn; ++u)
                {
                    int mult_pattern = G.getMultiplicity(u, j); // multiplicity of (u->j) in G
                    if (mult_pattern <= 0)
                        continue;

                    bool found = false;
                    for (int v = 0; v < Hn; ++v)
                    {
                        int mult_host = H.getMultiplicity(v, i); // multiplicity of (v->i) in H
                        if (mult_host >= mult_pattern && M[v][u] == 1)
                        {
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                    {
                        ok_in = false;
                        break;
                    }
                }

                if (!ok_in)
                {
                    M[i][j] = 0;
                    changed = true;
                }
            }
        }
    }

    // If any pattern-vertex j has no candidates left, no isomorphism possible
    for (int j = 0; j < Gn; ++j)
    {
        int col_sum = 0;
        for (int i = 0; i < Hn; ++i)
            col_sum += M[i][j];
        if (col_sum == 0)
            return false;
    }

    int *order = G.getVerticesByDegree();
    int *usedH = new int[Hn]();
    int *mapping = (int *)malloc(sizeof(int) * Gn);
    fill(mapping, mapping + Gn, -1);
    int count = 0;

    function<bool(int)> DFS = [&](int t) -> bool
    {
        if (count >= N)
            return true;

        if (t == Gn)
        {
            count++;
            if (verbose) {
                cout << "Succesful mapping: ";
                for (int i = 0; i < Gn; ++i)
                {
                    cout << mapping[i] << " ";
                }
                cout << endl;
            }
            if (count >= N)
                return true;

            return false;
        }

        int i = order[t];

        for (int j = 0; j < Hn; ++j)
        {
            if (M[j][i] == 1 && !usedH[j])
            {
                // check partial consistency
                bool consistent = true;

                // check for self loops
                if (G.getMultiplicity(i, i) > H.getMultiplicity(j, j))
                    consistent = false;

                for (int k = 0; k < t; ++k)
                {
                    int i2 = order[k];
                    if (mapping[i2] == -1)
                        continue;

                    // edge consistency
                    if (G.getMultiplicity(i2, i) > H.getMultiplicity(mapping[i2], j))
                        consistent = false;
                    if (G.getMultiplicity(i, i2) > H.getMultiplicity(j, mapping[i2]))
                        consistent = false;

                    if (!consistent)
                        break;
                }

                if (!consistent)
                    continue;

                mapping[i] = j;
                usedH[j] = 1;

                if (DFS(t + 1))
                    return true;

                usedH[j] = 0;
                mapping[i] = -1;
            }
        }
        return false;
    };

    bool result = DFS(0);

    for (int i = 0; i < Hn; ++i)
        delete[] M[i];
    delete[] M;
    delete[] order;

    return result;
}

bool Graph::hasNSubgraphsApprox(Graph &G, int K, const std::vector<Mapping> &mappings, int N, bool verbose)
{
    std::cout << "### N SUBGRAPH ISOMORPHISMS APPROXIMATION ###" << endl; 
    // Host graph is "this"; pattern graph is G
    Graph &H = *this;

    const int m = G.getVerticesCount(); // rows (pattern vertices)
    const int n = H.getVerticesCount(); // cols (host vertices)
    if (m == 0)
        return true;
    if (n == 0 || m > n)
        return false;

    int total = 0;
    // 4) Convert each association to a mapping φ and check
    for (int k = 0; k < K; ++k)
    {
        vector<int> mapping = mappings[k];
        // Enforce injectivity (no two pattern vertices map to the same host vertex)
        vector<int> used(n, 0);
        bool injective = true;

        for (int u = 0; u < m; ++u)
        {
            if (mapping[u] < 0)
            {
                injective = false;
                break;
            }
            if (used[mapping[u]])
            {
                injective = false;
                break;
            }
            used[mapping[u]] = 1;
        }
        if (!injective)
            continue;

        if (G.detectIsomorphism(H, mapping))
        {
            if (verbose) {
                cout << "isomorphism #" << total << ": " << mapping << '\n';
            }
            total += 1;
            if (total >= N)
            {
                return true;
            }
        }
    }

    return false;
}

bool Graph::hasNSubgraphsApprox(Graph &G, int K, int N, bool verbose)
{
    Graph H = *this;

    const int m = G.getVerticesCount(); // rows (pattern vertices)
    const int n = H.getVerticesCount(); // cols (host vertices)
    if (m == 0)
        return true;
    if (n == 0 || m > n)
        return false;

    vector<Mapping> mappings = H.selectMappings(G, K);
    return hasNSubgraphsApprox(G, K, mappings, N, verbose);
}

int *Graph::getVerticesByDegree()
{
    // Step 1: Compute degrees of all vertices
    vector<pair<int, int>> vertexDegrees; // pair<degree, index>

    for (int i = 0; i < nodes; i++)
    {
        int degree = getOutDegree(i) + getInDegree(i);
        vertexDegrees.push_back({degree, i});
    }

    // Step 2: Sort vertices by degree (descending)
    sort(vertexDegrees.begin(), vertexDegrees.end(),
         [](const pair<int, int> &a, const pair<int, int> &b)
         {
             return a.first > b.first; // descending order
         });

    // Step 3: Create array of indices in sorted order
    int *sortedVertices = new int[nodes];
    for (int i = 0; i < nodes; i++)
    {
        sortedVertices[i] = vertexDegrees[i].second;
    }

    return sortedVertices;
}

void Graph::findMinimalExtension(Graph &G, int N)
{
    std::cout << "### MINIMAL EXTENSION ###" << endl; 

    Graph H = *this;

    int Hn = H.getVerticesCount();
    int Gn = G.getVerticesCount();

    int *order = G.getVerticesByDegree();
    int *usedH = new int[Hn]();
    vector<int> mapping(Gn, -1);
    vector<vector<vector<int>>> all_Edgesets;

    function<void(vector<int>)> createEdgeset = [&](vector<int> mapping) -> void
    {
        vector<vector<int>> local_Edgeset(Hn, vector<int>(Hn, 0));
        for (int a = 0; a < Gn; ++a)
        {
            for (int b = 0; b < Gn; ++b)
            {
                int ja = mapping[a];
                int jb = mapping[b];
                if (ja < 0 || jb < 0)
                    continue;

                int multG = G.getMultiplicity(a, b);
                int multH = H.getMultiplicity(ja, jb);

                int total = max(0, multG - multH);
                if (total > 0)
                    local_Edgeset[ja][jb] = total; // for some reaon this should be flipped, idk rly why
            }
        }

        all_Edgesets.push_back(local_Edgeset);
    };

    function<void(int)> DFS = [&](int t) -> void
    {
        if (t == Gn)
        {
            createEdgeset(mapping);
            return;
        }

        int i = order[t];

        for (int j = 0; j < Hn; ++j)
        {
            if (usedH[j])
                continue;

            mapping[i] = j;
            usedH[j] = 1;

            DFS(t + 1);

            usedH[j] = 0;
            mapping[i] = -1;
        }
        return;
    };

    DFS(0);

    int counter = 0;
    int bestCost = INT_MAX;
    vector<vector<int>> bestEdgeSet(Hn, vector<int>(Hn, 0));
    vector<int> usedInSum(N, -1);
    vector<vector<int>> sum(Hn, vector<int>(Hn, 0));
    set<int> usedEdgeSets;

    auto AdjMatrixAdd = [&](vector<vector<int>> &mat1,
                            const vector<vector<int>> &mat2)
    {
        for (int i = 0; i < Hn; ++i)
        {
            for (int j = 0; j < Hn; ++j)
            {
                mat1[i][j] = max(mat1[i][j], mat2[i][j]);
            }
        }
    };

    auto EdgeSetCost = [&](const vector<vector<int>> &mat)
    {
        int cost = 0;
        for (int i = 0; i < Hn; ++i)
            for (int j = 0; j < Hn; ++j)
                cost += mat[i][j];
        return cost;
    };
    // Recursive DFS over combinations of edge sets
    function<void(int)> EdgeDfs = [&](int level)
    {
        ++counter;
        int cost = EdgeSetCost(sum);

        if (level == N)
        {
            if (cost < bestCost)
            {
                bestCost = cost;
                bestEdgeSet = sum;
            }
            return;
        }

        if (cost > bestCost)
            return;

        for (size_t i = 0; i < all_Edgesets.size(); i++)
        {
            if (usedEdgeSets.count(i) == 0)
            {
                usedEdgeSets.insert(i);

                vector<vector<int>> backup = sum;
                AdjMatrixAdd(sum, all_Edgesets[i]);
                EdgeDfs(level + 1);

                sum = backup;
                usedEdgeSets.erase(i);
            }
        }
    };

    EdgeDfs(0);

    cout << "Best cost: " << bestCost << endl;

    cout << "Extension Matrix: " << endl;
    for (int a = 0; a < Hn; a++)
    {
        for (int b = 0; b < Hn; b++)
        {
            cout << bestEdgeSet[b][a] << " ";
        }
        cout << endl;
    }
}

void Graph::findMinimalExtensionApprox(Graph &G, int K, const std::vector<Mapping> &mappings, int N, bool verbose)
{
    std::cout << "### MINIMAL EXTENSION APPROXIMATION ###" << endl; 
    Graph H = *this;

    int Hn = H.getVerticesCount();
    int Gn = G.getVerticesCount();

    vector<vector<int>> all_Edgesets(Hn, vector<int>(Hn, 0));
    
    vector<bool> is_isomorphism(mappings.size());
    int total = 0;
    std::transform(mappings.begin(),mappings.end(), is_isomorphism.begin(),
        [&G,&H,&total, verbose](auto it){
            if (G.detectIsomorphism(H, it)) {
                total += 1;
                if (verbose) cout << "isomorphism #" << total << ": " << it << '\n';
                return true;
            }
            return false;
        }
    );
    
    auto AdjMatrixAdd = [&](vector<vector<int>> &mat1,
                            const vector<vector<int>> &mat2)
    {
        int Rn = mat2.size();

        for (int i = 0; i < Rn; ++i)
        {
            for (int j = 0; j < Rn; ++j)
            {
                mat1[i][j] = max(mat1[i][j], mat2[i][j]);
            }
        }
    };


    for (int k = 0; k < K; k++)
    {
        if (is_isomorphism[k]) {
            continue;
        }
        vector<int> mapping = mappings[k];
        bool injective = true;
        for (int u = 0; u < Gn; ++u)
        {
            if (mapping[u] < 0)
            {
                injective = false;
                break;
            }
        }
        if (!injective) {
            continue;
        }
        total+=1;
        if (verbose) cout << "extension #" << total << ": " << mapping << '\n';
        vector<vector<int>> current_edgeset = H.constructEdgeSet(G, mapping);
        AdjMatrixAdd(all_Edgesets, current_edgeset);

        if (total == N)
        {
            break;
        }
    }

    int cost = 0;
    for (int a = 0; a < Hn; a++)
        for (int b = 0; b < Hn; b++)
            cost += all_Edgesets[a][b];
    cout << "Best cost: " << cost << endl;

    cout << "Extension Matrix: " << endl;

    for (int a = 0; a < Hn; a++)
    {
        for (int b = 0; b < Hn; b++)
        {
            cout << all_Edgesets[b][a] << " ";
        }
        cout << endl;
    }
}

void Graph::findMinimalExtensionApprox(Graph &G, int K, int N, bool verbose)
{
    Graph H = *this;
    vector<Mapping> mappings = H.selectMappings(G, K);
    return findMinimalExtensionApprox(G, K, mappings, N, verbose);
}

bool Graph::detectIsomorphism(Graph &hostGraph, const vector<int> &vertexMapping)
{
    // Size check: vertexMapping must map all vertices of this graph (G) into hostGraph
    if ((int)vertexMapping.size() != nodes)
        return false;
    if (hostGraph.getVerticesCount() <= 0)
        return false;

    const int hostVertexCount = hostGraph.getVerticesCount();

    // For all directed edges (u,v) in G, ensure mult_G(u,v) <= mult_H(phi(u), phi(v))
    for (int u = 0; u < nodes; ++u)
    {
        const int mappedU = vertexMapping[u];
        if (mappedU < 0 || mappedU >= hostVertexCount)
            return false;
        for (int v = 0; v < nodes; ++v)
        {
            int multG = getMultiplicity(u, v);
            if (multG <= 0)
                continue;
            const int mappedV = vertexMapping[v];
            if (mappedV < 0 || mappedV >= hostVertexCount)
                return false;
            int multH = hostGraph.getMultiplicity(mappedU, mappedV);
            if (multG > multH)
                return false;
        }
    }
    return true;
}

vector<vector<double>> Graph::computeVertexMappingCostMatrix(const Graph &hostGraph) const
{
    const int patternVertexCount = getVerticesCount();
    const int hostVertexCount = hostGraph.getVerticesCount();
    vector<vector<double>> cost(patternVertexCount, vector<double>(hostVertexCount, 0));
    double minCost = 0;
    for (int u = 0; u < patternVertexCount; ++u)
    {
        const int uOutDeg = getOutDegree(u);
        const int uInDeg = getInDegree(u);
        for (int v = 0; v < hostVertexCount; ++v)
        {
            int vOutDeg = hostGraph.getOutDegree(v);
            int vInDeg = hostGraph.getInDegree(v);
            // (uInDeg<vInDeg ? 1 : -1) - positive cost is bad, and it is bad for us if H has higher degree than G for given vertex in mapping
            // int inCost = (uInDeg<vInDeg ? 1 : -1) * (uInDeg-vInDeg) * (uInDeg-vInDeg);
            // int outCost = (uOutDeg<vOutDeg ? 1 : -1) * (uOutDeg-vOutDeg) * (uOutDeg-vOutDeg); 
            double finalCost = (uOutDeg+uInDeg)-(vOutDeg+vInDeg);
            minCost = min(minCost, finalCost);
            cost[u][v] = finalCost;
        }
    }

    if(minCost >= 0)
        return cost;
    
    for(int u = 0; u < patternVertexCount; ++u)
    {
        for(int v = 0; v < hostVertexCount; ++v)
        {
            cost[u][v] = cost[u][v] - minCost;
        }
    }

    return cost;
}

vector<Mapping> Graph::selectMappings(const Graph &G, const int K, const bool verbose) const
{
    const Graph& H = *this;
    const int m = G.getVerticesCount(); // rows (pattern vertices)
    const int n = H.getVerticesCount(); // cols (host vertices)
    if (m == 0)
        return vector<Mapping>();
    if (n == 0 || m > n)
        return vector<Mapping>();

    CostMatrix costMatrix = G.computeVertexMappingCostMatrix(H);
    
    vector<Assignment> x = murty(costMatrix, K);
    vector<Mapping> mappings(K);
    for (size_t i = 0; i<x.size(); i++) {
        auto &a = x[i];
        if (verbose) {
            cout << "assignment #" << i << ": ";
            for (size_t j = 0; j<a.mapping.size(); j++) {
                cout << a.mapping[j] << ' ';
            }
            cout << "/ " << a.cost << endl;
        }
        mappings[i]=a.mapping;
    }

    return mappings;
}

vector<vector<int>> Graph::constructEdgeSet(const Graph &G, Mapping mapping) const
{

    Graph H = *this;

    int Hn = H.getVerticesCount();
    int Gn = G.getVerticesCount();

    vector<vector<int>> edgeset(Hn, vector<int>(Hn, 0));

    for (int a = 0; a < Gn; ++a)
    {
        for (int b = 0; b < Gn; ++b)
        {
            int ja = mapping[a];
            int jb = mapping[b];
            if (ja < 0 || jb < 0)
                continue;

            int multG = G.getMultiplicity(a, b);
            int multH = H.getMultiplicity(ja, jb);

            int total = max(0, multG - multH);
            if (total > 0)
                edgeset[ja][jb] = total; // for some reaon this should be flipped, idk rly why
        }
    }

    return edgeset;
}

namespace {

    CostMatrix computeGedVertexCostMatrix(const Graph &small, const Graph &large)
    {
        const int m = small.getVerticesCount();
        const int n = large.getVerticesCount();
        CostMatrix cost(m, std::vector<Cost>(n, 0.0));

        for (int u = 0; u < m; ++u)
        {
            const int in_u = small.getInDegree(u);
            const int out_u = small.getOutDegree(u);
            for (int v = 0; v < n; ++v)
            {
                const int in_v = large.getInDegree(v);
                const int out_v = large.getOutDegree(v);
                cost[u][v] = static_cast<double>(std::abs(in_u - in_v) + std::abs(out_u - out_v));
            }
        }
        return cost;
    }

    std::vector<int> computeUnmatchedVertices(const int largeN, const Mapping &mapSmallToLarge)
    {
        std::vector<int> used(largeN, 0);
        for (int u = 0; u < (int)mapSmallToLarge.size(); ++u)
        {
            const int v = mapSmallToLarge[u];
            if (v >= 0 && v < largeN) used[v] = 1;
        }
        std::vector<int> unmatched;
        for (int v = 0; v < largeN; ++v)
            if (!used[v]) unmatched.push_back(v);

        return unmatched;
    }

    long long edgeL1CostEmbedded(const Graph &small, const Graph &large, const Mapping &mapSmallToLarge)
    {
        const int m = small.getVerticesCount();
        const int n = large.getVerticesCount();

        // inverse: large vertex -> small vertex or -1
        std::vector<int> inv(n, -1);
        for (int u = 0; u < m; ++u)
        {
            const int v = mapSmallToLarge[u];
            if (v < 0 || v >= n) return LLONG_MAX / 4; // invalid mapping
            if (inv[v] != -1) return LLONG_MAX / 4;     // not injective
            inv[v] = u;
        }

        long long cost = 0;
        for (int i = 0; i < n; ++i)
        {
            for (int j = 0; j < n; ++j)
            {
                const int si = inv[i];
                const int sj = inv[j];
                const int a = (si != -1 && sj != -1) ? small.getMultiplicity(si, sj) : 0;
                const int b = large.getMultiplicity(i, j);
                cost += std::llabs((long long)a - (long long)b);
            }
        }
        return cost;
    }

} // namespace

Graph::GedResult Graph::gedApprox(const Graph &other, int K, bool buildPath, bool verbose) const
{
    GedResult res;
    const int nA = this->getVerticesCount();
    const int nB = other.getVerticesCount();
    if (K < 1) K = 1;

    // Choose orientation for mapping enumeration: smaller -> larger
    const bool aIsSmallerOrEqual = (nA <= nB);
    const Graph &small = aIsSmallerOrEqual ? *this : other;
    const Graph &large = aIsSmallerOrEqual ? other : *this;
    const int m = small.getVerticesCount();
    const int n = large.getVerticesCount();

    // Generate K candidate injective mappings using Murty/Hungarian, then score by true GED edge cost.
    CostMatrix cm = computeGedVertexCostMatrix(small, large);
    std::vector<Assignment> candidates = murty(cm, K);
    if (candidates.empty())
    {
        // No feasible assignment (should not happen for m<=n), but keep behavior predictable.
        res.total_cost = LLONG_MAX / 4;
        return res;
    }

    long long bestEdge = LLONG_MAX / 4;
    long long bestTotal = LLONG_MAX / 4;
    Mapping bestMapSmallToLarge;

    for (size_t i = 0; i < candidates.size(); ++i)
    {
        const Mapping &map = candidates[i].mapping;
        const long long edgeCost = edgeL1CostEmbedded(small, large, map);
        const long long vertexCost = (long long)(n - m);
        const long long total = edgeCost + vertexCost;

        if (verbose)
        {
            std::cout << "GED candidate #" << i
                      << " vertexCost=" << vertexCost
                      << " edgeCost=" << edgeCost
                      << " total=" << total
                      << " mapping=" << map << "\n";
        }

        if (total < bestTotal)
        {
            bestTotal = total;
            bestEdge = edgeCost;
            bestMapSmallToLarge = map;
        }
    }

    res.vertex_ops = (long long)std::abs(nA - nB);
    res.edge_ops = bestEdge;
    res.total_cost = bestTotal;
    // Build mapping in "this -> other" direction and optionally construct an edit path.
    if (aIsSmallerOrEqual)
    {
        // this == small, other == large
        res.mapping_this_to_other = bestMapSmallToLarge;
        res.inserted_vertices_in_other = computeUnmatchedVertices(nB, bestMapSmallToLarge);

        if (buildPath)
        {
            // inverse: other vertex -> this vertex (or -1 if inserted)
            std::vector<int> inv(nB, -1);
            for (int u = 0; u < nA; ++u) inv[res.mapping_this_to_other[u]] = u;

            // 1) add missing vertices first
            for (int v : res.inserted_vertices_in_other)
            {
                res.ops.push_back({GedEditOp::Type::AddVertex, v, -1, 1});
            }

            // 2) edge edits in other's vertex space
            for (int i = 0; i < nB; ++i)
            {
                for (int j = 0; j < nB; ++j)
                {
                    const int si = inv[i];
                    const int sj = inv[j];
                    const int a = (si != -1 && sj != -1) ? this->getMultiplicity(si, sj) : 0;
                    const int b = other.getMultiplicity(i, j);
                    const int d = b - a;
                    if (d > 0)
                        res.ops.push_back({GedEditOp::Type::AddEdge, i, j, d});
                    else if (d < 0)
                        res.ops.push_back({GedEditOp::Type::DelEdge, i, j, -d});
                }
            }
        }
    }
    else
    {
        // this == large, other == small
        // bestMapSmallToLarge maps: other(u) -> this(v). Convert to partial mapping: this(v) -> other(u)
        res.mapping_this_to_other.assign(nA, -1);
        for (int u = 0; u < nB; ++u)
        {
            const int v = bestMapSmallToLarge[u];
            if (v >= 0 && v < nA) res.mapping_this_to_other[v] = u;
        }

        for (int v = 0; v < nA; ++v)
            if (res.mapping_this_to_other[v] == -1) res.deleted_vertices_in_this.push_back(v);

        if (buildPath)
        {
            // 1) delete/add edges first (so vertices can be isolated before deletion)
            for (int i = 0; i < nA; ++i)
            {
                for (int j = 0; j < nA; ++j)
                {
                    const int a = this->getMultiplicity(i, j);
                    int b = 0;
                    const int mi = res.mapping_this_to_other[i];
                    const int mj = res.mapping_this_to_other[j];
                    if (mi != -1 && mj != -1)
                        b = other.getMultiplicity(mi, mj);

                    const int d = b - a;
                    if (d > 0)
                        res.ops.push_back({GedEditOp::Type::AddEdge, i, j, d});
                    else if (d < 0)
                        res.ops.push_back({GedEditOp::Type::DelEdge, i, j, -d});
                }
            }

            // 2) now delete vertices that are not mapped
            for (int v : res.deleted_vertices_in_this)
            {
                res.ops.push_back({GedEditOp::Type::DelVertex, v, -1, 1});
            }
        }
    }

    return res;
}
