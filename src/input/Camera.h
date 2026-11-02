#pragma once
// Camera – два режима:
//   Orbit – камерата обикаля около точка (target) на разстояние distance;
//   Free  – свободно движение (WASD + мишка), като в 3D игра.
// В режим Orbit всички промени са плавни: задава се целево състояние (goal),
// към което текущото се приближава експоненциално всеки кадър.

#include <glm/glm.hpp>

enum class CameraMode { Orbit, Free };

class Camera {
public:
    Camera();

    void update(float dt);

    glm::mat4 viewMatrix() const;
    glm::mat4 projectionMatrix(float aspect) const;
    glm::vec3 position() const { return m_position; }
    glm::vec3 forward() const;

    CameraMode mode() const { return m_mode; }
    void setMode(CameraMode mode);

    // --- Orbit ---
    void orbit(float dYaw, float dPitch);
    void zoom(float steps);
    void pan(float dx, float dy);
    void flyTo(const glm::vec3& target, float distance, float yawDeg, float pitchDeg);
    void flyToTarget(const glm::vec3& target, float distance); // запазва текущия ъгъл
    void snap();                                                // моментално достига целта

    // --- Free ---
    void look(float dYaw, float dPitch);
    void move(const glm::vec3& localDir, float dt, bool fast);

    float yaw() const { return m_yaw; }
    float pitch() const { return m_pitch; }
    float distance() const { return m_distance; }
    glm::vec3 target() const { return m_target; }

    float fov = 50.0f;

private:
    void recomputeOrbitPosition();

    CameraMode m_mode = CameraMode::Orbit;

    // Текущо състояние (orbit)
    glm::vec3 m_target{0.0f, 3.0f, 0.0f};
    float m_distance = 24.0f, m_yaw = 60.0f, m_pitch = 22.0f;
    // Целево състояние (orbit)
    glm::vec3 m_goalTarget{0.0f, 3.0f, 0.0f};
    float m_goalDistance = 24.0f, m_goalYaw = 60.0f, m_goalPitch = 22.0f;

    // Free
    glm::vec3 m_position{0.0f};
    float m_freeYaw = 0.0f, m_freePitch = 0.0f;
};
