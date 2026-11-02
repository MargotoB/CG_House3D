#include "scene/Scene.h"

#include "graphics/Primitives.h"
#include "scene/HouseBuilder.h"

#include <cmath>

void Scene::init(const std::string& resourceDir) {
    auto keep = [this](std::unique_ptr<Mesh> m) {
        Mesh* raw = m.get();
        m_meshes.push_back(std::move(m));
        return raw;
    };
    cube     = keep(Primitives::cube());
    plane    = keep(Primitives::plane());
    cylinder = keep(Primitives::cylinder(28));
    cone     = keep(Primitives::cone(28));
    sphere   = keep(Primitives::sphere(16, 24));
    prism    = keep(Primitives::prism());

    textures.createAll(resourceDir);

    HouseBuilder builder(*this);
    builder.build();
}

void Scene::update(float dt) {
    // Плавно отваряне/затваряне на вратите (ротация около пантата).
    for (auto& obj : objects) {
        if (!obj.isDoor) continue;
        float diff = obj.doorTarget - obj.doorAngle;
        if (std::fabs(diff) < 0.01f) obj.doorAngle = obj.doorTarget;
        else obj.doorAngle += diff * (1.0f - std::exp(-dt * 6.0f));
        obj.transform.rotation.y = obj.doorAngle;
    }
}

void Scene::setAllLamps(bool on) {
    for (auto& l : pointLights) l.enabled = on;
}

void Scene::toggleDoor(GameObject& door) {
    if (!door.isDoor) return;
    door.doorTarget = (door.doorTarget == 0.0f) ? door.doorOpenAngle : 0.0f;
}

size_t Scene::totalTriangles() const {
    size_t n = 0;
    for (const auto& o : objects)
        if (o.mesh) n += o.mesh->triangleCount();
    return n;
}
