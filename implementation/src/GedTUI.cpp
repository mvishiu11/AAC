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

    int tab_selector = 0;
    std::vector<std::string> headers{" G "," H ", " Result "}; 
    auto toggle = Renderer([&headers, &tab_selector]{
        ftxui::Elements elems;
        elems.push_back(ftxui::separator());
        for (int i = 0; i<headers.size(); i++) {
            if (i==tab_selector) {
                elems.push_back(text(headers[i])|bold);
                elems.push_back(ftxui::separator());
            } else {
                elems.push_back(text(headers[i])|color(Color::GrayDark));
                elems.push_back(ftxui::separator());
            }
        }
        return hbox(elems) | center;
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
        } else if (ev == Event::ArrowRight) {
            tab_selector = std::clamp(tab_selector+1,0, 3);
        } else if (ev == Event::ArrowLeft) {
            tab_selector = std::clamp(tab_selector-1,0, 3);
        }
        return false;
    });
    
    auto mapping = ftxui::Renderer([this]{
        ftxui::Elements elems;
        {
            elems.push_back(ftxui::text("G => H")|center);
            elems.push_back(separator());
        }
        for (int i = 0; i<result.mapping_this_to_other.size(); i++) {
            std::ostringstream t;
            if (result.mapping_this_to_other[i]<0) {
                t << " " << i+1 << " (removed) ";
                elems.push_back(ftxui::text(t.str())|color(Color::RedLight));
            } else {
                t << " " << i+1 << " => " << result.mapping_this_to_other[i]+1;
                elems.push_back(ftxui::text(t.str()));
            }
        }
        int offset=result.mapping_this_to_other.size();
        for (int i = 0; i<result.inserted_vertices_in_other.size(); i++) {
            std::ostringstream t;
            t << " " << i+1+offset << " => " << result.inserted_vertices_in_other[i]+1 << " (added) ";
            elems.push_back(ftxui::text(t.str())|color(Color::GreenLight));
        }
        elems.push_back(filler());
        return window(text(" Isomorphic mapping "), vbox(elems));
    });
    auto operations = ftxui::Renderer([this]{
        using Type = Graph::GedEditOp::Type;
        ftxui::Elements elems;
        int opind = 1;
        for (int i = 0; i<result.ops.size(); i++) {
            std::ostringstream t;
            ftxui::Color col = Color::White;
            auto &op = result.ops[i];
            switch (op.type) {
            case Graph::GedEditOp::Type::AddVertex:
                t << " " << opind << ") " << "add vertex " << op.from+1 << " ";
                col = Color::GreenLight;
                opind+=1;
                break;
            case Graph::GedEditOp::Type::DelVertex:
                t << " " << opind << ") " << "remove vertex " << op.from+1 << " ";
                col = Color::RedLight;
                opind+=1;
                break;
            case Graph::GedEditOp::Type::AddEdge:
                if (op.multiplicity>1) {
                    t << " " << opind << "-" << opind+op.multiplicity-1 <<  ") ";
                } else {
                    t << " " << opind << ") ";
                }
                opind += op.multiplicity;
                t << "add edge " << op.to+1 << "->" << op.from+1 <<", " << op.multiplicity << " times ";
                col = Color::GreenLight;
                break;
            case Graph::GedEditOp::Type::DelEdge:
                if (op.multiplicity>1) {
                    t << " " << opind << "-" << opind+op.multiplicity-1 <<  ") ";
                } else {
                    t << " " << opind << ") ";
                }
                opind += op.multiplicity;
                t << "remove edge " << op.to+1 << "->" << op.from+1 <<", " << op.multiplicity << " times ";
                col = Color::RedLight;
                break;
              break;
            }
            elems.push_back(ftxui::text(t.str())| color(col));
        }
        elems.push_back(filler());
        return window(text(" Edit Path "), vbox(elems));
    });
    auto stats = ftxui::Renderer([this]{
        std::string total = " Distance: " + std::to_string(result.total_cost) + " ";
        std::string vert = " Vertex operations: " + std::to_string(result.vertex_ops) + " ";
        std::string edge = " Edge operations: " + std::to_string(result.edge_ops) + " ";
        return window(text(" Information "), vbox({
            text(total) | color(Color::GreenLight),
            text(vert) | color(Color::White),
            text(edge) | color(Color::White),
            filler()
        }));
    });

    auto results = Renderer([&]{
        return hbox({
                mapping->Render() | flex,
                operations->Render() | flex,
                stats->Render() | flex,
            }) | center | xflex_grow;
    });
    auto renderer = Renderer([&] {
        return vbox({
            hbox(text(message) | color(message_color), filler(), indicator->Render()),
            separator(),
            Container::Tab({
                g,h, results
            }, &tab_selector)->Render(),
            filler(),
            separator(),
            toggle->Render()
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
