#pragma once
#include "Graph.hpp"
#include <ftxui/screen/color.hpp>
#include <functional>
#include <mutex>
#include <vector>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/deprecated.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/dom/selection.hpp>
#include <ftxui/dom/table.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/screen/string.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/component/component.hpp>
namespace TUI {
    struct IsoTUI {
        bool finished;
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
        IsoTUI(Graph& G, Graph& H);
        
    private:
        std::mutex lock;
        ftxui::ScreenInteractive screen;
    public:
        void redraw();
        void alter(std::function<bool (IsoTUI&)>);
        void read(std::function<void(const IsoTUI&)>);
        void run();
    };

    struct GedTUI {
        bool finished;
        Graph &G;
        Graph &H;
        Graph::GedResult result;
        bool progress;
        std::string algorithm;
        std::string message;
        ftxui::Color message_color;
        GedTUI(Graph& G, Graph& H);
        
    private:
        std::mutex lock;
        ftxui::ScreenInteractive screen;
    public:
        void redraw();
        void alter(std::function<bool (GedTUI&)>);
        void read(std::function<void(const GedTUI&)>);
        void run();
    };
    ftxui::Table Matrix(const std::vector<std::vector<int>> &matrix, std::function<ftxui::Decorator(int x, int y)> st);
    ftxui::Component MatrixC(std::string &label, const std::vector<std::vector<int>> &matrix, std::function<ftxui::Decorator(int x, int y)> st);
}