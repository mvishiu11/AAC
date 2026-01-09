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
        if (result > 200000000/(h-i)) return 200000000;
        result *= (h - i);
    }
    return (int)result;
}

void usage(char *name) {
    std::cout << "Usage: " << name << " <ged|iso> ..." << std::endl;
    exit(-1);
}

void usage_iso(char *name) {
    std::cout << "Usage: " << name << " iso <exact|approx> <path> <N>" << std::endl;
    std::cout << "<exact|approx> - should the program use exact or approximation algorithm?" << std::endl;
    std::cout << "<path> - path to the input file" << std::endl;
    std::cout << "<N> - the number of subgraph isomorphisms to search for" << std::endl;
    exit(-1);
}

void usage_ged(char *name) {
    std::cout << "Usage: " << name << " ged <exact|approx> <path>" << std::endl;
    std::cout << "<exact|approx> - should the program use exact or approximation algorithm?" << std::endl;
    std::cout << "<path> - path to the input file" << std::endl;
    exit(-1);
}

int ged(int argc, char **argv) {
    std::string sub = argv[2];
    std::string filename;
    
    if (sub == "exact")
    {
        if (argc < 4) usage_ged(argv[0]);
        filename = argv[3];
    }
    else if (sub == "approx")
    {
        if (argc < 4) usage_ged(argv[0]);
        filename = argv[3];
    }
    else
    {
        // backward-compatible approx
        if (argc < 3) usage_ged(argv[0]);
        filename = argv[2];
        sub = "approx";
    }

    auto graphs = parseInput(filename);
    Graph G = graphs.first;
    Graph H = graphs.second;

    int K = 50 * std::max(G.getVerticesCount(), H.getVerticesCount());
    TUI::GedTUI state(G,H);
    std::thread tui_thread([&state]{
        state.run();
    });
    if (sub == "exact")
    {
        auto update_state = [&state](const Graph::GedResult& result) {
            state.alter([&result](TUI::GedTUI &tui) {
                tui.result = result;
                return true;
            });
        };
        state.alter([](TUI::GedTUI &tui) {
            tui.message = "Calculating distance...";
            tui.message_color = ftxui::Color::Yellow;
            return true;
        });
        int maxStates = 20*(H.getVerticesCount()+H.getVerticesCount()*(H.getVerticesCount()-1)/2);
        auto r = G.gedExact(H, true, maxStates, update_state);
        state.alter([&r](TUI::GedTUI &tui) {
            tui.result = r;
            tui.progress = false;
            tui.finished = true;
            tui.message = "Distance found!";
            tui.message_color = ftxui::Color::GreenLight;
            return true;
        });
        tui_thread.join();
        return 0;
    }

    // approx
    if (K < 1) K = 1;

    // Hard upper bound on #injective mappings used by the approximation.
    const int smallN = std::min(G.getVerticesCount(), H.getVerticesCount());
    const int largeN = std::max(G.getVerticesCount(), H.getVerticesCount());
    const int maxInjective = maxK(largeN, smallN);
    K = std::min(K, maxInjective);
    state.alter([](TUI::GedTUI &tui) {
        tui.message = "Approximating distance...";
        tui.message_color = ftxui::Color::Yellow;
        return true;
    });
    auto update_state = [&state](const Graph::GedResult& result) {
        state.alter([&result](TUI::GedTUI &tui) {
            tui.result = result;
            return true;
        });
    };

    auto r = G.gedApprox(H, K, true, update_state);
    state.alter([&r](TUI::GedTUI &tui) {
        tui.result = r;
        tui.progress = false;
        tui.finished = true;
        tui.message = "Distance found!";
        tui.message_color = ftxui::Color::GreenLight;
        return true;
    });
    tui_thread.join();
    exit(0);
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
