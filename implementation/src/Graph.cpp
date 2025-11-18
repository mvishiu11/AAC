#include "../include/Graph.hpp"
#include <functional>
#include "Graph.hpp"
#include <set>
extern "C" {
#include "../fastmurty/da.h"
}
#include <memory>

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

bool Graph::hasNSubgraphs(Graph &G, int N)
{
    Graph H = *this;
    if (G.getVerticesCount() == 0 || G.getVerticesCount() > H.getVerticesCount())
    {
        return true;
    }
    int Hn = H.getVerticesCount();
    int Gn = G.getVerticesCount();

    int **M = (int **)malloc(sizeof(int *) * Hn);
    for (int i = 0; i < Hn; i++)
    {
        M[i] = (int *)malloc(sizeof(int) * Gn);
    }

    for (int i = 0; i < Hn; i++)
    {
        for (int j = 0; j < Gn; j++)
        {
            cout << "i=" << i << " (out=" << H.getOutDegree(i) << ", in=" << H.getInDegree(i)
          << "), j=" << j << " (out=" << G.getOutDegree(j) << ", in=" << G.getInDegree(j) << ")\n";

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
        for(int i = 0; i<Hn; i++){
        for(int j=0; j<Gn; j++){
            cout<<M[i][j]<<" ";
        }
        cout<<endl;
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

    cout<<endl;
    for(int i = 0; i<Hn; i++){
        for(int j=0; j<Gn; j++){
            cout<<M[i][j]<<" ";
        }
        cout<<endl;
    }

    function<bool(int)> DFS = [&](int t) -> bool
    {
        if (count >= N)
            return true;

        if (t == Gn)
        {
            count++;
            cout << "Succesful mapping: ";
            for (int i = 0; i < Gn; ++i)
            {
                cout << mapping[i] << " ";
            }
            cout << endl;
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

                //check for self loops
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

bool Graph::hasNSubgraphsApprox(Graph &G, int N)
{
    // Host graph is "this"; pattern graph is G
    Graph H = *this;

    const int m = G.getVerticesCount(); // rows (pattern vertices)
    const int n = H.getVerticesCount(); // cols (host vertices)
    if (m == 0)
        return true;
    if (n == 0 || m > n)
        return false;

    // 1) Build cost matrix using degree differences
    std::vector<std::vector<int>> costMatrixInt = G.computeVertexMappingCostMatrix(H);
    std::vector<double> costMatrix(m * n, 0.0);
    for (int u = 0; u < m; ++u)
        for (int v = 0; v < n; ++v)
            costMatrix[u * n + v] = static_cast<double>(costMatrixInt[u][v]);

    // 2) Prepare priors (single prior that includes all rows/cols)
    const int numRowPriors = 1;
    const int numColPriors = 1;
    std::unique_ptr<bool[]> rowPriors(new bool[numRowPriors * m]);
    std::fill(rowPriors.get(), rowPriors.get() + (numRowPriors * m), true);
    std::vector<double> rowPriorWeights(numRowPriors, 0.0);
    std::unique_ptr<bool[]> colPriors(new bool[numColPriors * n]);
    std::fill(colPriors.get(), colPriors.get() + (numColPriors * n), true);
    std::vector<double> colPriorWeights(numColPriors, 0.0);

    // 3) Run Murty (K-best) via fastmurty
    const int K = 90;
    std::vector<int> outAssocs(K * (m + n) * 2, -2);
    std::vector<double> outCosts(K, 0.0);

    WorkvarsforDA work = allocateWorkvarsforDA(m, n, K);
    int ret = da(
        costMatrix.data(),
        numRowPriors, rowPriors.get(), rowPriorWeights.data(),
        numColPriors, colPriors.get(), colPriorWeights.data(),
        K, outAssocs.data(), outCosts.data(), &work);

    for(int i = 0; i<K*(m+n)*2; i++){
        cout<<"Association "<<i<<": "<<outAssocs[i]<<endl;
    }

    if(ret!=0) {
        cout<<"Error: "<<ret<<endl;
        return false;// ret==0 success, non-zero means fewer than K associations
    }

    
    int total = 0;
    // 4) Convert each association to a mapping φ and check
    for (int k = 0; k < K; ++k)
    {
        std::vector<int> mapping(m, -1);

        int base = k * (m + n) * 2;
        for (int z = 0; z < (m + n); ++z)
        {
            int a = outAssocs[base + 2 * z + 0];
            int b = outAssocs[base + 2 * z + 1];
            if (a >= 0 && a < m && b >= 0 && b < n)
            {
                mapping[a] = b;
            }
        }

        for(int i = 0; i<m; i++){
            cout<<"Mapping["<<i<<"]: "<<mapping[i]<<endl;
        }

        // // Enforce injectivity (no two pattern vertices map to the same host vertex)
        std::vector<int> used(n, 0);
        bool injective = true;
        for (int u = 0; u < m; ++u)
        {
            if (mapping[u] < 0) { injective = false; break; }
            if (used[mapping[u]]) { injective = false; break; }
            used[mapping[u]] = 1;
        }
        if (!injective)
            continue;

        if (G.detectIsomorphism(H, mapping))
        {
            total += 1;
            if (total >= N)
            {
                deallocateWorkvarsforDA(work);
                return true;
            }
        }
    }

    deallocateWorkvarsforDA(work);
    return false;
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
    int bestCost = __INT_MAX__;
    vector<vector<int>> bestEdgeSet(Hn, vector<int>(Hn, 0));
    vector<int> usedInSum(N, -1);
    vector<vector<int>> sum(Hn, vector<int>(Hn, 0));
    set<int> usedEdgeSets;

    auto AdjMatrixAdd = [&](std::vector<std::vector<int>> &mat1,
                            const std::vector<std::vector<int>> &mat2)
    {
        for (int i = 0; i < Hn; ++i)
        {
            for (int j = 0; j < Hn; ++j)
            {
                mat1[i][j] = max(mat1[i][j], mat2[i][j]);
            }
        }
    };

    auto EdgeSetCost = [&](const std::vector<std::vector<int>> &mat)
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

        for (int i = 0; i < all_Edgesets.size(); i++)
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

    for (int a = 0; a < Hn; a++)
    {
        for (int b = 0; b < Hn; b++)
        {
            cout << bestEdgeSet[b][a] << " ";
        }
        cout << endl;
    }
    // return {bestCost, bestEdgeSet};
}

bool Graph::detectIsomorphism(Graph &hostGraph, const std::vector<int> &vertexMapping)
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

std::vector<std::vector<int>> Graph::computeVertexMappingCostMatrix(const Graph &hostGraph) const
{
    const int patternVertexCount = getVerticesCount();
    const int hostVertexCount = hostGraph.getVerticesCount();
    std::vector<std::vector<int>> cost(patternVertexCount, std::vector<int>(hostVertexCount, 0));
    for (int u = 0; u < patternVertexCount; ++u)
    {
        const int degreeG = getOutDegree(u) + getInDegree(u);
        for (int v = 0; v < hostVertexCount; ++v)
        {
            const int degreeH = hostGraph.getOutDegree(v) + hostGraph.getInDegree(v);
            cost[u][v] = max(0, degreeG - degreeH);
            cout<<"Cost["<<u<<"]["<<v<<"]: "<<cost[u][v]<<endl;
        }
    }
    return cost;
}
