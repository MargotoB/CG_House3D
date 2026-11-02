#include "input/Camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

namespace {
// Посока от сферични координати (ъглите са в градуси).
glm::vec3 sphericalDir(float yawDeg, float pitchDeg) {
    float y = glm::radians(yawDeg), p = glm::radians(pitchDeg);
    return {std::cos(p) * std::cos(y), std::sin(p), std::cos(p) * std::sin(y)};
}

// Разлика между ъгли в интервала (-180, 180] – за въртене по най-краткия път.
float angleDelta(float from, float to) {
    float d = std::fmod(to - from + 540.0f, 360.0f) - 180.0f;
    return d;
}
} // namespace

Camera::Camera() { recomputeOrbitPosition(); }

void Camera::recomputeOrbitPosition() {
    m_position = m_target + sphericalDir(m_yaw, m_pitch) * m_distance;
}

void Camera::update(float dt) {
    if (m_mode != CameraMode::Orbit) return;
    float k = 1.0f - std::exp(-dt * 6.0f);   // скорост на плавното приближаване
    m_target += (m_goalTarget - m_target) * k;
    m_distance += (m_goalDistance - m_distance) * k;
    m_yaw += angleDelta(m_yaw, m_goalYaw) * k;
    m_pitch += (m_goalPitch - m_pitch) * k;
    recomputeOrbitPosition();
}

void Camera::snap() {
    m_target = m_goalTarget;
    m_distance = m_goalDistance;
    m_yaw = m_goalYaw;
    m_pitch = m_goalPitch;
    recomputeOrbitPosition();
}

glm::vec3 Camera::forward() const {
    if (m_mode == CameraMode::Free) return sphericalDir(m_freeYaw, m_freePitch);
    return glm::normalize(m_target - m_position);
}

glm::mat4 Camera::viewMatrix() const {
    return glm::lookAt(m_position, m_position + forward(), glm::vec3(0, 1, 0));
}

glm::mat4 Camera::projectionMatrix(float aspect) const {
    return glm::perspective(glm::radians(fov), aspect, 0.05f, 400.0f);
}

void Camera::setMode(CameraMode mode) {
    if (mode == m_mode) return;
    if (mode == CameraMode::Free) {
        // Продължаваме от текущата позиция, гледайки в същата посока.
        m_freeYaw = m_yaw + 180.0f;
        m_freePitch = -m_pitch;
    } else {
        // Връщаме се в Orbit около точка пред камерата.
        float d = std::clamp(m_distance, 4.0f, 30.0f);
        glm::vec3 f = forward();
        m_target = m_goalTarget = m_position + f * d;
        m_distance = m_goalDistance = d;
        m_yaw = m_goalYaw = m_freeYaw - 180.0f;
        m_pitch = m_goalPitch = -m_freePitch;
    }
    m_mode = mode;
}

void Camera::orbit(float dYaw, float dPitch) {
    m_goalYaw += dYaw;
    m_goalPitch = std::clamp(m_goalPitch + dPitch, -5.0f, 89.0f);
}

void Camera::zoom(float steps) {
    m_goalDistance = std::clamp(m_goalDistance * std::pow(0.88f, steps), 2.0f, 70.0f);
}

void Camera::pan(float dx, float dy) {
    glm::vec3 f = glm::normalize(m_target - m_position);
    glm::vec3 right = glm::normalize(glm::cross(f, glm::vec3(0, 1, 0)));
    glm::vec3 up = glm::cross(right, f);
    float scale = m_distance * 0.0015f;
    m_goalTarget += (-right * dx + up * dy) * scale;
}

void Camera::flyTo(const glm::vec3& target, float distance, float yawDeg, float pitchDeg) {
    setMode(CameraMode::Orbit);
    m_goalTarget = target;
    m_goalDistance = distance;
    m_goalYaw = m_yaw + angleDelta(m_yaw, yawDeg);
    m_goalPitch = pitchDeg;
}

void Camera::flyToTarget(const glm::vec3& target, float distance) {
    setMode(CameraMode::Orbit);
    m_goalTarget = target;
    m_goalDistance = distance;
}

void Camera::look(float dYaw, float dPitch) {
    m_freeYaw += dYaw;
    m_freePitch = std::clamp(m_freePitch + dPitch, -89.0f, 89.0f);
}

void Camera::move(const glm::vec3& localDir, float dt, bool fast) {
    glm::vec3 f = sphericalDir(m_freeYaw, m_freePitch);
    glm::vec3 right = glm::normalize(glm::cross(f, glm::vec3(0, 1, 0)));
    float speed = (fast ? 9.0f : 3.5f) * dt;
    m_position += (f * localDir.z + right * localDir.x + glm::vec3(0, 1, 0) * localDir.y) * speed;
}
