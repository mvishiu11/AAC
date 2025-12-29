#pragma once
#include "Graph.hpp"
#include <vector>
namespace TUI {
    struct State {
        Graph &G;
        Graph &H;
        std::vector<std::vector<int>> extension;
        std::vector<std::vector<int>> mappings;
        int current, total;
        std::string algorithm;
        State(Graph& G, Graph& H);
    };
    void TUI(State& state);
}