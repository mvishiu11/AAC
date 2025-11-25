#include <algorithm>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include "../include/Graph.hpp"

using namespace std;

Graph* parseInput(string filename, int* N);
Graph createGraphFromFile(ifstream& file);

int maxK(int h, int g) {
    // (h choose g)g! 
    // = h!/(h-g)! 
    // = h*(h-1)*...*(h-g+2)*(h-g+1)
    if (g > h) return 0;
    if (g == 0) return 1;
    int result = 1;
    for (int i = 0; i < g; ++i) {
        result *= (h - i);
    }
    return result;
}

int main(int argc, char** argv) {
    if(argc < 2) return -1;
    string filename = argv[1];
    int N;
    auto graphs = parseInput(filename, &N);
    Graph G = graphs[0];
    Graph H = graphs[1];

    int mK = maxK(H.getVerticesCount(),G.getVerticesCount());
    int K = 123;
    K = std::min(K,mK); // In case our K is greater than maximum number of possible mappings
    
    cout << "### SELECT MAPPINGS ###" << endl;
    const std::vector<Mapping> mappings = H.selectMappings(G,K);

    if(H.hasNSubgraphs(G, N)) {
        cout << "EXACT: YES" << endl;
    } else {
        cout << "EXACT: NO" << endl;
    }
    
    if(H.hasNSubgraphsApprox(G, K, mappings, N)) {
        cout << "APPROXIMATION: YES" << endl;
    } else {
        cout << "APPROXIMATION: NO" << endl;
    }
    
    H.findMinimalExtension(G, N);
    H.findMinimalExtensionApprox(G, K, mappings, N);
    return 0;
}

Graph* parseInput(string filename, int* N) {
    ifstream file(filename); 

    if (!file) { 
        cerr << "Error opening file!" << endl;
        return nullptr;
    }

    Graph G = createGraphFromFile(file);
    Graph H = createGraphFromFile(file);

    Graph* graphs = (Graph*)malloc(2 * sizeof(Graph));
    graphs[0] = G;
    graphs[1] = H;
    string line;
    getline(file, line);
    *N = stoi(line);
    return graphs;
}   

Graph createGraphFromFile(ifstream& file) {
    string line;
    getline(file, line);
    int nodes = stoi(line);
    int** edges = new int*[nodes];
    for(int i = 0; i < nodes; i++) {
        getline(file, line);
        istringstream iss(line);
        edges[i] = new int[nodes];
        string word;
        int j = 0;
        while(iss>>word) {
            edges[i][j++] = stoi(word);
        }
    }
    Graph graph(nodes, edges);
    return graph;
}

