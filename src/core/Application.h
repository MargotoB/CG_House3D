#pragma once
// Application – създава прозореца и OpenGL контекста, държи всички подсистеми
// и изпълнява главния цикъл. Тук е и логиката на взаимодействието
// (избор на обект/стая, изгледи, ден/нощ), която се използва от InputController и UIPanel.

#include "core/ViewState.h"
#include "graphics/Renderer.h"
#include "input/Camera.h"
#include "input/InputController.h"
#include "scene/Scene.h"
#include "ui/UIPanel.h"

#include <memory>
#include <string>

struct GLFWwindow;

struct AppOptions {
    int width = 1600, height = 900;
    std::string screenshotPath;   // ако е зададен: рендерира няколко кадъра, записва PNG и излиза
    int view = -1;                // предварително зададен изглед (0..5)
    int floor = 0;                // 0 – всички, 1, 2
    int room = -1;                // стая за фокусиране
    int selectObject = -1;
    bool night = false;
    bool hideUI = false;
    bool listRooms = false;
    int pickX = -1, pickY = -1;   // тест на избора: симулира клик в тази точка
};

enum ViewPreset { VIEW_FRONT = 0, VIEW_BACK, VIEW_LEFT, VIEW_RIGHT, VIEW_TOP, VIEW_ISO, VIEW_COUNT };

class Application {
public:
    explicit Application(AppOptions options) : m_options(std::move(options)) {}
    ~Application();

    bool init();
    void run();

    // --- Действия (извикват се от входа и от потребителския интерфейс) ---
    void selectObject(int index);
    void clearSelection();
    void focusRoom(int roomId);
    void setFloorView(FloorView fv, bool moveCamera = true);
    void setViewPreset(int preset);
    void setNight(bool night);
    void toggleFreeCamera();
    void requestScreenshot() { m_screenshotRequested = true; }

    Scene& scene() { return *m_scene; }
    Camera& camera() { return m_camera; }
    ViewState& view() { return m_view; }
    Renderer& renderer() { return *m_renderer; }
    GLFWwindow* window() { return m_window; }
    float fps() const { return m_fps; }
    const std::string& lastScreenshot() const { return m_lastScreenshot; }

    static const char* presetName(int preset);

private:
    void updateDayNight(float dt);
    void saveScreenshot(const std::string& path);
    std::string makeScreenshotPath() const;

    AppOptions m_options;
    std::string m_resourceDir;
    GLFWwindow* m_window = nullptr;

    // Сцената и рендерерът притежават OpenGL ресурси – унищожават се изрично,
    // докато контекстът още съществува.
    std::unique_ptr<Scene> m_scene;
    std::unique_ptr<Renderer> m_renderer;
    Camera m_camera;
    ViewState m_view;
    InputController m_input;
    UIPanel m_ui;

    bool m_screenshotRequested = false;
    std::string m_lastScreenshot;
    float m_fps = 0.0f;
};
