#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>

#pragma once


class Graph{
private:
    int** edges;
    int nodes;
public:
    Graph(int nodes, int** edges): nodes(nodes), edges(edges) {}
    int getSize();
    int getVerticesCount() { return nodes; }
    int getMultiplicity(int v, int w) { return edges[v][w]; }
    int getOutDegree(int v) { 
        int outDegree = 0;
        for(int i = 0; i < nodes; i++) {
            outDegree += edges[v][i];
        }
        return outDegree;
    }
    int getInDegree(int v) { 
        int inDegree = 0;
        for(int i = 0; i < nodes; i++) {
            inDegree += edges[i][v];
        }
        return inDegree;
    }
    bool hasNSubgraphs(Graph& G, int N=1);
    int* getVerticesByDegree();

};
