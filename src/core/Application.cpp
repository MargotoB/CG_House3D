#include "core/Application.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBIW_WINDOWS_UTF8
#include <stb_image_write.h>

#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

namespace fs = std::filesystem;

namespace {
// Търси папката с shaders/ – първо около текущата директория, после пътя от CMake.
std::string findResourceDir() {
    // Всички пътища се пазят в UTF-8 (u8path/u8string), за да работят и папки на кирилица в Windows.
    std::vector<fs::path> candidates = {fs::current_path(), fs::current_path() / "..",
                                        fs::current_path() / ".." / ".."};
#ifdef RESOURCE_DIR
    candidates.push_back(fs::u8path(RESOURCE_DIR));
#endif
    for (const auto& c : candidates) {
        std::error_code ec;
        if (fs::exists(c / "shaders" / "blinn_phong.vert", ec))
            return fs::weakly_canonical(c, ec).u8string();
    }
    return ".";
}

glm::vec3 lerp3(const glm::vec3& a, const glm::vec3& b, float t) { return a + (b - a) * t; }

void glfwErrorCallback(int code, const char* description) {
    std::cerr << "[GLFW] Error " << code << ": " << description << std::endl;
}
} // namespace

Application::~Application() {
    m_ui.shutdown();
    // Ресурсите на OpenGL трябва да се освободят, докато контекстът още съществува.
    m_scene.reset();
    m_renderer.reset();
    if (m_window) glfwDestroyWindow(m_window);
    glfwTerminate();
}

bool Application::init() {
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) return false;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_SAMPLES, 4); // MSAA – изглаждане на ръбовете

    m_window = glfwCreateWindow(m_options.width, m_options.height,
                                "3D House – Interactive Architectural Scene (OpenGL)", nullptr, nullptr);
    if (!m_window) {
        std::cerr << "Could not create an OpenGL 3.3 window." << std::endl;
        return false;
    }
    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1); // V-Sync

    if (!gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress))) {
        std::cerr << "Failed to load OpenGL functions (GLAD)." << std::endl;
        return false;
    }
    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\nGPU:    " << glGetString(GL_RENDERER) << std::endl;

    m_resourceDir = findResourceDir();
    std::cout << "Resources: " << m_resourceDir << std::endl;

    m_renderer = std::make_unique<Renderer>();
    if (!m_renderer->init(m_resourceDir)) return false;
    m_scene = std::make_unique<Scene>();
    m_scene->init(m_resourceDir);
    std::cout << "Objects: " << m_scene->objects.size() << ", triangles: " << m_scene->totalTriangles()
              << ", rooms: " << m_scene->rooms.size() << ", lights: " << m_scene->pointLights.size() << std::endl;

    int fbw, fbh;
    glfwGetFramebufferSize(m_window, &fbw, &fbh);
    m_renderer->resize(fbw, fbh);

    m_input.attach(m_window, this);
    m_ui.init(m_window, m_resourceDir);

    // Начално състояние според аргументите от командния ред.
    setViewPreset(m_options.view >= 0 ? m_options.view : VIEW_ISO);
    const bool keepCamera = m_options.view >= 0;
    if (m_options.floor == 1) setFloorView(FloorView::Floor1, !keepCamera);
    if (m_options.floor == 2) setFloorView(FloorView::Floor2, !keepCamera);
    if (m_options.room >= 0 && m_options.room < static_cast<int>(m_scene->rooms.size())) focusRoom(m_options.room);
    if (m_options.selectObject >= 0) selectObject(m_options.selectObject);
    if (m_options.night) {
        setNight(true);
        m_view.nightFactor = 1.0f;
    }
    m_view.showUI = !m_options.hideUI;
    m_camera.snap();
    updateDayNight(0.0f);

    if (m_options.pickX >= 0) {
        m_camera.snap();
        int idx = m_renderer->pick(*m_scene, m_camera, m_view, m_options.pickX, m_options.pickY);
        std::cout << "Pick at (" << m_options.pickX << ", " << m_options.pickY << "): ";
        if (idx >= 0) {
            selectObject(idx);
            const GameObject& o = m_scene->objects[idx];
            std::cout << "#" << idx << " " << o.name;
            if (o.roomId >= 0) std::cout << " – room " << m_scene->rooms[o.roomId].name;
            std::cout << std::endl;
        } else {
            std::cout << "no object" << std::endl;
        }
    }
    if (m_options.listRooms)
        for (const auto& r : m_scene->rooms) std::cout << r.id << ": " << r.name << " (floor " << r.floor << ")\n";
    return true;
}

// ---------------------------------------------------------------------------
// Главен цикъл
// ---------------------------------------------------------------------------
void Application::run() {
    double last = glfwGetTime();
    double fpsTimer = 0.0;
    int fpsFrames = 0;
    int frame = 0;
    const bool screenshotMode = !m_options.screenshotPath.empty();

    while (!glfwWindowShouldClose(m_window)) {
        double now = glfwGetTime();
        float dt = static_cast<float>(std::min(now - last, 0.1));
        last = now;

        fpsTimer += dt;
        ++fpsFrames;
        if (fpsTimer >= 0.5) {
            m_fps = static_cast<float>(fpsFrames / fpsTimer);
            fpsTimer = 0.0;
            fpsFrames = 0;
        }

        glfwPollEvents();
        m_input.update(dt);
        updateDayNight(dt);
        m_scene->update(dt);
        m_camera.update(dt);
        if (screenshotMode) m_camera.snap();

        float time = static_cast<float>(now);
        if (screenshotMode) time = 0.0f;   // стабилна подсветка на снимките
        m_renderer->render(*m_scene, m_camera, m_view, time);
        if (m_view.showUI) m_ui.draw(*this);

        if (m_screenshotRequested) {
            m_screenshotRequested = false;
            saveScreenshot(makeScreenshotPath());
        }
        if (screenshotMode && ++frame == 4) {
            saveScreenshot(m_options.screenshotPath);
            glfwSetWindowShouldClose(m_window, GLFW_TRUE);
        }
        glfwSwapBuffers(m_window);
    }
}

// ---------------------------------------------------------------------------
// Ден / нощ – плавен преход за около 1.5 s
// ---------------------------------------------------------------------------
void Application::updateDayNight(float dt) {
    float target = m_view.night ? 1.0f : 0.0f;
    float step = dt / 1.5f;
    if (m_view.nightFactor < target) m_view.nightFactor = std::min(target, m_view.nightFactor + step);
    else m_view.nightFactor = std::max(target, m_view.nightFactor - step);

    float t = glm::smoothstep(0.0f, 1.0f, m_view.nightFactor);
    DirectionalLight& sun = m_scene->sun;
    sun.direction = glm::normalize(lerp3(glm::normalize(glm::vec3(-0.45f, 0.75f, 0.55f)),
                                         glm::normalize(glm::vec3(0.55f, 0.6f, -0.4f)), t));
    sun.color = lerp3({1.0f, 0.95f, 0.86f}, {0.55f, 0.65f, 1.0f}, t);
    sun.intensity = glm::mix(1.15f, 0.16f, t);
    sun.ambientSky = lerp3({0.30f, 0.34f, 0.42f}, {0.020f, 0.026f, 0.055f}, t);
    sun.ambientGround = lerp3({0.15f, 0.14f, 0.11f}, {0.008f, 0.008f, 0.012f}, t);
}

// ---------------------------------------------------------------------------
// Действия
// ---------------------------------------------------------------------------
void Application::selectObject(int index) {
    if (index < 0 || index >= static_cast<int>(m_scene->objects.size())) {
        clearSelection();
        return;
    }
    m_view.selectedObject = index;
    int room = m_scene->objects[index].roomId;
    // Клик върху нова стая – камерата отива при нея; клик в текущата стая само избира обекта.
    if (room >= 0 && room != m_view.selectedRoom) focusRoom(room);
    else if (room < 0) m_view.selectedRoom = -1;
}

void Application::clearSelection() {
    m_view.selectedObject = -1;
    m_view.selectedRoom = -1;
}

void Application::focusRoom(int roomId) {
    if (roomId < 0 || roomId >= static_cast<int>(m_scene->rooms.size())) return;
    const Room& r = m_scene->rooms[roomId];
    m_view.selectedRoom = roomId;
    // За да се вижда вътре, скриваме всичко над етажа на стаята.
    setFloorView(r.floor == 1 ? FloorView::Floor1 : FloorView::Floor2, false);

    glm::vec3 size = r.boundsMax - r.boundsMin;
    float dist = std::max(size.x, size.z) * 1.25f + 4.0f;
    glm::vec3 target = r.center();
    target.y = r.boundsMin.y + 0.8f;
    m_camera.flyTo(target, dist, m_camera.yaw(), 58.0f);
}

void Application::setFloorView(FloorView fv, bool moveCamera) {
    m_view.floorView = fv;
    if (!moveCamera) return;
    switch (fv) {
        case FloorView::All:    m_camera.flyTo({0, 3.0f, 0}, 26.0f, m_camera.yaw(), 24.0f); break;
        case FloorView::Floor1: m_camera.flyTo({0, 0.5f, 0}, 20.0f, m_camera.yaw(), 55.0f); break;
        case FloorView::Floor2: m_camera.flyTo({0, 3.5f, 0}, 20.0f, m_camera.yaw(), 55.0f); break;
    }
}

const char* Application::presetName(int preset) {
    static const char* names[VIEW_COUNT] = {"Front", "Back", "Left", "Right", "Top", "Perspective"};
    return (preset >= 0 && preset < VIEW_COUNT) ? names[preset] : "";
}

void Application::setViewPreset(int preset) {
    glm::vec3 target(0, 3.0f, 0);
    switch (preset) {
        case VIEW_FRONT: m_camera.flyTo(target, 21.0f, 90.0f, 14.0f); break;
        case VIEW_BACK:  m_camera.flyTo(target, 24.0f, -90.0f, 12.0f); break;
        case VIEW_LEFT:  m_camera.flyTo(target, 22.0f, 180.0f, 12.0f); break;
        case VIEW_RIGHT: m_camera.flyTo(target, 22.0f, 0.0f, 12.0f); break;
        case VIEW_TOP:   m_camera.flyTo({0, 0, 0}, 26.0f, 90.0f, 89.0f); break;
        case VIEW_ISO:   m_camera.flyTo(target, 26.0f, 55.0f, 26.0f); break;
        default: break;
    }
}

void Application::setNight(bool night) {
    m_view.night = night;
    m_scene->setAllLamps(night); // нощем всички лампи се включват автоматично
}

void Application::toggleFreeCamera() {
    bool toFree = m_camera.mode() == CameraMode::Orbit;
    m_camera.setMode(toFree ? CameraMode::Free : CameraMode::Orbit);
    glfwSetInputMode(m_window, GLFW_CURSOR, toFree ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}

// ---------------------------------------------------------------------------
// Снимка на екрана (PNG)
// ---------------------------------------------------------------------------
std::string Application::makeScreenshotPath() const {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream ss;
    ss << m_resourceDir << "/screenshots/screenshot_" << std::put_time(&tm, "%Y%m%d_%H%M%S") << ".png";
    return ss.str();
}

void Application::saveScreenshot(const std::string& path) {
    int w, h;
    glfwGetFramebufferSize(m_window, &w, &h);
    std::vector<unsigned char> pixels(static_cast<size_t>(w) * h * 3);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glReadBuffer(GL_BACK);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    std::error_code ec;
    fs::create_directories(fs::u8path(path).parent_path(), ec);
    stbi_flip_vertically_on_write(1);
    if (stbi_write_png(path.c_str(), w, h, 3, pixels.data(), w * 3)) {
        m_lastScreenshot = path;
        std::cout << "Screenshot saved: " << path << std::endl;
    } else {
        std::cerr << "Failed to write " << path << std::endl;
    }
}
