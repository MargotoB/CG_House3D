#pragma once
// InputController – обработва клавиатурата и мишката (чрез GLFW) и ги превръща
// в действия: управление на камерата, избор на обект, превключване на режими.

struct GLFWwindow;
class Application;

class InputController {
public:
    void attach(GLFWwindow* window, Application* app);
    void update(float dt); // непрекъснато движение в свободен режим

private:
    static void onMouseButton(GLFWwindow* w, int button, int action, int mods);
    static void onCursorPos(GLFWwindow* w, double x, double y);
    static void onScroll(GLFWwindow* w, double dx, double dy);
    static void onKey(GLFWwindow* w, int key, int scancode, int action, int mods);
    static void onFramebufferSize(GLFWwindow* w, int width, int height);

    void handleClick(double x, double y);

    GLFWwindow* m_window = nullptr;
    Application* m_app = nullptr;

    bool m_left = false, m_right = false, m_middle = false;
    bool m_dragged = false;
    double m_pressX = 0, m_pressY = 0;
    double m_lastX = 0, m_lastY = 0;
    bool m_firstFreeMouse = true;
};
