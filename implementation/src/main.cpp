#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <tuple>
#include "../include/Graph.hpp"

using namespace std;

Graph* parseInput(string filename, int* N);
Graph createGraphFromFile(ifstream& file);

int main(int argc, char** argv) {
    if(argc < 2) return -1;
    string filename = argv[1];
    int N;
    auto graphs = parseInput(filename, &N);
    Graph H = graphs[0];
    Graph G = graphs[1];
    cout<< N << endl;
    if(G.hasNSubgraphs(H, N)) {
        cout << "YES" << endl;
    } else {
        cout << "NO" << endl;
    }
    return 0;
}

Graph* parseInput(string filename, int* N) {
    ifstream file(filename); 

    if (!file) { 
        cerr << "Error opening file!" << endl;
        return nullptr;
    }

    Graph H = createGraphFromFile(file);
    Graph G = createGraphFromFile(file);

    Graph* graphs = (Graph*)malloc(2 * sizeof(Graph));
    graphs[0] = H;
    graphs[1] = G;
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