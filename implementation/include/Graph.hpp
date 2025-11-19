#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>

#pragma once

class Graph
{
private:
    int **edges;
    int nodes;
    bool detectIsomorphism(Graph &hostGraph, const std::vector<int> &vertexMapping);
    std::vector<std::vector<int>> computeVertexMappingCostMatrix(const Graph &hostGraph) const;
    std::vector<std::vector<int>> selectMappings(const Graph &G, const int K) const;
    std::vector<std::vector<int>> constructEdgeSet(const Graph &G, std::vector<int> mapping) const;
    int *getVerticesByDegree();

public:
    Graph(int nodes, int **edges) : nodes(nodes), edges(edges) {}
    int getSize() const;
    int getVerticesCount() const { return nodes; }
    int getMultiplicity(int v, int w) const { return edges[w][v]; }
    int getOutDegree(int v) const
    {
        int outDegree = 0;
        for (int i = 0; i < nodes; i++)
        {
            outDegree += edges[i][v];
        }
        return outDegree;
    }
    int getInDegree(int v) const
    {
        int inDegree = 0;
        for (int i = 0; i < nodes; i++)
        {
            inDegree += edges[v][i];
        }
        return inDegree;
    }
    bool hasNSubgraphs(Graph &G, int N = 1);
    bool hasNSubgraphsApprox(Graph &G, int N = 1);
    void findMinimalExtension(Graph &G, int N = 1);
    void findMinimalExtensionApprox(Graph &G, int N = 1);
 
};
