#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <string>

// Configuration
struct Config {
    int nodesG;
    int nodesH;
    double density;
    int maxMultiplicity;
    int seed;
};

void writeGraph(std::ofstream& out, int nodes, const std::vector<std::vector<int>>& adj) {
    out << nodes << "\n";
    for (int i = 0; i < nodes; ++i) {
        for (int j = 0; j < nodes; ++j) {
            out << adj[i][j] << (j == nodes - 1 ? "" : " ");
        }
        out << "\n";
    }
}

int main(int argc, char* argv[]) {
    if (argc < 6) {
        std::cerr << "Usage: ./generator <nodesG> <nodesH> <density> <maxMultiplicity> <output_file>\n";
        return 1;
    }

    Config cfg;
    cfg.nodesG = std::stoi(argv[1]);
    cfg.nodesH = std::stoi(argv[2]);
    cfg.density = std::stod(argv[3]);
    cfg.maxMultiplicity = std::stoi(argv[4]);
    std::string filename = argv[5];

    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    std::uniform_int_distribution<int> multDist(1, cfg.maxMultiplicity);

    // Generate G
    std::vector<std::vector<int>> adjG(cfg.nodesG, std::vector<int>(cfg.nodesG, 0));
    for (int i = 0; i < cfg.nodesG; ++i) {
        for (int j = i; j < cfg.nodesG; ++j) { // Undirected logic, but filling full matrix
            if (dist(rng) < cfg.density) {
                int w = multDist(rng);
                adjG[i][j] = w;
                adjG[j][i] = w;
            }
        }
    }

    // Generate H
    std::vector<std::vector<int>> adjH(cfg.nodesH, std::vector<int>(cfg.nodesH, 0));
    for (int i = 0; i < cfg.nodesH; ++i) {
        for (int j = i; j < cfg.nodesH; ++j) {
            if (dist(rng) < cfg.density) {
                int w = multDist(rng);
                adjH[i][j] = w;
                adjH[j][i] = w;
            }
        }
    }

    // Write to file
    std::ofstream file(filename);
    if (!file.is_open()) return -1;

    writeGraph(file, cfg.nodesG, adjG);
    writeGraph(file, cfg.nodesH, adjH);
    
    file.close();
    return 0;
}
