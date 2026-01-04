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

    inline int multPad(const Graph& g, const int realN, const int v, const int w)
    {
        if (v < 0 || w < 0) return 0;
        if (v >= realN || w >= realN) return 0;
        return g.getMultiplicity(v, w);
    }

    std::vector<int> invertPermutation(const std::vector<int>& f)
    {
        std::vector<int> inv(f.size(), -1);
        for (int i = 0; i < (int)f.size(); ++i)
        {
            const int j = f[i];
            if (j >= 0 && j < (int)f.size()) inv[j] = i;
        }
        return inv;
    }

    long long edgeL1CostPaddedUnderBijection(
        const Graph& A, const int nA,
        const Graph& B, const int nB,
        const std::vector<int>& f)
    {
        const int n = (int)f.size();
        long long cost = 0;
        for (int i = 0; i < n; ++i)
        {
            const int fi = f[i];
            for (int j = 0; j < n; ++j)
            {
                const int fj = f[j];
                const int a = multPad(A, nA, i, j);
                const int b = multPad(B, nB, fi, fj);
                cost += std::llabs((long long)a - (long long)b);
            }
        }
        return cost;
    }

    void fillMappingAndVertexListsFromBijection(
        const int nA,
        const int nB,
        const std::vector<int>& f,
        Mapping& outThisToOther,
        std::vector<int>& insertedVerticesInOther,
        std::vector<int>& deletedVerticesInThis)
    {
        outThisToOther.assign(nA, -1);
        deletedVerticesInThis.clear();
        insertedVerticesInOther.clear();

        for (int i = 0; i < nA; ++i)
        {
            const int h = f[i];
            if (h >= 0 && h < nB)
                outThisToOther[i] = h;
            else
            {
                outThisToOther[i] = -1;
                deletedVerticesInThis.push_back(i);
            }
        }

        const auto inv = invertPermutation(f);
        for (int h = 0; h < nB; ++h)
        {
            const int pre = inv[h];
            if (pre >= nA) insertedVerticesInOther.push_back(h);
        }
    }

    std::vector<Graph::GedEditOp> buildEditPathFromMapping(
        const Graph& A,
        const Graph& B,
        const Mapping& mapAtoB,
        const std::vector<int>& insertedVerticesInB,
        const std::vector<int>& deletedVerticesInA)
    {
        std::vector<Graph::GedEditOp> ops;
        const int nA = A.getVerticesCount();
        const int nB = B.getVerticesCount();

        if (nA <= nB)
        {
            // Emit ops in target (B) indexing.
            std::vector<int> inv(nB, -1); // B vertex -> A vertex, or -1 for inserted
            for (int u = 0; u < nA; ++u)
            {
                const int v = mapAtoB[u];
                if (v >= 0 && v < nB) inv[v] = u;
            }

            for (int v : insertedVerticesInB)
                ops.push_back({Graph::GedEditOp::Type::AddVertex, v, -1, 1});

            for (int i = 0; i < nB; ++i)
            {
                for (int j = 0; j < nB; ++j)
                {
                    const int ai = inv[i];
                    const int aj = inv[j];
                    const int cur = (ai != -1 && aj != -1) ? A.getMultiplicity(ai, aj) : 0;
                    const int desired = B.getMultiplicity(i, j);
                    const int delta = desired - cur;
                    if (delta > 0) ops.push_back({Graph::GedEditOp::Type::AddEdge, i, j, delta});
                    else if (delta < 0) ops.push_back({Graph::GedEditOp::Type::DelEdge, i, j, -delta});
                }
            }
            return ops;
        }

        // nA > nB: emit ops in source (A) indexing.
        for (int i = 0; i < nA; ++i)
        {
            for (int j = 0; j < nA; ++j)
            {
                const int cur = A.getMultiplicity(i, j);
                int desired = 0;
                const int mi = mapAtoB[i];
                const int mj = mapAtoB[j];
                if (mi != -1 && mj != -1) desired = B.getMultiplicity(mi, mj);

                const int delta = desired - cur;
                if (delta > 0) ops.push_back({Graph::GedEditOp::Type::AddEdge, i, j, delta});
                else if (delta < 0) ops.push_back({Graph::GedEditOp::Type::DelEdge, i, j, -delta});
            }
        }
        for (int v : deletedVerticesInA)
            ops.push_back({Graph::GedEditOp::Type::DelVertex, v, -1, 1});

        return ops;
    }

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

} // namespace

Graph::GedResult Graph::gedApprox(const Graph &other, int K, bool buildPath, bool verbose) const
{
    GedResult res;
    const int nA = this->getVerticesCount();
    const int nB = other.getVerticesCount();
    if (K < 1) K = 1;

    const int nPad = std::max(nA, nB);
    const long long vertexCost = (long long)std::llabs((long long)nA - (long long)nB);

    // Trivial/degenerate cases (avoid Murty/Hungarian on empty matrices).
    if (nPad == 0)
    {
        res.total_cost = 0;
        res.vertex_ops = 0;
        res.edge_ops = 0;
        return res;
    }
    if (std::min(nA, nB) == 0)
    {
        std::vector<int> f(nPad);
        for (int i = 0; i < nPad; ++i) f[i] = i;
        const long long edgeCost = edgeL1CostPaddedUnderBijection(*this, nA, other, nB, f);

        res.vertex_ops = vertexCost;
        res.edge_ops = edgeCost;
        res.total_cost = vertexCost + edgeCost;
        fillMappingAndVertexListsFromBijection(nA, nB, f, res.mapping_this_to_other, res.inserted_vertices_in_other, res.deleted_vertices_in_this);
        if (buildPath)
            res.ops = buildEditPathFromMapping(*this, other, res.mapping_this_to_other, res.inserted_vertices_in_other, res.deleted_vertices_in_this);
        return res;
    }

    // Choose orientation for mapping enumeration: smaller -> larger
    const bool aIsSmallerOrEqual = (nA <= nB);
    const Graph &small = aIsSmallerOrEqual ? *this : other;
    const Graph &large = aIsSmallerOrEqual ? other : *this;

    // Generate K candidate injective mappings using Murty/Hungarian, then score by true GED cost.
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
    std::vector<int> bestF;

    for (size_t i = 0; i < candidates.size(); ++i)
    {
        const Mapping &map = candidates[i].mapping;

        // Build an explicit bijection f on the padded vertex set of size n.
        std::vector<int> f(nPad, -1); // this(padded) -> other(padded)
        if (aIsSmallerOrEqual)
        {
            // this == small, other == large
            std::vector<int> used(nB, 0);
            for (int u = 0; u < nA; ++u)
            {
                const int v = map[u];
                if (v < 0 || v >= nB) { f.clear(); break; }
                f[u] = v;
                used[v] = 1;
            }
            if (!f.empty())
            {
                int t = nA;
                for (int v = 0; v < nB; ++v)
                    if (!used[v]) f[t++] = v;
            }
        }
        else
        {
            // this == large, other == small; map is other(u)->this(v)
            std::vector<int> gToH(nA, -1);
            std::vector<int> usedG(nA, 0);
            for (int h = 0; h < nB; ++h)
            {
                const int g = map[h];
                if (g < 0 || g >= nA) { f.clear(); break; }
                if (usedG[g]) { f.clear(); break; }
                usedG[g] = 1;
                gToH[g] = h;
            }
            if (!f.empty())
            {
                int nextDummy = nB;
                for (int g = 0; g < nA; ++g)
                {
                    if (gToH[g] != -1) f[g] = gToH[g];
                    else f[g] = nextDummy++;
                }
            }
        }

        if (f.empty())
            continue;

        const long long edgeCost = edgeL1CostPaddedUnderBijection(*this, nA, other, nB, f);
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
            bestF = f;
        }
    }

    res.vertex_ops = vertexCost;
    res.edge_ops = bestEdge;
    res.total_cost = bestTotal;
    fillMappingAndVertexListsFromBijection(nA, nB, bestF, res.mapping_this_to_other, res.inserted_vertices_in_other, res.deleted_vertices_in_this);
    if (buildPath)
        res.ops = buildEditPathFromMapping(*this, other, res.mapping_this_to_other, res.inserted_vertices_in_other, res.deleted_vertices_in_this);

    return res;
}

Graph::GedResult Graph::gedExact(const Graph& other, bool buildPath, bool verbose, long long maxStates) const
{
    GedResult res;
    const int nA = this->getVerticesCount();
    const int nB = other.getVerticesCount();
    const int n = std::max(nA, nB);
    const long long vertexCost = (long long)std::llabs((long long)nA - (long long)nB);

    // Precompute padded in/out degrees.
    std::vector<int> outA(n, 0), inA(n, 0), outB(n, 0), inB(n, 0);
    for (int v = 0; v < n; ++v)
    {
        int oA = 0, iA = 0, oB = 0, iB = 0;
        for (int w = 0; w < n; ++w)
        {
            oA += multPad(*this, nA, v, w);
            iA += multPad(*this, nA, w, v);
            oB += multPad(other, nB, v, w);
            iB += multPad(other, nB, w, v);
        }
        outA[v] = oA; inA[v] = iA;
        outB[v] = oB; inB[v] = iB;
    }

    std::vector<int> order(n);
    for (int i = 0; i < n; ++i) order[i] = i;
    std::sort(order.begin(), order.end(), [&](int a, int b) {
        return (outA[a] + inA[a]) > (outA[b] + inA[b]);
    });

    std::vector<int> f(n, -1);
    std::vector<char> usedH(n, 0);

    long long bestEdge = LLONG_MAX / 4;
    std::vector<int> bestF;
    bool stopped = false;

    auto hungarianLB = [&](const std::vector<int>& remG, const std::vector<int>& remH) -> long long {
        const int r = (int)remG.size();
        if (r == 0) return 0;

        CostMatrix outCost(r, std::vector<Cost>(r, 0.0));
        CostMatrix inCost(r, std::vector<Cost>(r, 0.0));
        for (int i = 0; i < r; ++i)
        {
            const int gv = remG[i];
            for (int j = 0; j < r; ++j)
            {
                const int hv = remH[j];
                outCost[i][j] = (double)std::abs(outA[gv] - outB[hv]);
                inCost[i][j] = (double)std::abs(inA[gv] - inB[hv]);
            }
        }

        const double lbOut = hungarian(outCost).cost;
        const double lbIn = hungarian(inCost).cost;
        const long long lbOutLL = (long long)std::floor(lbOut + 1e-9);
        const long long lbInLL = (long long)std::floor(lbIn + 1e-9);
        return std::max(lbOutLL, lbInLL);
    };

    std::function<void(int, long long)> dfs = [&](int depth, long long currentCost) {
        if (stopped) return;
        if (res.states_visited >= maxStates)
        {
            stopped = true;
            return;
        }
        ++res.states_visited;

        if (depth == n)
        {
            if (currentCost < bestEdge)
            {
                bestEdge = currentCost;
                bestF = f;
            }
            return;
        }

        const int gv = order[depth];

        // Remaining-vertices Hungarian degree bound.
        {
            std::vector<int> remG;
            remG.reserve(n - depth);
            for (int d = depth; d < n; ++d) remG.push_back(order[d]);

            std::vector<int> remH;
            remH.reserve(n - depth);
            for (int hv = 0; hv < n; ++hv) if (!usedH[hv]) remH.push_back(hv);

            const long long lb = hungarianLB(remG, remH);
            if (currentCost + lb >= bestEdge)
            {
                ++res.states_pruned;
                return;
            }
        }

        for (int hv = 0; hv < n; ++hv)
        {
            if (usedH[hv]) continue;
            usedH[hv] = 1;
            f[gv] = hv;

            long long inc = 0;
            inc += std::llabs((long long)multPad(*this, nA, gv, gv) - (long long)multPad(other, nB, hv, hv));
            for (int d = 0; d < depth; ++d)
            {
                const int gu = order[d];
                const int hu = f[gu];
                inc += std::llabs((long long)multPad(*this, nA, gv, gu) - (long long)multPad(other, nB, hv, hu));
                inc += std::llabs((long long)multPad(*this, nA, gu, gv) - (long long)multPad(other, nB, hu, hv));
            }

            if (currentCost + inc < bestEdge)
                dfs(depth + 1, currentCost + inc);
            else
                ++res.states_pruned;

            f[gv] = -1;
            usedH[hv] = 0;
            if (stopped) return;
        }
    };

    if (verbose)
    {
        std::cout << "GED exact: n=" << n << " maxStates=" << maxStates << "\n";
    }

    dfs(0, 0);

    res.complete = !stopped;
    res.vertex_ops = vertexCost;
    res.edge_ops = bestEdge;
    res.total_cost = vertexCost + bestEdge;
    if (!bestF.empty())
    {
        fillMappingAndVertexListsFromBijection(nA, nB, bestF, res.mapping_this_to_other, res.inserted_vertices_in_other, res.deleted_vertices_in_this);
        if (buildPath)
            res.ops = buildEditPathFromMapping(*this, other, res.mapping_this_to_other, res.inserted_vertices_in_other, res.deleted_vertices_in_this);
    }
    return res;
}
