#pragma once
// GameObject – обект в сцената. Следва схемата GameObject → Mesh + Material + Transform.

#include "graphics/Material.h"
#include "graphics/Mesh.h"
#include "scene/Transform.h"
#include <string>

enum class ObjectCategory { Wall, Floor, Window, Door, Furniture, Lamp, Stairs, Roof, Yard };

inline const char* categoryName(ObjectCategory c) {
    switch (c) {
        case ObjectCategory::Wall:      return "Wall";
        case ObjectCategory::Floor:     return "Floor / slab";
        case ObjectCategory::Window:    return "Window";
        case ObjectCategory::Door:      return "Door";
        case ObjectCategory::Furniture: return "Furniture";
        case ObjectCategory::Lamp:      return "Light fixture";
        case ObjectCategory::Stairs:    return "Stairs";
        case ObjectCategory::Roof:      return "Roof";
        case ObjectCategory::Yard:      return "Yard";
    }
    return "";
}

struct GameObject {
    std::string name;
    ObjectCategory category = ObjectCategory::Furniture;

    Mesh* mesh = nullptr;          // споделена геометрия (притежава се от Scene)
    Material material;             // собствено копие – може да се редактира поотделно
    Material originalMaterial;     // за бутона "Възстанови"
    Transform transform;

    int floor = 0;                 // 0 – двор, 1/2 – етаж, 3 – покрив/таван
    int roomId = -1;               // към коя стая принадлежи (-1 – никоя)
    int lightIndex = -1;           // за части от лампа – коя светлина управлява
    bool isRoomFloor = false;      // подът на стая се подсветява при избор на стаята
    // Обзавеждането (мебели и осветителни тела) е с фиксиран материал –
    // цветът и текстурата му не могат да се променят от панела.
    bool canEditMaterial() const {
        return category != ObjectCategory::Furniture && category != ObjectCategory::Lamp;
    }
    float glow = 0.0f;             // сила на собственото светене, когато лампата (lightIndex) е включена
    bool visible = true;
    bool ceilingFixture = false;   // плафон – скрива се, когато таванът над етажа е премахнат
    bool pickable = true;

    // Анимирана врата: текущ и целеви ъгъл на завъртане около пантата.
    bool isDoor = false;
    float doorAngle = 0.0f, doorTarget = 0.0f;
    float doorOpenAngle = 95.0f;   // знакът определя посоката на отваряне
};
