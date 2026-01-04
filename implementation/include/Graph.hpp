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
    struct GedEditOp {
        enum class Type { AddVertex, DelVertex, AddEdge, DelEdge };
        Type type;
        int from = -1;
        int to = -1;
        int multiplicity = 1;
    };
    struct GedResult {
        long long total_cost = 0;
        long long vertex_ops = 0;
        long long edge_ops = 0;

        Mapping mapping_this_to_other;
        std::vector<int> inserted_vertices_in_other;
        std::vector<int> deleted_vertices_in_this;
        std::vector<GedEditOp> ops;

        bool complete = true;
        long long states_visited = 0;
        long long states_pruned = 0;
    };

    std::vector<Mapping> selectMappings(const Graph &G, const int K, bool verbose = false) const;
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
    bool hasNSubgraphs(Graph &G, int N = 1, bool verbose = false);
    bool hasNSubgraphsApprox(Graph &G, int K, int N = 1, bool verbose = false);
    bool hasNSubgraphsApprox(Graph &G, int K, const std::vector<Mapping> &mappings, int N = 1, bool verbose = false);
    void findMinimalExtension(Graph &G, int N = 1);
    void findMinimalExtensionApprox(Graph &G, int K, int N = 1, bool verbose = false);
    void findMinimalExtensionApprox(Graph &G, int K, const std::vector<Mapping> &mappings, int N = 1, bool verbose = false);
    GedResult gedApprox(const Graph &other, int K, bool buildPath = false, bool verbose = false) const;
    GedResult gedExact(const Graph &other, bool buildPath = false, bool verbose = false, long long maxStates = 5000000) const;
};
