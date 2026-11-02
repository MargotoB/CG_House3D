#pragma once
// UIPanel – страничен панел (Dear ImGui) за управление на сцената:
// изгледи, етажи, ден/нощ, лампи, стаи, информация и материал на избрания обект.

#include <string>

struct GLFWwindow;
class Application;

class UIPanel {
public:
    void init(GLFWwindow* window, const std::string& resourceDir);
    void draw(Application& app);
    void shutdown();

private:
    void drawViewSection(Application& app);
    void drawLightingSection(Application& app);
    void drawRoomsSection(Application& app);
    void drawSelectionSection(Application& app);
    void drawHelpSection();

    bool m_initialized = false;
};
