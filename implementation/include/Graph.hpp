#include "Hungarian.hpp"
#include <vector>

#pragma once

class Graph
{
private:
    int nodes;
    int **edges;
    bool detectIsomorphism(Graph &hostGraph, const std::vector<int> &vertexMapping);
    CostMatrix computeVertexMappingCostMatrix(const Graph &hostGraph) const;
    std::vector<std::vector<int>> constructEdgeSet(const Graph &G, Mapping mapping) const;
    int *getVerticesByDegree();
    
public:
    std::vector<Mapping> selectMappings(const Graph &G, const int K) const;
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
    bool hasNSubgraphsApprox(Graph &G, int K, int N = 1);
    bool hasNSubgraphsApprox(Graph &G, int K, const std::vector<Mapping> &mappings, int N = 1);
    void findMinimalExtension(Graph &G, int N = 1);
    void findMinimalExtensionApprox(Graph &G, int K, int N = 1);
    void findMinimalExtensionApprox(Graph &G, int K, const std::vector<Mapping> &mappings, int N = 1);
};
