#include <algorithm>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include "../include/Graph.hpp"

using namespace std;

Graph *parseInput(string filename);
Graph createGraphFromFile(ifstream &file);

int maxK(int h, int g)
{
    // (h choose g)g!
    // = h!/(h-g)!
    // = h*(h-1)*...*(h-g+2)*(h-g+1)
    if (g > h)
        return 0;
    if (g == 0)
        return 1;
    long long result = 1;
    for (int i = 0; i < g; ++i)
    {
        result *= (h - i);
        if (result > 2147483647) return 2147483647;
    }
    return (int)result;
}

int main(int argc, char **argv)
{
    if (argc < 4)
        return -1;
    string algorithm = argv[1];
    string filename = argv[2];
    int N = stoi(argv[3]);
    if(algorithm!="exact" && algorithm!="approx"){
        cout<<"Wrong algorithm option provided!"<<endl;
        cout<<algorithm<<endl;
        return -1;
    }
    if(N<1){
        cout<<"N needa to be >1!"<<endl;
        return -1;
    }
    auto graphs = parseInput(filename);
    Graph G = graphs[0];
    Graph H = graphs[1];

    int mK = maxK(H.getVerticesCount(), G.getVerticesCount());
    int K = H.getVerticesCount()*G.getVerticesCount();
    K = std::min(K, mK); // In case our K is greater than maximum number of possible mappings

    cout << "### SELECT MAPPINGS ###" << endl;
    const std::vector<Mapping> mappings = H.selectMappings(G, K);

    if (algorithm == "exact")
    {
        cout<<"\n ===== EXACT ALGORITHMS ===== \n"<<endl;
        if (H.hasNSubgraphs(G, N))
        {
            cout << "EXACT: YES" << endl;
        }
        else
        {
            cout << "EXACT: NO" << endl;
            H.findMinimalExtension(G, N);
        }
    }

    if (algorithm == "approx")
    {
        cout<<"\n ===== APPROXIMATE ALGORITHMS ===== \n"<<endl;
        if (H.hasNSubgraphsApprox(G, K, mappings, N))
        {
            cout << "APPROXIMATION: YES" << endl;
        }
        else
        {
            cout << "APPROXIMATION: NO" << endl;
            H.findMinimalExtensionApprox(G, K, mappings, N);
        }
    }

    return 0;
}

Graph *parseInput(string filename)
{
    ifstream file(filename);

    if (!file)
    {
        cerr << "Error opening file!" << endl;
        return nullptr;
    }

    Graph G = createGraphFromFile(file);
    Graph H = createGraphFromFile(file);

    Graph *graphs = (Graph *)malloc(2 * sizeof(Graph));
    graphs[0] = G;
    graphs[1] = H;
    string line;
    return graphs;
}

Graph createGraphFromFile(ifstream &file)
{
    string line;
    getline(file, line);
    int nodes = stoi(line);
    int **edges = new int *[nodes];
    for (int i = 0; i < nodes; i++)
    {
        getline(file, line);
        istringstream iss(line);
        edges[i] = new int[nodes];
        string word;
        int j = 0;
        while (iss >> word)
        {
            edges[i][j++] = stoi(word);
        }
    }
    Graph graph(nodes, edges);
    return graph;
}
