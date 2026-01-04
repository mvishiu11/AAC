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

struct CliFlags {
    bool verbose = false;
    bool path = false;
};

CliFlags parseFlags(int argc, char **argv, int startIndex)
{
    CliFlags f;
    for (int i = startIndex; i < argc; ++i)
    {
        std::string s = argv[i];
        if (s == "v" || s.find('v') != std::string::npos) f.verbose = true;
        if (s == "p" || s == "path" || s.find('p') != std::string::npos) f.path = true;
    }
    return f;
}

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
        if (result > 2000000000/(h-i)) return 2000000000;
        result *= (h - i);
    }
    return (int)result;
}

int main(int argc, char **argv)
{
    if (argc < 4) return -1;
        string mode = argv[1];
        string filename = argv[2];
        int third = stoi(argv[3]);
        const CliFlags flags = parseFlags(argc, argv, 4);
        const bool verbose = flags.verbose;
    auto graphs = parseInput(filename);
    Graph G = graphs[0];
    Graph H = graphs[1];
    if (mode == "ged")
    {
        int K = third;
        if (K < 1) K = 1;

        // Hard upper bound on #injective mappings
        const int smallN = std::min(G.getVerticesCount(), H.getVerticesCount());
        const int largeN = std::max(G.getVerticesCount(), H.getVerticesCount());
        const int maxInjective = maxK(largeN, smallN);
        K = std::min(K, maxInjective);

        cout << "\n ===== GRAPH EDIT DISTANCE (APPROX) ===== \n" << endl;
        auto r = G.gedApprox(H, K, flags.path, verbose);

        cout << "GED: " << r.total_cost
             << " (vertex_ops=" << r.vertex_ops
             << ", edge_ops=" << r.edge_ops << ")\n";
        cout << "Mapping (G -> H, -1 means deleted): " << r.mapping_this_to_other << "\n";

        if (!r.inserted_vertices_in_other.empty())
        {
            cout << "Inserted vertices in H: " << r.inserted_vertices_in_other << "\n";
        }
        if (!r.deleted_vertices_in_this.empty())
        {
            cout << "Deleted vertices in G: " << r.deleted_vertices_in_this << "\n";
        }

        if (flags.path)
        {
            cout << "\n--- Edit path (aggregated) ---\n";
            for (const auto &op : r.ops)
            {
                using T = Graph::GedEditOp::Type;
                switch (op.type)
                {
                case T::AddVertex: cout << "addv(" << op.from << ")\n"; break;
                case T::DelVertex: cout << "delv(" << op.from << ")\n"; break;
                case T::AddEdge:   cout << "adde(" << op.from << " -> " << op.to << ") x" << op.multiplicity << "\n"; break;
                case T::DelEdge:   cout << "dele(" << op.from << " -> " << op.to << ") x" << op.multiplicity << "\n"; break;
                }
            }
        }
        return 0;
    }
    if (mode != "exact" && mode != "approx")
    {
        cout << "Wrong mode provided! Use: exact | approx | ged\n";
        cout << mode << endl;
        return -1;
    }

    int N = third;
    if (N < 1)
    {
        cout << "N needa to be >1!" << endl;
        return -1;
    }

    int K = maxK(H.getVerticesCount(), G.getVerticesCount());
    K = std::min(H.getVerticesCount() * G.getVerticesCount(), K);
    K = std::min(50 * N, K);

    if (mode == "exact")
    {
        cout<<"\n ===== EXACT ALGORITHMS ===== \n"<<endl;
        if (H.hasNSubgraphs(G, N, verbose))
        {
            cout << "EXACT: YES" << endl;
        }
        else
        {
            cout << "EXACT: NO" << endl;
            H.findMinimalExtension(G, N);
        }
    }

    if (mode == "approx")
    {
        if (verbose) {
            cout << "### SELECT MAPPINGS ###" << endl;
        }
        const std::vector<Mapping> mappings = H.selectMappings(G, K, verbose);

        cout<<"\n ===== APPROXIMATE ALGORITHMS ===== \n"<<endl;
        if (H.hasNSubgraphsApprox(G, K, mappings, N, verbose))
        {
            cout << "APPROXIMATION: YES" << endl;
        }
        else
        {
            cout << "APPROXIMATION: NO" << endl;
            H.findMinimalExtensionApprox(G, K, mappings, N, verbose);
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
