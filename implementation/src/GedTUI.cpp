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

TUI::GedTUI::GedTUI(Graph &G, Graph &H): G(G), H(H),
screen(ftxui::ScreenInteractive::Fullscreen()), result() {
    progress = true;
}

void TUI::GedTUI::alter(std::function<bool(GedTUI&)> f) {
    lock.lock();
    auto dirty = f(*this);
    lock.unlock();
    if (dirty) {
        redraw();
    }
}

void TUI::GedTUI::read(std::function<void(const GedTUI&)> f) {
    lock.lock();
    f(*this);
    lock.unlock();
}

void TUI::GedTUI::redraw() {
    screen.PostEvent(ftxui::Event::Character('r'));
}

void TUI::GedTUI::run() {
    using namespace ftxui;
    using namespace std::chrono_literals;
    std::string gLabel{" Adjacency matrix of G "};
    std::string hLabel{" Adjacency matrix of H "};
    std::string eLabel{" Adjacency matrix of extension of H "};
    auto g = MatrixC(gLabel, G.Edges(), [](int x, int y){return color(Color::Green);});
    auto h = MatrixC(hLabel, H.Edges(), [](int x, int y){return color(Color::Green);});
    auto colors = LinearGradient().Stop(Color::Red, 0.0).Stop(Color::Yellow, 0.5).Stop(Color::Green, 1.0);
    

    bool original = false;
    auto matrices = Renderer([&]{
        return vbox({g->Render(), h->Render()});
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
        } else if (ev == Event::q) {
            screen.Exit();
            if (!finished) {
                std::cout << std::endl << "Aborted!" << std::endl;
            }
            exit(0);
        }
        return false;
    });

    auto mapping = ftxui::Renderer([this]{
        ftxui::Elements elems;
        {
            elems.push_back(ftxui::text("G => H")|center);
        }
        for (int i = 0; i<result.mapping_this_to_other.size(); i++) {
            std::ostringstream t;
            t << i+1 << " => " << result.mapping_this_to_other[i]+1;
            elems.push_back(ftxui::text(t.str())|center);
        }
        elems.push_back(filler());
        return window(text(" Isomorphic mapping "), vbox(elems));
    });
    auto operations = ftxui::Renderer([this]{
        using Type = Graph::GedEditOp::Type;
        ftxui::Elements elems;
        for (int i = 0; i<result.ops.size(); i++) {
            std::ostringstream t;
            ftxui::Color col = Color::White;
            //t << "G[" << i+1 << "]=>H[" << v[i]+1 << "]";
            auto &op = result.ops[i];
            switch (op.type) {
            case Graph::GedEditOp::Type::AddVertex:
                t << "add vertex " << op.from;
                col = Color::GreenLight;
                break;
            case Graph::GedEditOp::Type::DelVertex:
                t << "remove vertex " << op.from;
                col = Color::RedLight;
                break;
            case Graph::GedEditOp::Type::AddEdge:
                t << "add edge " << op.from << "->" << op.to <<", " << op.multiplicity << " times";
                col = Color::GreenLight;
                break;
            case Graph::GedEditOp::Type::DelEdge:
                t << "remove edge " << op.from << "->" << op.to <<", " << op.multiplicity << " times";
                col = Color::RedLight;
                break;
              break;
            }
            elems.push_back(ftxui::text(t.str())| color(col) |center);
        }
        elems.push_back(filler());
        return window(text(" Edit Path "), vbox(elems));
    });
    auto stats = ftxui::Renderer([this]{
        std::string total = "Distance: " + std::to_string(result.total_cost);
        std::string vert = "Vertex operations: " + std::to_string(result.vertex_ops);
        std::string edge = "Edge operations: " + std::to_string(result.edge_ops);
        return window(text(" Information "), vbox({
            text(total) | color(Color::GreenLight),
            text(vert) | color(Color::White),
            text(edge) | color(Color::White),
            filler()
        }));
    });
    auto renderer = Renderer([&] {
        return vbox({
            hbox(text(message) | color(message_color), filler(), indicator->Render()),
            separator(),
            matrices->Render(),
            hbox({
                mapping->Render() | flex,
                operations->Render() | flex,
                stats->Render() | flex,
            }) | center | xflex_grow
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
    updater.join();
}
