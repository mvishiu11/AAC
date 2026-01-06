#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <ftxui/screen/color.hpp>
#include <ostream>
#include <string>
#include <fstream>
#include <iostream>
#include <thread>
#include <unistd.h>
#include <vector>
#include "../include/Graph.hpp"
#include "../include/TUI.hpp"
#include "Hungarian.hpp"

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
    std::cout << "Usage: " << name << " <ged|iso> ..." << std::endl;
    exit(-1);
}

void usage_iso(char *name) {
    std::cout << "Usage: " << name << " iso <exact|approx> <path/to/input> <N>" << std::endl;
    exit(-1);
}

void usage_ged(char *name) {
    std::cout << "Usage: " << name << " ged <exact|approx> <path> <K>" << std::endl;
    std::cout << "<exact|approx> - should the program use exact or approximation algorithm?" << std::endl;
    std::cout << "<path> - path to the input file" << std::endl;
    std::cout << "<K> - the upper bound on the mappings checked by the approximation algorithm; ignored in case of the exact algorithm" << std::endl;
    exit(-1);
}

int ged(int argc, char **argv) {
        // New forms:
        //   main ged exact <file> [v] [p]
        //   main ged approx <file> <K> [v] [p]
        // Backward-compatible:
        //   main ged <file> <K> [v] [p]  == approx

        std::string sub = argv[2];
        std::string filename;
        int K = 0;
        CliFlags flags;
        flags.path = true;
        flags.verbose = true;

        if (sub == "exact")
        {
            if (argc < 4) usage_ged(argv[0]);
            filename = argv[3];
            //flags = parseFlags(argc, argv, 4);
        }
        else if (sub == "approx")
        {
            if (argc < 5) usage_ged(argv[0]);
            filename = argv[3];
            K = stoi(argv[4]);
            //flags = parseFlags(argc, argv, 5);
        }
        else
        {
            // backward-compatible approx
            if (argc < 4) usage_ged(argv[0]);
            filename = argv[2];
            K = stoi(argv[3]);
            flags = parseFlags(argc, argv, 4);
            sub = "approx";
        }

        const bool verbose = flags.verbose;
        auto graphs = parseInput(filename);
        Graph G = graphs.first;
        Graph H = graphs.second;

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

int iso(int argc, char **argv) {
    if (argc < 5) usage_iso(argv[0]);

    string mode = argv[2];
    string filename = argv[3];

    auto graphs = parseInput(filename);
    Graph G = graphs.first;
    Graph H = graphs.second;
    bool verbose = true;
    int N = std::stoi(argv[4]);
    if (N < 1)
    {
        cout << "N needs to be >=1!" << endl;
        exit(-1);
    }

    TUI::IsoTUI state(G,H);
    std::thread tui_thread([&state]{
        state.run();
        exit(0);
    });
    int K = maxK(H.getVerticesCount(), G.getVerticesCount());
    K = std::min(H.getVerticesCount() * G.getVerticesCount(), K);
    K = std::min(50 * N, K);

    auto push_mapping = [&state](const Mapping& m) {
            state.alter([&m](TUI::IsoTUI &tui){
                tui.found_mappings.push_back(m);
                return true;
            });
        };
    int times = 0;
    auto replace_extension = [&state, &times](const std::vector<Mapping>& mp, const Graph::EdgeMatrix& em, int cost) {
            state.alter([&mp,&em,&cost,&times](TUI::IsoTUI &tui){
                for (int i = 0; i < tui.extension.size(); i++) {
                    for (int j = 0; j < tui.extension[i].size(); j++) {
                        tui.extension[i][j] = tui.H.Edges()[i][j]+em[i][j];
                    }
                }
                tui.extension_cost = cost;
                //tui.created_mappings = mp;
                times+=1;
                tui.found_mappings = mp;
                return true;
            });
        };
    if (mode == "exact")
    {
        //std::cout<<"\n ===== EXACT ALGORITHMS ===== \n"<<endl;
        state.alter([](TUI::IsoTUI &tui){
            tui.message = "Searching for mappings that form an isomorphism..."; 
                tui.message_color = ftxui::Color::Yellow;
            return true;
        });
        if (H.hasNSubgraphs(G, N, push_mapping))
        {
            state.alter([N](TUI::IsoTUI &tui){
                tui.message = "Found all " + std::to_string(N) + " isomorphic mappings!"; 
                tui.message_color = ftxui::Color::GreenLight;
                tui.progress = false;
                return true;
            });
        }
        else
        {
            state.alter([N](TUI::IsoTUI &tui){
                tui.message = "Searching for minimal extension... (below shown is current best)"; 
                tui.message_color = ftxui::Color::Yellow;
                return true;
            });
            //std::cout << "EXACT: NO" << endl;
            H.findMinimalExtension(G, N, replace_extension);
            state.alter([N](TUI::IsoTUI &tui){
                tui.message = "Extension found!";
                tui.message_color = ftxui::Color::GreenLight;
                tui.progress = false;
                tui.finished = true;
                return true;
            });
        }
    }

    if (mode == "approx")
    {
        state.alter([K](TUI::IsoTUI &tui){
            tui.message = "Selecting " + std::to_string(K) + " most promising mappings..."; 
            tui.message_color = ftxui::Color::Yellow;
            return true;
        });
        const std::vector<Mapping> mappings = H.selectMappings(G, K, verbose);
        if (H.hasNSubgraphsApprox(G, K, mappings, N, push_mapping))
        {
            state.alter([N](TUI::IsoTUI &tui){
                tui.message = "Found all " + std::to_string(N) + " isomorphic mappings!"; 
                tui.message_color = ftxui::Color::GreenLight;
                tui.progress = false;
                return true;
            });
        }
        else
        {
            state.alter([N](TUI::IsoTUI &tui){
                tui.message = "Found " + std::to_string(tui.found_mappings.size()) + " isomorphic mappings; searching for minimal extension..."; 
                tui.message_color = ftxui::Color::Yellow;
                return true;
            });
            H.findMinimalExtensionApprox(G, K, mappings, N, replace_extension);
            state.alter([N](TUI::IsoTUI &tui){
                tui.message = "Extension found!";
                tui.message_color = ftxui::Color::GreenLight;
                tui.progress = false;
                tui.finished = true;
                return true;
            });
        }
    }

    tui_thread.join();
    exit(0);
}

int main(int argc, char **argv)
{
    if (argc < 2) usage(argv[0]);;

    string alg = argv[1];
    if (alg == "ged") {
        return ged(argc, argv);
    } else if (alg == "iso") {
        return iso(argc, argv);
    }
    
    usage(argv[0]);
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
