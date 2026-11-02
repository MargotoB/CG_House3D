#pragma once
// Scene – съдържа всички обекти, стаи, светлини и споделени ресурси (мешове, текстури).

#include "graphics/Mesh.h"
#include "graphics/Texture.h"
#include "scene/GameObject.h"
#include "scene/Light.h"
#include "scene/Room.h"

#include <memory>
#include <string>
#include <vector>

class Scene {
public:
    void init(const std::string& resourceDir);
    void update(float dt);

    // Споделени мешове
    Mesh* cube = nullptr;
    Mesh* plane = nullptr;
    Mesh* cylinder = nullptr;
    Mesh* cone = nullptr;
    Mesh* sphere = nullptr;
    Mesh* prism = nullptr;

    TextureLibrary textures;

    std::vector<GameObject> objects;
    std::vector<Room> rooms;
    std::vector<PointLight> pointLights;
    DirectionalLight sun;

    // Габарит на сградата – използва се от външните лампи.
    glm::vec3 houseMin{0.0f}, houseMax{0.0f};

    void setAllLamps(bool on);
    void toggleDoor(GameObject& door);
    size_t totalTriangles() const;

private:
    std::vector<std::unique_ptr<Mesh>> m_meshes;
};
