#include "input/InputController.h"

#include "core/Application.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <algorithm>
#include <cmath>

namespace {
InputController* self(GLFWwindow* w) { return static_cast<InputController*>(glfwGetWindowUserPointer(w)); }

// Когато курсорът е върху панела на ImGui, входът е за панела, а не за сцената.
bool uiWantsMouse() { return ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse; }
bool uiWantsKeyboard() { return ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureKeyboard; }
} // namespace

void InputController::attach(GLFWwindow* window, Application* app) {
    m_window = window;
    m_app = app;
    glfwSetWindowUserPointer(window, this);
    // ImGui се инициализира след това и автоматично предава събитията и на тези функции.
    glfwSetMouseButtonCallback(window, onMouseButton);
    glfwSetCursorPosCallback(window, onCursorPos);
    glfwSetScrollCallback(window, onScroll);
    glfwSetKeyCallback(window, onKey);
    glfwSetFramebufferSizeCallback(window, onFramebufferSize);
}

void InputController::onFramebufferSize(GLFWwindow* w, int width, int height) {
    self(w)->m_app->renderer().resize(width, height);
}

void InputController::onMouseButton(GLFWwindow* w, int button, int action, int mods) {
    InputController* in = self(w);
    bool pressed = action == GLFW_PRESS;
    if (pressed && uiWantsMouse()) return;
    if (in->m_app->camera().mode() == CameraMode::Free) return;

    double x, y;
    glfwGetCursorPos(w, &x, &y);

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (pressed) {
            in->m_left = true;
            in->m_dragged = false;
            in->m_pressX = x;
            in->m_pressY = y;
        } else if (in->m_left) {
            in->m_left = false;
            if (!in->m_dragged) in->handleClick(x, y); // кратък клик = избор
        }
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        in->m_right = pressed;
    } else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        in->m_middle = pressed;
    }
    in->m_lastX = x;
    in->m_lastY = y;
}

void InputController::onCursorPos(GLFWwindow* w, double x, double y) {
    InputController* in = self(w);
    Camera& cam = in->m_app->camera();
    double dx = x - in->m_lastX, dy = y - in->m_lastY;
    in->m_lastX = x;
    in->m_lastY = y;

    if (cam.mode() == CameraMode::Free) {
        if (in->m_firstFreeMouse) { in->m_firstFreeMouse = false; return; }
        cam.look(static_cast<float>(dx) * 0.12f, static_cast<float>(-dy) * 0.12f);
        return;
    }
    in->m_firstFreeMouse = true;

    if (in->m_left) {
        if (std::abs(x - in->m_pressX) + std::abs(y - in->m_pressY) > 4.0) in->m_dragged = true;
        if (in->m_dragged) cam.orbit(static_cast<float>(dx) * 0.3f, static_cast<float>(dy) * 0.25f);
    } else if (in->m_right || in->m_middle) {
        cam.pan(static_cast<float>(dx), static_cast<float>(dy));
    }
}

void InputController::onScroll(GLFWwindow* w, double dx, double dy) {
    if (uiWantsMouse()) return;
    InputController* in = self(w);
    if (in->m_app->camera().mode() == CameraMode::Orbit) in->m_app->camera().zoom(static_cast<float>(dy));
}

void InputController::onKey(GLFWwindow* w, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS || uiWantsKeyboard()) return;
    InputController* in = self(w);
    Application& app = *in->m_app;
    ViewState& view = app.view();

    if (key >= GLFW_KEY_1 && key <= GLFW_KEY_6) {
        app.setViewPreset(key - GLFW_KEY_1);
        return;
    }
    switch (key) {
        case GLFW_KEY_F1: app.setFloorView(FloorView::All); break;
        case GLFW_KEY_F2: app.setFloorView(FloorView::Floor1); break;
        case GLFW_KEY_F3: app.setFloorView(FloorView::Floor2); break;
        case GLFW_KEY_N:  app.setNight(!view.night); break;
        case GLFW_KEY_L: {
            bool anyOn = false;
            for (const auto& l : app.scene().pointLights) anyOn |= l.enabled;
            app.scene().setAllLamps(!anyOn);
            break;
        }
        case GLFW_KEY_O:
            for (auto& o : app.scene().objects)
                if (o.isDoor) app.scene().toggleDoor(o);
            break;
        case GLFW_KEY_TAB: app.toggleFreeCamera(); break;
        case GLFW_KEY_ESCAPE:
            if (app.camera().mode() == CameraMode::Free) app.toggleFreeCamera();
            else app.clearSelection();
            break;
        case GLFW_KEY_H:   view.showUI = !view.showUI; break;
        case GLFW_KEY_P:   view.wireframe = !view.wireframe; break;
        case GLFW_KEY_F12: app.requestScreenshot(); break;
        default: break;
    }
}

void InputController::update(float dt) {
    Camera& cam = m_app->camera();
    if (cam.mode() != CameraMode::Free || uiWantsKeyboard()) return;
    glm::vec3 dir(0.0f);
    if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS) dir.z += 1;
    if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS) dir.z -= 1;
    if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS) dir.x += 1;
    if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS) dir.x -= 1;
    if (glfwGetKey(m_window, GLFW_KEY_E) == GLFW_PRESS || glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS) dir.y += 1;
    if (glfwGetKey(m_window, GLFW_KEY_Q) == GLFW_PRESS) dir.y -= 1;
    bool fast = glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
    if (glm::length(dir) > 0.0f) cam.move(glm::normalize(dir), dt, fast);
}

void InputController::handleClick(double x, double y) {
    // Координатите на мишката са в "екранни точки"; на HiDPI екрани framebuffer-ът е по-голям.
    int ww, wh, fw, fh;
    glfwGetWindowSize(m_window, &ww, &wh);
    glfwGetFramebufferSize(m_window, &fw, &fh);
    int px = static_cast<int>(x * fw / std::max(1, ww));
    int py = static_cast<int>(y * fh / std::max(1, wh));

    int index = m_app->renderer().pick(m_app->scene(), m_app->camera(), m_app->view(), px, py);
    if (index >= 0) m_app->selectObject(index);
    else m_app->clearSelection();
}
