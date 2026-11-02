#pragma once
// Room – стая в къщата: име, етаж, граници и свързаната с нея лампа.

#include <glm/glm.hpp>
#include <string>

struct Room {
    int id = -1;
    std::string name;
    int floor = 1;                 // 1 – първи етаж, 2 – втори етаж
    glm::vec3 boundsMin{0.0f}, boundsMax{0.0f};
    int lightIndex = -1;           // индекс в Scene::pointLights
    std::string description;

    glm::vec3 center() const { return (boundsMin + boundsMax) * 0.5f; }
    float area() const { return (boundsMax.x - boundsMin.x) * (boundsMax.z - boundsMin.z); }
    bool contains(const glm::vec3& p) const {
        return glm::all(glm::greaterThanEqual(p, boundsMin)) && glm::all(glm::lessThanEqual(p, boundsMax));
    }
};
