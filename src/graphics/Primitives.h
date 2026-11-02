#pragma once
// Primitives – процедурно генериране на основните геометрични примитиви.
// Всички примитиви са с единични размери и център в началото на координатната
// система; реалните размери се задават чрез мащабиране в Transform.

#include "graphics/Mesh.h"
#include <memory>

namespace Primitives {
    std::unique_ptr<Mesh> cube();                       // куб 1×1×1
    std::unique_ptr<Mesh> plane();                      // квадрат 1×1 в равнината XZ, нормала +Y
    std::unique_ptr<Mesh> cylinder(int segments = 24);  // радиус 0.5, височина 1
    std::unique_ptr<Mesh> cone(int segments = 24);      // радиус на основата 0.5, височина 1
    std::unique_ptr<Mesh> sphere(int stacks = 16, int slices = 24); // радиус 0.5
    std::unique_ptr<Mesh> prism();                      // триъгълна призма (фронтон на покрива)
}
