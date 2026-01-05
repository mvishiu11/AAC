#pragma once
#include "Graph.hpp"
#include <ftxui/screen/color.hpp>
#include <functional>
#include <mutex>
#include <vector>
#include <ftxui/component/screen_interactive.hpp>
namespace TUI {
    struct TUI {
        Graph &G;
        Graph &H;
        std::vector<std::vector<int>> extension;
        int extension_cost;
        std::vector<std::vector<int>> found_mappings;
        std::vector<std::vector<int>> created_mappings;
        bool progress;
        std::string algorithm;
        std::string message;
        ftxui::Color message_color;
        TUI(Graph& G, Graph& H);
        
    private:
        std::mutex lock;
        ftxui::ScreenInteractive screen;
    public:
        void redraw();
        void alter(std::function<bool (TUI&)>);
        void read(std::function<void(const TUI&)>);
        void run();
    };
}