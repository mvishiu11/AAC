#include <ftxui/component/task.hpp>
#include <functional>
#include <thread>
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
#include <sstream>
#include <string>
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

TUI::TUI::TUI(Graph &G, Graph &H): G(G), H(H),
screen(ftxui::ScreenInteractive::TerminalOutput()), found_mappings(), created_mappings() {
    progress = true;
    //total = count_assignments(G.getVerticesCount(), H.getVerticesCount());
    extension = H.Edges();    
}

void TUI::TUI::alter(std::function<bool(TUI&)> f) {
    lock.lock();
    auto dirty = f(*this);
    lock.unlock();
    if (dirty) {
        redraw();
    }
}

void TUI::TUI::read(std::function<void(const TUI&)> f) {
    lock.lock();
    f(*this);
    lock.unlock();
}

void TUI::TUI::redraw() {
    screen.PostEvent(ftxui::Event::Character('r'));
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
    auto child = ftxui::Renderer([&label, &matrix, st]{
        auto m = Matrix(matrix, st);
        return window(ftxui::text(label),m.Render());
    }); 
    return ftxui::CatchEvent(child,[](ftxui::Event ev){return ev==ftxui::Event::Custom;});
}

ftxui::Component Isomorphisms(const TUI::TUI &state) {
    return ftxui::Renderer(
        [&state]{
            ftxui::Components children;
            for (auto v: state.found_mappings) {
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

void TUI::TUI::run() {
    using namespace ftxui;
    using namespace std::chrono_literals;
    std::string gLabel{"Adjacency matrix of G"};
    std::string hLabel{"Adjacency matrix of H"};
    std::string eLabel{"Adjacency matrix of H (extended)"};
    auto g = MatrixC(gLabel, G.Edges(), [](int x, int y){return color(Color::Green);});
    auto h = MatrixC(hLabel, H.Edges(), [](int x, int y){return color(Color::Green);});
    auto e = MatrixC(eLabel, extension, [this](int x, int y){return (H.Edges()[x][y]!=extension[x][y]) ? color(Color::Red) : color(Color::Green);});
    
    int selector = 1;


    auto colors = LinearGradient().Stop(Color::Red, 0.0).Stop(Color::Yellow, 0.5).Stop(Color::Green, 1.0);
    /*auto progress = Renderer([this, &colors]{
        return window(text("Constructing extension..."), hbox({
            gauge(this->progress) | color(colors),
            text(std::to_string(int(this->progress*100)))
        }));
    });*/
    auto isomorphisms = Isomorphisms(*this);
    auto outputTab = Container::Vertical({e});
    //auto tabs = Container::Tab({isomorphismsTab, inputTab, outputTab}, &selector);
    //auto container = Container::Vertical({toggle,tabs});
    bool original = false;
    auto matrices = Renderer([&]{
        const Component &hv = original ? h : e;
        return vbox({g->Render(), hv->Render()});
    });
    int frame = 0;
    auto indicator = Renderer([this, &colors,&frame]{
        if (!progress) {
            return text("");
        }
        return spinner(17,frame)|color(colors);
    });
    auto handler = CatchEvent([&](Event ev){
        if (ev == Event::Character(' ')) {
            original = !original;
            return true;
        }
        return false;
    });
    auto renderer = Renderer([&] {
        return vbox({
            hbox(text(message) | color(message_color), filler(), indicator->Render()),
            separator(),
            matrices->Render(),
            separator(),
            window(text("Isomorphic mappings"), isomorphisms->Render() | center)
        }) | border;
    });
    bool running = true;
    std::thread updater([this, &frame, &running]{
        while (running) {
            frame += 1;
            screen.RequestAnimationFrame();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });
    screen.Loop(renderer | handler);
    running = false;
}
