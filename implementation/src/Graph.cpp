#include "../include/Graph.hpp"
#include <functional>
#include "Graph.hpp"
#include <set>

using namespace std;

int Graph::getSize()
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
            cout << "Succesful mapping: ";
            for (int i = 0; i < Gn; ++i)
            {
                cout << mapping[i] << " ";
            }
            cout << endl;
            return false;
        }

        int i = order[t];

        for (int j = 0; j < Hn; ++j)
        {
            if (M[j][i] == 1 && !usedH[j])
            {
                // check partial consistency
                bool consistent = true;
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
                    local_Edgeset[jb][ja] = total; // for some reaon this should be flipped, idk rly why
            }
        }

        all_Edgesets.push_back(local_Edgeset);
    };

    function<void(int)> DFS = [&](int t) -> void
    {
        if (t == Gn)
        {
            createEdgeset(mapping);
            // if (all_Edgesets.size() < 6)
            // {
            //     for (int i = 0; i < Gn; i++)
            //     {
            //         cout << mapping[i] << " ";
            //     }
            //     cout << endl;
            // }
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

    // for (int i = 0; i < 10; i++)
    // {
    //     vector<vector<int>> local_Edgeset = all_Edgesets[i];
    //     for (int a = 0; a < Hn; a++)
    //     {
    //         for (int b = 0; b < Hn; b++)
    //         {
    //             cout << local_Edgeset[b][a] << " ";
    //         }
    //         cout << endl;
    //     }
    //     cout << endl;
    // }

    //cout << all_Edgesets.size() << endl;

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
        // cout << ++counter << endl;
        ++counter;
        // cout<<" HI "<< level << endl;
        int cost = EdgeSetCost(sum);

        if (level == N)
        {
            if (cost < bestCost)
            {
                bestCost = cost;
                bestEdgeSet = sum;
                // cout << " HI " << level << endl;
                // cout << counter << endl;
                // for (auto itr = usedEdgeSets.begin(); itr != usedEdgeSets.end(); itr++)
                // {
                //     cout << *itr << " ";
                // }
                // cout << endl;
                // cout << "Best cost: " << bestCost << endl;

                // for (int a = 0; a < Hn; a++)
                // {
                //     for (int b = 0; b < Hn; b++)
                //     {
                //         cout << bestEdgeSet[b][a] << " ";
                //     }
                //     cout << endl;
                // }
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
