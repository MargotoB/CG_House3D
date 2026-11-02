#include "graphics/Primitives.h"

#include <glm/gtc/constants.hpp>
#include <vector>

namespace {

// Добавя четириъгълник (два триъгълника) с обща нормала.
void addQuad(std::vector<Vertex>& v, std::vector<unsigned>& idx,
             glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, glm::vec3 n) {
    unsigned base = static_cast<unsigned>(v.size());
    v.push_back({a, n, {0, 0}});
    v.push_back({b, n, {1, 0}});
    v.push_back({c, n, {1, 1}});
    v.push_back({d, n, {0, 1}});
    idx.insert(idx.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
}

void addTriangle(std::vector<Vertex>& v, std::vector<unsigned>& idx,
                 glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 n) {
    unsigned base = static_cast<unsigned>(v.size());
    v.push_back({a, n, {0, 0}});
    v.push_back({b, n, {1, 0}});
    v.push_back({c, n, {0.5f, 1}});
    idx.insert(idx.end(), {base, base + 1, base + 2});
}

} // namespace

namespace Primitives {

std::unique_ptr<Mesh> cube() {
    std::vector<Vertex> v;
    std::vector<unsigned> idx;
    const float h = 0.5f;
    // Всяка стена има собствени върхове, за да има коректни (остри) нормали.
    addQuad(v, idx, {-h, -h,  h}, { h, -h,  h}, { h,  h,  h}, {-h,  h,  h}, { 0,  0,  1}); // предна
    addQuad(v, idx, { h, -h, -h}, {-h, -h, -h}, {-h,  h, -h}, { h,  h, -h}, { 0,  0, -1}); // задна
    addQuad(v, idx, {-h, -h, -h}, {-h, -h,  h}, {-h,  h,  h}, {-h,  h, -h}, {-1,  0,  0}); // лява
    addQuad(v, idx, { h, -h,  h}, { h, -h, -h}, { h,  h, -h}, { h,  h,  h}, { 1,  0,  0}); // дясна
    addQuad(v, idx, {-h,  h,  h}, { h,  h,  h}, { h,  h, -h}, {-h,  h, -h}, { 0,  1,  0}); // горна
    addQuad(v, idx, {-h, -h, -h}, { h, -h, -h}, { h, -h,  h}, {-h, -h,  h}, { 0, -1,  0}); // долна
    return std::make_unique<Mesh>("Cube", v, idx);
}

std::unique_ptr<Mesh> plane() {
    std::vector<Vertex> v;
    std::vector<unsigned> idx;
    addQuad(v, idx, {-0.5f, 0, 0.5f}, {0.5f, 0, 0.5f}, {0.5f, 0, -0.5f}, {-0.5f, 0, -0.5f}, {0, 1, 0});
    return std::make_unique<Mesh>("Plane", v, idx);
}

std::unique_ptr<Mesh> cylinder(int segments) {
    std::vector<Vertex> v;
    std::vector<unsigned> idx;
    const float r = 0.5f, h = 0.5f;
    const float step = glm::two_pi<float>() / segments;

    // Странична повърхнина – гладки нормали, насочени радиално.
    for (int i = 0; i <= segments; ++i) {
        float a = i * step;
        glm::vec3 n(std::cos(a), 0, std::sin(a));
        float u = static_cast<float>(i) / segments;
        v.push_back({{r * n.x, -h, r * n.z}, n, {u, 0}});
        v.push_back({{r * n.x,  h, r * n.z}, n, {u, 1}});
    }
    for (int i = 0; i < segments; ++i) {
        unsigned b = i * 2;
        idx.insert(idx.end(), {b, b + 1, b + 3, b, b + 3, b + 2});
    }

    // Горна и долна основа (ветрило от триъгълници).
    for (int side = 0; side < 2; ++side) {
        float y = side == 0 ? h : -h;
        glm::vec3 n(0, side == 0 ? 1.0f : -1.0f, 0);
        unsigned center = static_cast<unsigned>(v.size());
        v.push_back({{0, y, 0}, n, {0.5f, 0.5f}});
        for (int i = 0; i <= segments; ++i) {
            float a = i * step;
            v.push_back({{r * std::cos(a), y, r * std::sin(a)}, n,
                         {0.5f + 0.5f * std::cos(a), 0.5f + 0.5f * std::sin(a)}});
        }
        for (int i = 0; i < segments; ++i) {
            if (side == 0) idx.insert(idx.end(), {center, center + 2 + i, center + 1 + i});
            else           idx.insert(idx.end(), {center, center + 1 + i, center + 2 + i});
        }
    }
    return std::make_unique<Mesh>("Cylinder", v, idx);
}

std::unique_ptr<Mesh> cone(int segments) {
    std::vector<Vertex> v;
    std::vector<unsigned> idx;
    const float r = 0.5f, h = 0.5f;
    const float step = glm::two_pi<float>() / segments;
    const float slope = r / (2 * h); // наклон на нормалата спрямо Y

    for (int i = 0; i < segments; ++i) {
        float a0 = i * step, a1 = (i + 1) * step, am = (a0 + a1) * 0.5f;
        glm::vec3 p0(r * std::cos(a0), -h, r * std::sin(a0));
        glm::vec3 p1(r * std::cos(a1), -h, r * std::sin(a1));
        glm::vec3 n0 = glm::normalize(glm::vec3(std::cos(a0), slope, std::sin(a0)));
        glm::vec3 n1 = glm::normalize(glm::vec3(std::cos(a1), slope, std::sin(a1)));
        glm::vec3 nm = glm::normalize(glm::vec3(std::cos(am), slope, std::sin(am)));
        unsigned b = static_cast<unsigned>(v.size());
        v.push_back({p0, n0, {static_cast<float>(i) / segments, 0}});
        v.push_back({{0, h, 0}, nm, {(i + 0.5f) / segments, 1}});
        v.push_back({p1, n1, {static_cast<float>(i + 1) / segments, 0}});
        idx.insert(idx.end(), {b, b + 1, b + 2});
    }
    // Основа
    unsigned center = static_cast<unsigned>(v.size());
    v.push_back({{0, -h, 0}, {0, -1, 0}, {0.5f, 0.5f}});
    for (int i = 0; i <= segments; ++i) {
        float a = i * step;
        v.push_back({{r * std::cos(a), -h, r * std::sin(a)}, {0, -1, 0},
                     {0.5f + 0.5f * std::cos(a), 0.5f + 0.5f * std::sin(a)}});
    }
    for (int i = 0; i < segments; ++i)
        idx.insert(idx.end(), {center, center + 1 + i, center + 2 + i});

    return std::make_unique<Mesh>("Cone", v, idx);
}

std::unique_ptr<Mesh> sphere(int stacks, int slices) {
    std::vector<Vertex> v;
    std::vector<unsigned> idx;
    const float r = 0.5f;
    for (int i = 0; i <= stacks; ++i) {
        float phi = glm::pi<float>() * i / stacks;          // от 0 (горе) до π (долу)
        for (int j = 0; j <= slices; ++j) {
            float theta = glm::two_pi<float>() * j / slices;
            glm::vec3 n(std::sin(phi) * std::cos(theta), std::cos(phi), std::sin(phi) * std::sin(theta));
            v.push_back({n * r, n, {static_cast<float>(j) / slices, 1.0f - static_cast<float>(i) / stacks}});
        }
    }
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            unsigned a = i * (slices + 1) + j;
            unsigned b = a + slices + 1;
            idx.insert(idx.end(), {a, a + 1, b, a + 1, b + 1, b});
        }
    }
    return std::make_unique<Mesh>("Sphere", v, idx);
}

std::unique_ptr<Mesh> prism() {
    // Напречното сечение е триъгълник в равнината YZ (основа по Z, връх горе),
    // изтеглен по оста X. Използва се за фронтона под двускатния покрив.
    std::vector<Vertex> v;
    std::vector<unsigned> idx;
    const float h = 0.5f;
    glm::vec3 fl(-h, -h, h), fr(h, -h, h);   // долу отпред
    glm::vec3 bl(-h, -h, -h), br(h, -h, -h); // долу отзад
    glm::vec3 tl(-h, h, 0), tr(h, h, 0);     // билото

    glm::vec3 nFront = glm::normalize(glm::vec3(0, 1, 1));
    glm::vec3 nBack  = glm::normalize(glm::vec3(0, 1, -1));
    addQuad(v, idx, fl, fr, tr, tl, nFront);
    addQuad(v, idx, br, bl, tl, tr, nBack);
    addQuad(v, idx, bl, br, fr, fl, {0, -1, 0});
    addTriangle(v, idx, bl, fl, tl, {-1, 0, 0});
    addTriangle(v, idx, fr, br, tr, {1, 0, 0});
    return std::make_unique<Mesh>("Prism", v, idx);
}

} // namespace Primitives
