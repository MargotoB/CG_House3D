#pragma once
// Светлинни източници в сцената.

#include <glm/glm.hpp>
#include <string>

// Насочена светлина – слънце през деня, луна през нощта.
struct DirectionalLight {
    glm::vec3 direction{0.0f, 1.0f, 0.0f}; // посока КЪМ източника (нормализирана)
    glm::vec3 color{1.0f};
    float intensity = 1.0f;
    glm::vec3 ambientSky{0.3f};    // околна светлина отгоре
    glm::vec3 ambientGround{0.1f}; // околна светлина отдолу
};

// Точкова светлина – лампа. Светлината действа само в своята "зона" (стаята),
// за да не прониква през стените, тъй като сцената няма сенки.
struct PointLight {
    std::string name;
    glm::vec3 position{0.0f};
    glm::vec3 color{1.0f, 0.85f, 0.6f};
    float intensity = 1.5f;
    float radius = 8.0f;           // разстояние, след което светлината изчезва
    bool enabled = false;
    int roomId = -1;               // -1 = външна светлина
    glm::vec3 zoneMin{-1e4f}, zoneMax{1e4f};
    bool invertZone = false;       // true – свети само ИЗВЪН зоната (външни лампи)
    bool flicker = false;          // трептене (огън в камината)
};
