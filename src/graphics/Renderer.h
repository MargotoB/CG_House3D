#pragma once
// Renderer – рисува сцената: небе, непрозрачни обекти, прозрачни обекти (стъкла).
// Отговаря и за подаването на светлините към шейдъра и за избора чрез цветово кодиране.

#include "core/ViewState.h"
#include "graphics/PickingBuffer.h"
#include "graphics/Shader.h"
#include "input/Camera.h"
#include "scene/Scene.h"

#include <string>

class Renderer {
public:
    ~Renderer();
    bool init(const std::string& resourceDir);
    void resize(int width, int height);

    void render(const Scene& scene, const Camera& camera, const ViewState& view, float time);

    // Връща индекса на обекта под пиксела (x, y) – координати от горния ляв ъгъл
    // на framebuffer-а, или -1, ако няма обект.
    int pick(const Scene& scene, const Camera& camera, const ViewState& view, int x, int y);

    static bool isVisible(const GameObject& obj, FloorView floorView);
    static float flickerFactor(float time); // псевдослучайно трептене на огъня

    int drawCalls() const { return m_drawCalls; }

private:
    void drawSky(const Scene& scene, const glm::mat4& view, const glm::mat4& proj, float night);
    void setupLights(const Scene& scene, const Camera& camera, float time);
    void drawObject(const Scene& scene, const GameObject& obj, int index, const ViewState& view, float time);

    Shader m_main, m_picking, m_sky;
    GLuint m_skyVao = 0;
    PickingBuffer m_pickBuffer;
    int m_width = 1, m_height = 1;
    int m_drawCalls = 0;
};
