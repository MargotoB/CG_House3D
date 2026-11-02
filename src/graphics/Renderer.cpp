#include "graphics/Renderer.h"

#include <glm/gtc/matrix_inverse.hpp>
#include <algorithm>
#include <cmath>
#include <string>

namespace {
constexpr int MAX_LIGHTS = 16; // трябва да съвпада с #define MAX_LIGHTS в шейдъра

glm::vec3 lerp3(const glm::vec3& a, const glm::vec3& b, float t) { return a + (b - a) * t; }
} // namespace

Renderer::~Renderer() {
    if (m_skyVao) glDeleteVertexArrays(1, &m_skyVao);
}

bool Renderer::init(const std::string& resourceDir) {
    const std::string dir = resourceDir + "/shaders/";
    if (!m_main.load(dir + "blinn_phong.vert", dir + "blinn_phong.frag")) return false;
    if (!m_picking.load(dir + "picking.vert", dir + "picking.frag")) return false;
    if (!m_sky.load(dir + "sky.vert", dir + "sky.frag")) return false;

    glGenVertexArrays(1, &m_skyVao); // небето няма върхове, но Core профилът изисква VAO

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_MULTISAMPLE);
    return true;
}

void Renderer::resize(int width, int height) {
    m_width = std::max(1, width);
    m_height = std::max(1, height);
}

bool Renderer::isVisible(const GameObject& obj, FloorView floorView) {
    if (!obj.visible) return false;
    // Плафоните на най-горния видим етаж висят във въздуха, когато таванът е скрит.
    if (obj.ceilingFixture && floorView != FloorView::All && obj.floor == static_cast<int>(floorView)) return false;
    switch (floorView) {
        case FloorView::All:    return true;
        case FloorView::Floor1: return obj.floor <= 1;  // скрит е вторият етаж и покривът
        case FloorView::Floor2: return obj.floor <= 2;  // скрит е само покривът
    }
    return true;
}

// ---------------------------------------------------------------------------
// Небе
// ---------------------------------------------------------------------------
void Renderer::drawSky(const Scene& scene, const glm::mat4& view, const glm::mat4& proj, float night) {
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    m_sky.use();
    m_sky.setMat4("uInvViewProj", glm::inverse(proj * view));
    m_sky.setVec3("uZenithColor",  lerp3({0.22f, 0.45f, 0.85f}, {0.01f, 0.015f, 0.05f}, night));
    m_sky.setVec3("uHorizonColor", lerp3({0.72f, 0.84f, 0.95f}, {0.06f, 0.08f, 0.16f}, night));
    m_sky.setVec3("uGroundColor",  lerp3({0.35f, 0.42f, 0.32f}, {0.02f, 0.03f, 0.03f}, night));
    m_sky.setVec3("uSunDirection", scene.sun.direction);
    m_sky.setVec3("uSunDiskColor", lerp3({1.0f, 0.95f, 0.8f}, {0.85f, 0.88f, 1.0f}, night));
    m_sky.setFloat("uNight", night);
    glBindVertexArray(m_skyVao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

// ---------------------------------------------------------------------------
// Светлини
// ---------------------------------------------------------------------------
float Renderer::flickerFactor(float time) {
    // Сума от няколко синусоиди с несъизмерими честоти – изглежда като случаен пламък.
    float f = std::sin(time * 11.0f) * 0.5f + std::sin(time * 17.3f + 1.3f) * 0.3f + std::sin(time * 5.1f + 0.7f) * 0.2f;
    return 0.85f + 0.15f * f;
}

void Renderer::setupLights(const Scene& scene, const Camera& camera, float time) {
    m_main.setVec3("uViewPos", camera.position());
    m_main.setVec3("uSunDirection", scene.sun.direction);
    m_main.setVec3("uSunColor", scene.sun.color);
    m_main.setFloat("uSunIntensity", scene.sun.intensity);
    m_main.setVec3("uAmbientSky", scene.sun.ambientSky);
    m_main.setVec3("uAmbientGround", scene.sun.ambientGround);

    int count = 0;
    for (const auto& l : scene.pointLights) {
        if (!l.enabled || count >= MAX_LIGHTS) continue;
        std::string p = "uLights[" + std::to_string(count) + "].";
        m_main.setVec3(p + "position", l.position);
        m_main.setVec3(p + "color", l.color);
        m_main.setFloat(p + "intensity", l.intensity * (l.flicker ? flickerFactor(time) : 1.0f));
        m_main.setFloat(p + "radius", l.radius);
        m_main.setVec3(p + "zoneMin", l.zoneMin);
        m_main.setVec3(p + "zoneMax", l.zoneMax);
        m_main.setInt(p + "invertZone", l.invertZone ? 1 : 0);
        ++count;
    }
    m_main.setInt("uNumLights", count);
}

// ---------------------------------------------------------------------------
// Един обект
// ---------------------------------------------------------------------------
void Renderer::drawObject(const Scene& scene, const GameObject& obj, int index, const ViewState& view, float time) {
    const Material& m = obj.material;
    glm::mat4 model = obj.transform.matrix();

    m_main.setMat4("uModel", model);
    m_main.setMat3("uNormalMatrix", glm::inverseTranspose(glm::mat3(model)));
    m_main.setInt("uUVMode", static_cast<int>(m.uvMode));
    m_main.setVec3("uObjectScale", obj.transform.scale);
    m_main.setFloat("uTiling", m.tiling);
    m_main.setVec2("uUVOffset", m.uvScroll * time);

    m_main.setVec3("uBaseColor", m.color);
    bool textured = m.texture && m.texture->key() != "none";
    m_main.setInt("uHasTexture", textured ? 1 : 0);
    if (textured) m.texture->bind(0);
    m_main.setFloat("uSpecular", m.specular);
    m_main.setFloat("uShininess", m.shininess);
    m_main.setFloat("uOpacity", m.opacity);

    // Крушки, абажури, пламъкът в камината и водата в басейна светят, когато
    // свързаната с тях светлина е включена.
    glm::vec3 emissive = m.emissive;
    if (obj.glow > 0.0f && obj.lightIndex >= 0) {
        const PointLight& l = scene.pointLights[obj.lightIndex];
        if (l.enabled) emissive += l.color * obj.glow * (l.flicker ? flickerFactor(time) : 1.0f);
    }
    // Нощем стъклата на осветените стаи излъчват топла светлина (ефект "светещ прозорец").
    if (obj.category == ObjectCategory::Window && m.opacity < 1.0f && obj.roomId >= 0) {
        int li = scene.rooms[obj.roomId].lightIndex;
        if (li >= 0 && scene.pointLights[li].enabled)
            emissive += glm::vec3(0.55f, 0.42f, 0.22f) * view.nightFactor;
    }
    m_main.setVec3("uEmissive", emissive);

    // Подсветка: избраният обект пулсира в оранжево, подът на избраната стая – в синьо.
    float amount = 0.0f;
    glm::vec3 hl(0.0f);
    if (index == view.selectedObject) {
        amount = 0.28f + 0.12f * std::sin(time * 5.0f);
        hl = {1.0f, 0.6f, 0.1f};
    } else if (obj.isRoomFloor && obj.roomId == view.selectedRoom && view.selectedRoom >= 0) {
        amount = 0.16f;
        hl = {0.25f, 0.6f, 1.0f};
    }
    m_main.setVec3("uHighlightColor", hl);
    m_main.setFloat("uHighlightAmount", amount);

    obj.mesh->draw();
    ++m_drawCalls;
}

// ---------------------------------------------------------------------------
// Основен кадър
// ---------------------------------------------------------------------------
void Renderer::render(const Scene& scene, const Camera& camera, const ViewState& view, float time) {
    m_drawCalls = 0;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_width, m_height);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float aspect = static_cast<float>(m_width) / m_height;
    glm::mat4 viewM = camera.viewMatrix();
    glm::mat4 proj = camera.projectionMatrix(aspect);

    drawSky(scene, viewM, proj, view.nightFactor);

    if (view.wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    m_main.use();
    m_main.setMat4("uView", viewM);
    m_main.setMat4("uProjection", proj);
    m_main.setInt("uDiffuseMap", 0);
    m_main.setVec3("uFogColor", lerp3({0.72f, 0.84f, 0.95f}, {0.06f, 0.08f, 0.16f}, view.nightFactor));
    m_main.setFloat("uFogDensity", 0.008f);
    setupLights(scene, camera, time);

    // 1) Непрозрачни обекти
    glDisable(GL_BLEND);
    for (size_t i = 0; i < scene.objects.size(); ++i) {
        const GameObject& o = scene.objects[i];
        if (o.material.opacity < 1.0f || !isVisible(o, view.floorView)) continue;
        drawObject(scene, o, static_cast<int>(i), view, time);
    }

    // 2) Прозрачни обекти (стъкла) – със смесване и без запис в буфера за дълбочина.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    for (size_t i = 0; i < scene.objects.size(); ++i) {
        const GameObject& o = scene.objects[i];
        if (o.material.opacity >= 1.0f || !isVisible(o, view.floorView)) continue;
        drawObject(scene, o, static_cast<int>(i), view, time);
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    if (view.wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

// ---------------------------------------------------------------------------
// Избор чрез цветово кодиране
// ---------------------------------------------------------------------------
int Renderer::pick(const Scene& scene, const Camera& camera, const ViewState& view, int x, int y) {
    m_pickBuffer.resize(m_width, m_height);
    m_pickBuffer.bind();
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_BLEND);

    float aspect = static_cast<float>(m_width) / m_height;
    glm::mat4 vp = camera.projectionMatrix(aspect) * camera.viewMatrix();

    m_picking.use();
    for (size_t i = 0; i < scene.objects.size(); ++i) {
        const GameObject& o = scene.objects[i];
        if (!o.pickable || !isVisible(o, view.floorView)) continue;
        unsigned id = static_cast<unsigned>(i) + 1;  // 0 е запазено за фона
        glm::vec3 color((id & 0xFF) / 255.0f, ((id >> 8) & 0xFF) / 255.0f, ((id >> 16) & 0xFF) / 255.0f);
        m_picking.setVec3("uIdColor", color);
        m_picking.setMat4("uMVP", vp * o.transform.matrix());
        o.mesh->draw();
    }

    // OpenGL брои редовете отдолу нагоре, а мишката – отгоре надолу.
    unsigned id = m_pickBuffer.readId(x, m_height - 1 - y);
    PickingBuffer::unbind();
    return static_cast<int>(id) - 1;
}
