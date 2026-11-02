#pragma once
// Material – параметри на повърхността, използвани от Blinn–Phong шейдъра.

#include <glm/glm.hpp>
#include <string>

class Texture;

enum class UVMode {
    Mesh = 0, // текстурни координати от самата геометрия
    Box  = 1  // "кутийна" проекция в метри – текстурата не се разтяга при мащабиране
};

struct Material {
    std::string name = "Material";
    glm::vec3 color{1.0f};        // основен (дифузен) цвят, умножава се с текстурата
    Texture* texture = nullptr;   // дифузна текстура (nullptr = без текстура)
    float tiling = 1.0f;          // колко пъти се повтаря текстурата на 1 m
    UVMode uvMode = UVMode::Box;

    float specular = 0.2f;        // сила на огледалното отражение
    float shininess = 32.0f;      // степен в модела на Blinn–Phong
    glm::vec3 emissive{0.0f};     // собствено светене (крушки)
    float opacity = 1.0f;         // < 1 – прозрачен (стъкло)
    glm::vec2 uvScroll{0.0f};     // скорост на плъзгане на текстурата (анимирана вода)
};
