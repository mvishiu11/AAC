#include <algorithm>
#include <cstddef>
#include <ostream>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include "../include/Graph.hpp"
#include "../include/TUI.hpp"

using namespace std;

std::pair<Graph, Graph> parseInput(string filename);
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

void usage(char *name) {
    std::cout << "Usage: " << name << " <exact|approx> <path/to/input> <N>" << std::endl;
    exit(-1);
}

int main(int argc, char **argv)
{
    if (argc < 3) return -1;

    string mode = argv[1];
    if (mode == "ged")
    {
        // New forms:
        //   main ged exact <file> [v] [p]
        //   main ged approx <file> <K> [v] [p]
        // Backward-compatible:
        //   main ged <file> <K> [v] [p]  == approx

        std::string sub = argv[2];
        std::string filename;
        int K = 0;
        CliFlags flags;

        if (sub == "exact")
        {
            if (argc < 4) return -1;
            filename = argv[3];
            flags = parseFlags(argc, argv, 4);
        }
        else if (sub == "approx")
        {
            if (argc < 5) return -1;
            filename = argv[3];
            K = stoi(argv[4]);
            flags = parseFlags(argc, argv, 5);
        }
        else
        {
            // backward-compatible approx
            if (argc < 4) return -1;
            filename = argv[2];
            K = stoi(argv[3]);
            flags = parseFlags(argc, argv, 4);
            sub = "approx";
        }

        const bool verbose = flags.verbose;
        auto graphs = parseInput(filename);
        Graph G = graphs[0];
        Graph H = graphs[1];

        if (sub == "exact")
        {
            cout << "\n ===== GRAPH EDIT DISTANCE (EXACT) ===== \n" << endl;
            auto r = G.gedExact(H, flags.path, verbose);

            cout << "GED: " << r.total_cost
                 << " (vertex_ops=" << r.vertex_ops
                 << ", edge_ops=" << r.edge_ops << ")\n";
            cout << "Mapping (G -> H, -1 means deleted): " << r.mapping_this_to_other << "\n";
            cout << "Exact: complete=" << (r.complete ? "true" : "false")
                 << " states_visited=" << r.states_visited
                 << " states_pruned=" << r.states_pruned << "\n";

            if (!r.inserted_vertices_in_other.empty())
                cout << "Inserted vertices in H: " << r.inserted_vertices_in_other << "\n";
            if (!r.deleted_vertices_in_this.empty())
                cout << "Deleted vertices in G: " << r.deleted_vertices_in_this << "\n";

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

        // approx
        if (K < 1) K = 1;

        // Hard upper bound on #injective mappings used by the approximation.
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
            cout << "Inserted vertices in H: " << r.inserted_vertices_in_other << "\n";
        if (!r.deleted_vertices_in_this.empty())
            cout << "Deleted vertices in G: " << r.deleted_vertices_in_this << "\n";

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

    if (argc < 4) return -1;

    string filename = argv[2];
    int third = stoi(argv[3]);
    const CliFlags flags = parseFlags(argc, argv, 4);
    const bool verbose = flags.verbose;

    auto graphs = parseInput(filename);
    Graph G = graphs[0];
    Graph H = graphs[1];
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

    auto graphs = parseInput(filename);
    Graph G = graphs.first;
    Graph H = graphs.second;
    TUI::State state(G,H);
    TUI::TUI(state);
    return 0;
    int K = maxK(H.getVerticesCount(), G.getVerticesCount());
    K = std::min(H.getVerticesCount() * G.getVerticesCount(), K);
    K = std::min(50 * N, K);

    if (mode == "exact")
    {
        //std::cout<<"\n ===== EXACT ALGORITHMS ===== \n"<<endl;
        if (H.hasNSubgraphs(G, N, verbose))
        {
            //std::cout << "EXACT: YES" << endl;
        }
        else
        {
            //std::cout << "EXACT: NO" << endl;
            H.findMinimalExtension(G, N);
        }
    }

    if (mode == "approx")
    {
        if (verbose) {
            //std::cout << "### SELECT MAPPINGS ###" << endl;
        }
        const std::vector<Mapping> mappings = H.selectMappings(G, K, verbose);

        //std::cout<<"\n ===== APPROXIMATE ALGORITHMS ===== \n"<<endl;
        if (H.hasNSubgraphsApprox(G, K, mappings, N, verbose))
        {
            //std::cout << "APPROXIMATION: YES" << endl;
        }
        else
        {
            //std::cout << "APPROXIMATION: NO" << endl;
            H.findMinimalExtensionApprox(G, K, mappings, N, verbose);
        }
    }

    return 0;
}

std::pair<Graph,Graph> parseInput(string filename)
{
    ifstream file(filename);

    if (!file)
    {
        cerr << "Error opening file!" << endl;
        exit(-1);
    }

    Graph G = createGraphFromFile(file);
    Graph H = createGraphFromFile(file);
    return {G, H};
}

Graph createGraphFromFile(ifstream &file)
{
    //getline(file, line);
    int nodes;// = stoi(line);
    file >> nodes;
    std::vector<std::vector<int>> edges(static_cast<size_t>(nodes), std::vector<int>(nodes, 0));
    for (int i = 0; i < nodes; i++)
    {
        string word;
        int j = 0;
        while (j < nodes)
        {
            file >> edges[i][j];
            j += 1;
        }
    }
    Graph graph(nodes, edges);
    return graph;
}
