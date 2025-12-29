#define UNICODE
#include "TUI.hpp"
#include "Graph.hpp"
#include <algorithm>
#include <cstdlib>
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
#include <iostream>
#include <new>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace {
    std::string padLeft(std::string source, std::size_t totalWidth, char paddingChar = ' ') {
        if (totalWidth > source.size()) {
            source.insert(0, totalWidth - source.size(), paddingChar);
        }
        return source;
    }
    std::string padRight(std::string source, std::size_t totalWidth, char paddingChar = ' ') {
        if (totalWidth > source.size()) {
            source.insert(source.size(), totalWidth - source.size(), paddingChar);
        }
        return source;
    }
    std::string pad(std::string source, std::size_t totalWidth, char paddingChar = ' ') {
        if (totalWidth > source.size()) {
            std::size_t totalPadWidth = totalWidth - source.size();
            std::size_t rightPadWidth = (totalPadWidth / 2) + source.size();
            source = padLeft(padRight(source, rightPadWidth, paddingChar), totalWidth, paddingChar);
        }
        return source;
    }
}

ftxui::Table Matrix(const std::vector<std::vector<int>> &matrix, std::function<ftxui::Decorator(int x, int y)> st) {
    using namespace ftxui;
    int max = 0;
    for (int i = 0; i<matrix.size(); i++) {
        for (int j = 0; j<matrix.size(); j++) {
            max = std::max(max, matrix[i][j]);
        }
    }
    int max_len = std::to_string(max).size();
    auto number = [&max_len](const int &i){
            return paragraphAlignCenter(padLeft(std::to_string(i), 0*max_len));
        };
    std::vector<std::vector<ftxui::Element>> cells;
    for (int x = 0; x < matrix.size(); x++) {
        cells.emplace_back();
        for (int y = 0; y < matrix.size(); y++) {
            cells[x].push_back(number(matrix[x][y]) | center | flex | st(x,y));
        }
    }
    return Table(cells);
}

ftxui::Component MatrixC(std::string &label, const std::vector<std::vector<int>> &matrix, std::function<ftxui::Decorator(int x, int y)> st) {
    return ftxui::Renderer([&label, &matrix, &st]{
        auto m = Matrix(matrix, st);
        return window(ftxui::text(label),m.Render());
    });
}

ftxui::Component Isomorphisms(const TUI::State &state) {
    return ftxui::Renderer(
        [&state]{
            ftxui::Components children;
            for (auto v: state.mappings) {
                children.push_back(ftxui::Renderer(
                    [v]{
                        ftxui::Elements elems;
                        for (int i = 0; i<v.size(); i++) {
                            std::ostringstream t;
                            t << "G[" << i+1 << "]=>H[" << v[i]+1 << "]";
                            elems.push_back(ftxui::text(t.str()));
                        }
                        return ftxui::vbox(elems)|ftxui::border;}));
            }
            return ftxui::Container::Horizontal(std::move(children))->Render();
        }
    );
}

TUI::State::State(Graph &G, Graph &H): G(G), H(H) {
    current = 100;
    total = 100;
    extension = H.Edges();
    for (int i = 0; i<H.getVerticesCount(); i++)
        for (int j = 0; j<H.getVerticesCount(); j++) {
            if (rand()%5==0) {
                extension[i][j] += 1;
            }
    }
    mappings = std::vector<std::vector<int>>{
        {1, 2, 3},
        {3, 0, 1},
        {4,2,0}
    };
}

void TUI::TUI(State & state) {
    using namespace ftxui;
    using namespace std::chrono_literals;
    std::string gLabel{"Adjacency matrix of G"};
    std::string hLabel{"Adjacency matrix of H"};
    std::string eLabel{"Adjacency matrix of H (extended)"};
    auto g = MatrixC(gLabel, state.G.Edges(), [](int x, int y){return color(Color::Green);});
    auto h = MatrixC(hLabel, state.H.Edges(), [](int x, int y){return color(Color::Green);});
    auto e = MatrixC(eLabel, state.extension, [&state](int x, int y){return (state.H.Edges()[x][y]!=state.extension[x][y]) ? color(Color::Red) : color(Color::Green);});
    
    int selector = 0;
    std::vector<std::string> tab_headers{
      "Input", "Isomorphisms", "Extension"
    };
    auto toggle = Toggle(tab_headers, &selector)|center|flex;
    int frame = 0;
    auto spin = Renderer([&frame]{
        return spinner(0, frame);
    });
    auto inputTab = Container::Vertical({h, g});
    auto colors = LinearGradient().Stop(Color::Red, 0.0).Stop(Color::Yellow, 0.5).Stop(Color::Green, 1.0);
    auto progress = Renderer([&state, &colors]{
        return window(text("Constructing extension..."), hbox({
            gauge(float(state.current)/state.total) | color(colors),
            text(std::to_string(state.current)+"/"+std::to_string(state.total))
        }));
    });
    auto isomorphismsTab = Isomorphisms(state);
    auto outputTab = Container::Vertical({e, progress});
    auto tabs = Container::Tab({inputTab, isomorphismsTab, outputTab }, &selector);
    auto container = Container::Vertical({toggle,tabs});
    auto renderer = Renderer(container, [&] {
        return vbox({
                    toggle->Render(),
                    separator(),
                    tabs->Render(),
                }) |
                border;
        });
    auto screen = ScreenInteractive::TerminalOutput();
    screen.Loop(renderer);
}