#pragma once
// Transform – транслация, ротация и мащабиране на обект.
// Моделната матрица се получава като M = T(position) · R(rotation) · T(pivotOffset) · S(scale).
// pivotOffset позволява въртене около точка, различна от центъра (напр. панта на врата).

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Transform {
    glm::vec3 position{0.0f};
    glm::vec3 rotation{0.0f};    // ъгли на Ойлер в градуси (X, Y, Z)
    glm::vec3 scale{1.0f};
    glm::vec3 pivotOffset{0.0f}; // отместване на геометрията спрямо точката на въртене

    glm::mat4 matrix() const {
        glm::mat4 m(1.0f);
        m = glm::translate(m, position);
        m = glm::rotate(m, glm::radians(rotation.y), {0, 1, 0});
        m = glm::rotate(m, glm::radians(rotation.x), {1, 0, 0});
        m = glm::rotate(m, glm::radians(rotation.z), {0, 0, 1});
        m = glm::translate(m, pivotOffset);
        m = glm::scale(m, scale);
        return m;
    }

    // Световната позиция на центъра на геометрията.
    glm::vec3 worldCenter() const { return glm::vec3(matrix() * glm::vec4(0, 0, 0, 1)); }
};
