#include "../include/Graph.hpp"
#include <functional>

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
    // !!! STILL DOESN'T WORK PROPERLY

    if (G.getVerticesCount() == 0 || G.getVerticesCount() > this->getVerticesCount())
    {
        return true;
    }
    int Hn = this->getVerticesCount();
    int Gn = G.getVerticesCount();

    int **M = (int **)malloc(sizeof(int *) * Hn);
    for (int i = 0; i < Hn; i++)
    {
        M[i] = (int *)malloc(sizeof(int) * Gn);
    }

    std::cout << "Initializing candidate matrix..." << std::endl;

    for (int i = 0; i < Hn; i++)
    {
        for (int j = 0; j < Gn; j++)
        {
            if (this->getOutDegree(i) >= G.getOutDegree(j) && this->getInDegree(i) >= G.getInDegree(j))
            {
                M[i][j] = 1;
            }
            else
            {
                M[i][j] = 0;
            }
        }
    }

    for (int i = 0; i < Hn; i++)
    {
        for (int j = 0; j < Gn; j++)
        {
            std::cout << M[i][j] << " ";
        }
        std::cout << std::endl;
    }

    std::cout << "Refining candidate matrix..." << std::endl;
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
                        int mult_host = this->getMultiplicity(i, v); // multiplicity of (i->v) in H
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
                        int mult_host = this->getMultiplicity(v, i); // multiplicity of (v->i) in H
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

    for (int i = 0; i < Hn; i++)
    {
        for (int j = 0; j < Gn; j++)
        {
            std::cout << M[i][j] << " ";
        }
        std::cout << std::endl;
    }

    int *order = G.getVerticesByDegree();
    int *used_H = new int[Hn]();
    std::vector<int> mapping(Gn, -1);
    int count = 0;

    std::function<bool(int)> DFS = [&](int t) -> bool
    {
        if (count >= N)
            return true;

        if (t == Gn)
        {
            count++;
            std::cout << "Succesful mapping: ";
            for (int i = 0; i < Gn; ++i)
            {
                std::cout << mapping[i] << " ";
            }
            std::cout << std::endl;
            return false;
        }

        int i = order[t];

        for (int j = 0; j < Hn; ++j)
        {
            if (M[j][i] == 1 && !used_H[j])
            {
                // check partial consistency
                bool consistent = true;
                for (int k = 0; k < t; ++k)
                {
                    int i2 = order[k];
                    if (mapping[i2] == -1)
                        continue;

                    // edge consistency
                    if (G.getMultiplicity(i2, i) > this->getMultiplicity(mapping[i2], j))
                        consistent = false;
                    if (G.getMultiplicity(i, i2) > this->getMultiplicity(j, mapping[i2]))
                        consistent = false;

                    if (!consistent)
                        break;
                }

                if (!consistent)
                    continue;

                mapping[i] = j;
                used_H[j] = 1;

                if (DFS(t + 1))
                    return true;

                used_H[j] = 0;
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
    std::vector<std::pair<int, int>> vertexDegrees; // pair<degree, index>

    for (int i = 0; i < nodes; i++)
    {
        int degree = getOutDegree(i) + getInDegree(i);
        vertexDegrees.push_back({degree, i});
    }

    // Step 2: Sort vertices by degree (descending)
    std::sort(vertexDegrees.begin(), vertexDegrees.end(),
              [](const std::pair<int, int> &a, const std::pair<int, int> &b)
              {
                  return a.first > b.first; // descending order
              });

    // Step 3: Create array of indices in sorted order
    int *sortedVertices = new int[nodes];
    for (int i = 0; i < nodes; i++)
    {
        sortedVertices[i] = vertexDegrees[i].second;
        std::cout << "Vertex: " << sortedVertices[i] << " Degree: " << vertexDegrees[i].first << std::endl;
    }

    return sortedVertices;
}