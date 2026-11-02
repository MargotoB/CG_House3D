// =============================================================================
//  3D Къща – интерактивна архитектурна сцена с осветление
//  Курсов проект по Компютърна графика (BCSE51), ТУ – София
//
//  OpenGL 3.3 Core · GLFW · GLAD · GLM · Dear ImGui · stb
// =============================================================================

#include "core/Application.h"

#include <cstring>
#include <iostream>
#include <string>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

static void printUsage() {
    std::cout <<
        "Usage: House3D [options]\n"
        "  --width W --height H     window size\n"
        "  --view N                 start view: 0 front, 1 back, 2 left, 3 right, 4 top, 5 perspective\n"
        "  --floor F                1 – floor 1 only, 2 – floor 2 only\n"
        "  --room R                 focus room R (see --list-rooms)\n"
        "  --select I               select object with index I\n"
        "  --night                  night mode\n"
        "  --no-ui                  hide the panel\n"
        "  --screenshot FILE.png    render, save a screenshot and exit\n"
        "  --list-rooms             print the list of rooms\n"
        "  --pick X Y               simulate a click at pixel (X, Y) – picking test\n";
}

int main(int argc, char** argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8); // кирилица в конзолата
#endif
    AppOptions opt;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&](int& target) { if (i + 1 < argc) target = std::stoi(argv[++i]); };
        if (a == "--width") next(opt.width);
        else if (a == "--height") next(opt.height);
        else if (a == "--view") next(opt.view);
        else if (a == "--floor") next(opt.floor);
        else if (a == "--room") next(opt.room);
        else if (a == "--select") next(opt.selectObject);
        else if (a == "--night") opt.night = true;
        else if (a == "--no-ui") opt.hideUI = true;
        else if (a == "--list-rooms") opt.listRooms = true;
        else if (a == "--pick") { next(opt.pickX); next(opt.pickY); }
        else if (a == "--screenshot" && i + 1 < argc) opt.screenshotPath = argv[++i];
        else if (a == "--help" || a == "-h") { printUsage(); return 0; }
        else { std::cerr << "Unknown option: " << a << "\n"; printUsage(); return 1; }
    }

    Application app(opt);
    if (!app.init()) {
        std::cerr << "Initialisation failed." << std::endl;
        return 1;
    }
    app.run();
    return 0;
}
