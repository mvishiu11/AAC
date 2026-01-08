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

TUI::IsoTUI::IsoTUI(Graph &G, Graph &H): G(G), H(H),
screen(ftxui::ScreenInteractive::Fullscreen()), found_mappings(), created_mappings() {
    progress = true;
    //total = count_assignments(G.getVerticesCount(), H.getVerticesCount());
    extension = H.Edges();    
}

void TUI::IsoTUI::alter(std::function<bool(IsoTUI&)> f) {
    lock.lock();
    auto dirty = f(*this);
    lock.unlock();
    if (dirty) {
        redraw();
    }
}

void TUI::IsoTUI::read(std::function<void(const IsoTUI&)> f) {
    lock.lock();
    f(*this);
    lock.unlock();
}

void TUI::IsoTUI::redraw() {
    screen.PostEvent(ftxui::Event::Character('r'));
}



ftxui::Table TUI::Matrix(const std::vector<std::vector<int>> &matrix, std::function<ftxui::Decorator(int x, int y)> st) {
    using namespace ftxui;
    if (matrix.size()>50 || matrix[0].size()>50) {
        return Table(std::vector<std::vector<ftxui::Element>>{{text("Matrix larger than 50 in either dimension - not displaying it for readability")}});
    }
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

ftxui::Component TUI::MatrixC(std::string &label, const std::vector<std::vector<int>> &matrix, std::function<ftxui::Decorator(int x, int y)> st) {
    auto child = ftxui::Renderer([&label, &matrix, st]{
        auto m = Matrix(matrix, st);
        return window(ftxui::text(label),ftxui::vbox({m.Render(), ftxui::filler()}));
    }); 
    return ftxui::CatchEvent(child,[](ftxui::Event ev){return ev==ftxui::Event::Custom;});
}

void TUI::IsoTUI::run() {
    using namespace ftxui;
    using namespace std::chrono_literals;
    std::string gLabel{" Adjacency matrix of G "};
    std::string hLabel{" Adjacency matrix of H "};
    std::string eLabel{" Adjacency matrix of extension of H "};
    auto g = MatrixC(gLabel, G.Edges(), [](int x, int y){return color(Color::Green);});
    auto h = MatrixC(hLabel, H.Edges(), [](int x, int y){return color(Color::Green);});
    auto e = MatrixC(eLabel, extension, [this](int x, int y){return (H.Edges()[x][y]!=extension[x][y]) ? color(Color::Red) : color(Color::Green);});
    
    int selector = 1;


    auto colors = LinearGradient().Stop(Color::Red, 0.0).Stop(Color::Yellow, 0.5).Stop(Color::Green, 1.0);
    float scroll_x;
    auto isomorphisms = ftxui::Renderer([this,&scroll_x]{
        ftxui::Elements children;
        int ind = 1;
        children.push_back(separator());
        for (auto v: found_mappings) {
            ftxui::Elements elems;
            {
                std::ostringstream t;
                t << "#" << ind;
                elems.push_back(ftxui::text(t.str())|center);
                elems.push_back(ftxui::separator());
                ind+=1;
            }
            for (int i = 0; i<v.size(); i++) {
                std::ostringstream t;
                //t << "G[" << i+1 << "]=>H[" << v[i]+1 << "]";
                t << " " << i+1 << " => " << v[i]+1 << " " ;
                elems.push_back(ftxui::text(t.str())|center);
            }
            children.push_back(ftxui::vbox(elems));
            children.push_back(separator());
        }
        return window(text(std::string(" Isomorphic mappings (G => H) ")),hbox(children) | center);
    });
    
    auto outputTab = Container::Vertical({e});
    //auto tabs = Container::Tab({isomorphismsTab, inputTab, outputTab}, &selector);
    //auto container = Container::Vertical({toggle,tabs});
    bool original = false;
    auto matrices = Renderer([&]{
        const Component &hv = original ? h : e;
        return vbox({g->Render(), hv->Render(), text("Extension Cost: " + std::to_string(this->extension_cost))});
    });
    int frame = 0;
    auto indicator = Renderer([this, &colors,&frame]{
        if (!progress) {
            return text("");
        }
        return spinner(17,frame)|color(colors);
    });

    int tab_selector = 0;
    std::vector<std::string> headers{" G "," H "," Extension "," Isomorphic Mappings "}; 
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
    auto ext = Renderer([this, &e] {
        std::string txt = "Extension cost: " + std::to_string(extension_cost);
        return vbox({e->Render(), text(txt)});
    });
    auto renderer = Renderer([&] {
        return vbox({
            hbox(text(message) | color(message_color), filler(), indicator->Render()),
            separator(),
            Container::Tab({
                g,h,ext, isomorphisms
            }, &tab_selector)->Render(),
            filler(),
            separator(),
            toggle->Render(),
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
