#include "scene/HouseBuilder.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

// ============================================================================
// Размери на вилата (в метри). Началото на координатната система е в центъра
// на сградата на нивото на пода; +Z е лицето (входът), +Y е нагоре.
// ============================================================================
namespace {
constexpr float HX = 6.0f;          // половин ширина по X
constexpr float HZ = 4.0f;          // половин дълбочина по Z
constexpr float FLOOR_H = 3.0f;     // височина на етаж
constexpr float EXT_T = 0.2f;       // дебелина на външна стена
constexpr float INT_T = 0.12f;      // дебелина на вътрешна стена
constexpr float INT_H = 2.9f;       // височина на вътрешна стена (до плочата)

// Стаите се създават в този ред, затова индексите им са постоянни.
enum RoomId { LIVING = 0, KITCHEN, HALL1, BATH, BED1, BED2, HALL2, STUDY };

glm::vec3 rgb(int r, int g, int b) { return glm::vec3(r, g, b) / 255.0f; }
} // namespace

// ============================================================================
// Помощни функции
// ============================================================================
Material HouseBuilder::mat(const std::string& name, glm::vec3 color, const std::string& tex,
                           float tiling, float specular, float shininess) {
    Material m;
    m.name = name;
    m.color = color;
    m.texture = m_scene.textures.get(tex);
    m.tiling = tiling;
    m.specular = specular;
    m.shininess = shininess;
    return m;
}

GameObject& HouseBuilder::shape(Mesh* mesh, const std::string& name, ObjectCategory cat, glm::vec3 center,
                                glm::vec3 size, const Material& m, int floor, int room) {
    GameObject o;
    o.name = name;
    o.category = cat;
    o.mesh = mesh;
    o.material = m;
    o.originalMaterial = m;
    o.transform.position = center;
    o.transform.scale = size;
    o.floor = floor;
    o.roomId = room;
    m_scene.objects.push_back(o);
    return m_scene.objects.back();
}

GameObject& HouseBuilder::box(const std::string& name, ObjectCategory cat, glm::vec3 center, glm::vec3 size,
                              const Material& m, int floor, int room) {
    return shape(m_scene.cube, name, cat, center, size, m, floor, room);
}

GameObject& HouseBuilder::round(Mesh* mesh, const std::string& name, ObjectCategory cat, glm::vec3 center,
                                glm::vec3 size, const Material& m, int floor, int room) {
    GameObject& o = shape(mesh, name, cat, center, size, m, floor, room);
    o.material.uvMode = o.originalMaterial.uvMode = UVMode::Mesh;
    return o;
}

// Част от мебел: позицията е зададена в локалната система на мебелта
// (local.y е височината на центъра над основата), след което се завърта около Y.
GameObject& HouseBuilder::part(const std::string& name, Mesh* mesh, glm::vec3 base, float rotY, glm::vec3 local,
                               glm::vec3 size, const Material& m, int floor, int room) {
    glm::mat4 r = glm::rotate(glm::mat4(1.0f), glm::radians(rotY), glm::vec3(0, 1, 0));
    glm::vec3 offset = glm::vec3(r * glm::vec4(local, 0.0f));
    GameObject& o = shape(mesh, name, ObjectCategory::Furniture, base + offset, size, m, floor, room);
    o.transform.rotation.y = rotY;
    if (mesh != m_scene.cube) o.material.uvMode = o.originalMaterial.uvMode = UVMode::Mesh;
    return o;
}

// ============================================================================
// Стени с отвори. Стената се разделя на правоъгълни сегменти: пълни части
// между отворите, както и части под и над всеки отвор.
// ============================================================================
void HouseBuilder::wall(const std::string& name, bool alongX, float fixed, float a0, float a1, float y0,
                        float height, float thickness, const Material& m, int floor, std::vector<Opening> ops) {
    std::sort(ops.begin(), ops.end(), [](const Opening& a, const Opening& b) { return a.center < b.center; });

    auto segment = [&](float s0, float s1, float bottom, float top) {
        if (s1 - s0 < 0.001f || top - bottom < 0.001f) return;
        float along = (s0 + s1) * 0.5f, len = s1 - s0;
        float y = y0 + (bottom + top) * 0.5f, h = top - bottom;
        glm::vec3 pos  = alongX ? glm::vec3(along, y, fixed) : glm::vec3(fixed, y, along);
        glm::vec3 size = alongX ? glm::vec3(len, h, thickness) : glm::vec3(thickness, h, len);
        box(name, ObjectCategory::Wall, pos, size, m, floor);
    };

    float cur = a0;
    for (const auto& op : ops) {
        float s = op.center - op.width * 0.5f, e = op.center + op.width * 0.5f;
        segment(cur, s, 0.0f, height);         // пълна част преди отвора
        segment(s, e, 0.0f, op.bottom);        // под прозореца
        segment(s, e, op.top, height);         // над отвора (преклад)
        cur = e;
    }
    segment(cur, a1, 0.0f, height);
}

// Външната стена има два слоя: външен (фасада) и вътрешен (мазилка).
void HouseBuilder::exteriorWall(const std::string& name, bool alongX, float fixed, float outwardSign,
                                float a0, float a1, float y0, int floor, const Material& outer,
                                const std::vector<Opening>& ops) {
    const float outerT = 0.12f, innerT = 0.08f;
    float outerFixed = fixed + outwardSign * (EXT_T * 0.5f - outerT * 0.5f);
    float innerFixed = fixed - outwardSign * (EXT_T * 0.5f - innerT * 0.5f);

    // Слоевете са с различна дължина, за да се затворят ъглите без застъпване.
    float inset = alongX ? 0.12f : 0.0f;
    float outerExtend = alongX ? 0.0f : 0.08f;
    wall(name + " (facade)", alongX, outerFixed, a0 - outerExtend, a1 + outerExtend, y0, FLOOR_H, outerT, outer, floor, ops);
    wall(name, alongX, innerFixed, a0 + inset, a1 - inset, y0, FLOOR_H, innerT, m_intWall, floor, ops);

    for (const auto& op : ops)
        if (op.window) windowFrame(alongX, fixed, outwardSign, op, y0, floor);
}

// Тънка алуминиева рамка в антрацит, стъкло и (при по-широките прозорци) вертикални делители.
void HouseBuilder::windowFrame(bool alongX, float fixed, float outwardSign, const Opening& op, float y0, int floor) {
    const float f = 0.05f, depth = 0.22f;
    float yb = y0 + op.bottom, yt = y0 + op.top, ym = (yb + yt) * 0.5f, h = op.top - op.bottom;
    float s = op.center - op.width * 0.5f, e = op.center + op.width * 0.5f;

    auto place = [&](const std::string& n, float along, float y, float len, float hh, float d, float off,
                     const Material& m) {
        glm::vec3 pos  = alongX ? glm::vec3(along, y, fixed + off) : glm::vec3(fixed + off, y, along);
        glm::vec3 size = alongX ? glm::vec3(len, hh, d) : glm::vec3(d, hh, len);
        box(n, ObjectCategory::Window, pos, size, m, floor);
    };

    place("Glass", op.center, ym, op.width, h, 0.03f, 0.0f, m_glass);
    place("Window frame", s + f * 0.5f, ym, f, h, depth, 0.0f, m_frame);
    place("Window frame", e - f * 0.5f, ym, f, h, depth, 0.0f, m_frame);
    place("Window frame", op.center, yt - f * 0.5f, op.width, f, depth, 0.0f, m_frame);
    if (op.bottom > 0.3f)   // подпрозоречна дъска от травертин, издадена навън
        place("Window sill", op.center, yb + 0.025f, op.width + 0.12f, 0.05f, 0.18f, outwardSign * 0.1f, m_travertine);
    place("Window frame", op.center, yb + f * 0.5f, op.width, f, depth, 0.0f, m_frame);

    int mullions = static_cast<int>(op.width / 1.25f);
    for (int i = 1; i <= mullions; ++i)
        place("Window frame", s + op.width * i / (mullions + 1), ym, f * 0.8f, h, depth * 0.8f, 0.0f, m_frame);
}

// ============================================================================
// Стаи и осветителни тела
// ============================================================================
int HouseBuilder::addRoom(const std::string& name, int floor, float x0, float x1, float z0, float z1,
                          const Material& floorMat, const std::string& description, bool makeFloor) {
    Room r;
    r.id = static_cast<int>(m_scene.rooms.size());
    r.name = name;
    r.floor = floor;
    float yBase = (floor - 1) * FLOOR_H;
    r.boundsMin = {x0, yBase, z0};
    r.boundsMax = {x1, yBase + FLOOR_H, z1};
    r.description = description;
    m_scene.rooms.push_back(r);

    if (makeFloor) {
        GameObject& f = box("Floor – " + name, ObjectCategory::Floor,
                            {(x0 + x1) * 0.5f, yBase + 0.01f, (z0 + z1) * 0.5f},
                            {x1 - x0, 0.02f, z1 - z0}, floorMat, floor, r.id);
        f.isRoomFloor = true;
    }
    return r.id;
}

int HouseBuilder::addCeilingLamp(int roomIndex, float x, float z, float c, LampStyle style) {
    Room& room = m_scene.rooms[roomIndex];
    const int fl = room.floor;

    PointLight light;
    light.name = "Lamp – " + room.name;
    light.roomId = roomIndex;
    light.zoneMin = room.boundsMin - glm::vec3(0.02f);
    light.zoneMax = room.boundsMax + glm::vec3(0.02f);
    light.intensity = 1.6f;
    light.radius = 7.5f;
    int li = static_cast<int>(m_scene.pointLights.size());

    Material bulb = mat("Bulb", rgb(255, 246, 225), "none", 1, 0.5f, 64);
    Material cord = mat("Cord", rgb(25, 25, 25), "none", 1, 0.1f, 8);
    auto lampPart = [&](Mesh* mesh, const std::string& n, glm::vec3 p, glm::vec3 s, const Material& m, float glow) {
        GameObject& o = round(mesh, n, ObjectCategory::Lamp, p, s, m, fl, roomIndex);
        o.lightIndex = li;
        o.glow = glow;
        o.ceilingFixture = style == LampStyle::Flush;
    };

    switch (style) {
        case LampStyle::Pendant:
            lampPart(m_scene.cylinder, "Lamp cord", {x, c - 0.3f, z}, {0.015f, 0.6f, 0.015f}, cord, 0);
            lampPart(m_scene.cone, "Brass shade", {x, c - 0.72f, z}, {0.5f, 0.3f, 0.5f}, m_brass, 0.25f);
            lampPart(m_scene.sphere, "Bulb", {x, c - 0.86f, z}, glm::vec3(0.12f), bulb, 1.0f);
            light.position = {x, c - 0.9f, z};
            break;
        case LampStyle::Chandelier: {
            lampPart(m_scene.cylinder, "Chandelier – rod", {x, c - 0.25f, z}, {0.03f, 0.5f, 0.03f}, m_brass, 0);
            lampPart(m_scene.cylinder, "Chandelier – ring", {x, c - 0.55f, z}, {0.95f, 0.03f, 0.95f}, m_brass, 0);
            lampPart(m_scene.cylinder, "Chandelier – ring", {x, c - 0.75f, z}, {0.55f, 0.03f, 0.55f}, m_brass, 0);
            for (int i = 0; i < 10; ++i) {
                float a = i * 6.2832f / 10.0f;
                lampPart(m_scene.sphere, "Bulb", {x + 0.45f * std::cos(a), c - 0.5f, z + 0.45f * std::sin(a)},
                         glm::vec3(0.1f), bulb, 1.0f);
            }
            for (int i = 0; i < 6; ++i) {
                float a = i * 6.2832f / 6.0f + 0.5f;
                lampPart(m_scene.sphere, "Bulb", {x + 0.25f * std::cos(a), c - 0.7f, z + 0.25f * std::sin(a)},
                         glm::vec3(0.09f), bulb, 1.0f);
            }
            lampPart(m_scene.sphere, "Chandelier – crystal", {x, c - 0.9f, z}, {0.14f, 0.22f, 0.14f}, m_glass, 0.6f);
            light.position = {x, c - 0.7f, z};
            break;
        }
        case LampStyle::Flush:
            lampPart(m_scene.cylinder, "Ceiling light", {x, c - 0.03f, z}, {0.6f, 0.06f, 0.6f}, bulb, 0.9f);
            lampPart(m_scene.cylinder, "Ceiling light – ring", {x, c - 0.035f, z}, {0.64f, 0.05f, 0.64f}, m_brass, 0);
            light.position = {x, c - 0.3f, z};
            break;
        case LampStyle::Trio:
            for (int i = -1; i <= 1; ++i) {
                float px = x + i * 0.7f;
                lampPart(m_scene.cylinder, "Lamp cord", {px, c - 0.35f, z}, {0.015f, 0.7f, 0.015f}, cord, 0);
                lampPart(m_scene.cone, "Lamp shade", {px, c - 0.82f, z}, {0.32f, 0.26f, 0.32f}, m_anthracite, 0.1f);
                lampPart(m_scene.sphere, "Bulb", {px, c - 0.95f, z}, glm::vec3(0.1f), bulb, 1.0f);
            }
            light.position = {x, c - 1.0f, z};
            break;
    }

    m_scene.pointLights.push_back(light);
    room.lightIndex = li;
    return li;
}

int HouseBuilder::addOutdoorLight(const std::string& name, glm::vec3 pos, glm::vec3 color, float intensity,
                                  float radius) {
    PointLight l;
    l.name = name;
    l.position = pos;
    l.color = color;
    l.roomId = -1;
    l.zoneMin = m_scene.houseMin;     // свети само ИЗВЪН къщата
    l.zoneMax = m_scene.houseMax;
    l.invertZone = true;
    l.intensity = intensity;
    l.radius = radius;
    m_scene.pointLights.push_back(l);
    return static_cast<int>(m_scene.pointLights.size()) - 1;
}

// ============================================================================
// Конструкция: плочи, стени, прозорци, врати
// ============================================================================
void HouseBuilder::buildStructure() {
    Material facade2 = mat("White facade render", rgb(246, 245, 241), "plaster", 0.5f, 0.05f, 8);
    Material slab    = mat("Concrete slab", rgb(210, 208, 202), "plaster", 0.5f, 0.05f, 8);
    Material plinth  = mat("Plinth", rgb(70, 70, 72), "stone", 1.0f, 0.1f, 16);
    Material parquet = mat("Oak parquet", glm::vec3(1.0f), "parquet", 1.25f, 0.35f, 48);
    Material marbleFloor = m_marble;
    marbleFloor.name = "Marble floor";
    Material darkMarble = mat("Dark marble", rgb(150, 150, 155), "marble", 1.0f, 0.7f, 96);
    Material carpet  = mat("Carpet", rgb(205, 195, 180), "fabric", 2.0f, 0.02f, 4);

    // ---------------------- Стаи (редът определя индексите) ----------------------
    addRoom("Living room",       1, -HX, 0,   -HZ, HZ, marbleFloor, "Living room with a fireplace, corner sofa and dining for six.");
    addRoom("Kitchen",     1, 0,   HX,  0,   HZ, marbleFloor, "Kitchen with an island, marble worktops and a built-in fridge.");
    addRoom("Hallway",   1, 0,   3,   -HZ, 0,  marbleFloor, "Entrance hall with a floating staircase and glass railing.");
    addRoom("Bathroom",      1, 3,   HX,  -HZ, 0,  darkMarble, "Bathroom with a freestanding bathtub, shower and double sink.");
    addRoom("Bedroom 1",  2, -HX, 0,   0,   HZ, parquet, "Master bedroom with access to the balcony.");
    addRoom("Bedroom 2",  2, -HX, 0,   -HZ, 0,  carpet, "Guest bedroom with a desk.");
    addRoom("Hallway (floor 2)", 2, 0, 3, -HZ, HZ, parquet, "Gallery above the staircase.", false);
    addRoom("Study",   2, 3,   HX,  -HZ, HZ, parquet, "Study with a bookshelf and reading corner.");

    // Подът на коридора на 2-ри етаж има отвор за стълбището (x 0..1.6, z -4..-0.9).
    {
        GameObject& a = box("Floor – Hallway (floor 2)", ObjectCategory::Floor, {2.3f, 3.01f, -2.45f},
                            {1.4f, 0.02f, 3.1f}, parquet, 2, HALL2);
        a.isRoomFloor = true;
        GameObject& b = box("Floor – Hallway (floor 2)", ObjectCategory::Floor, {1.5f, 3.01f, 1.55f},
                            {3.0f, 0.02f, 4.9f}, parquet, 2, HALL2);
        b.isRoomFloor = true;
    }

    // ---------------------- Плочи ----------------------
    box("Foundation", ObjectCategory::Floor, {0, -0.075f, 0}, {2 * HX + EXT_T + 0.04f, 0.15f, 2 * HZ + EXT_T + 0.04f}, plinth, 0);
    const float ix = HX - EXT_T * 0.5f, iz = HZ - EXT_T * 0.5f; // вътрешни граници 5.9 / 3.9
    box("Floor slab", ObjectCategory::Floor, {-ix * 0.5f, 2.95f, 0}, {ix, 0.1f, 2 * iz}, slab, 2);
    box("Floor slab", ObjectCategory::Floor, {(1.6f + ix) * 0.5f, 2.95f, 0}, {ix - 1.6f, 0.1f, 2 * iz}, slab, 2);
    box("Floor slab", ObjectCategory::Floor, {0.8f, 2.95f, (-0.9f + iz) * 0.5f}, {1.6f, 0.1f, iz + 0.9f}, slab, 2);
    box("Ceiling", ObjectCategory::Roof, {0, 5.95f, 0}, {2 * ix, 0.1f, 2 * iz}, slab, 3);

    // ---------------------- Външни стени ----------------------
    for (int fl = 1; fl <= 2; ++fl) {
        float y0 = (fl - 1) * FLOOR_H;
        const Material& outer = fl == 1 ? m_travertine : facade2;
        std::vector<Opening> front, back, left, right;
        if (fl == 1) {
            front = {{-1.5f, 1.6f, 0.0f, 2.5f, false},                 // двукрила входна врата
                     {-4.2f, 2.0f, 0.1f, 2.6f, true},                  // панорамен прозорец на хола
                     {3.0f, 1.8f, 0.9f, 2.5f, true}};
            back  = {{-3.2f, 3.0f, 0.05f, 2.6f, true},                 // стъклена стена към терасата
                     {4.5f, 0.7f, 1.5f, 2.3f, true}};
            left  = {{0.0f, 2.4f, 0.1f, 2.6f, true}};
            right = {{2.0f, 1.2f, 1.05f, 2.4f, true}};                 // над кухненския плот
        } else {
            front = {{-3.0f, 2.4f, 0.05f, 2.6f, true},                 // врата към балкона
                     {1.5f, 0.8f, 0.9f, 2.4f, true}, {4.5f, 1.8f, 0.5f, 2.6f, true}};
            back  = {{-3.0f, 1.8f, 1.0f, 2.5f, true}, {4.5f, 1.8f, 0.5f, 2.6f, true}};
            left  = {{2.0f, 1.2f, 0.9f, 2.4f, true}, {-2.0f, 1.2f, 0.9f, 2.4f, true}};
            right = {{0.0f, 1.8f, 0.5f, 2.6f, true}};
        }
        std::string suffix = fl == 1 ? " (floor 1)" : " (floor 2)";
        exteriorWall("Front wall" + suffix, true, HZ, +1, -HX - EXT_T * 0.5f, HX + EXT_T * 0.5f, y0, fl, outer, front);
        exteriorWall("Back wall" + suffix, true, -HZ, -1, -HX - EXT_T * 0.5f, HX + EXT_T * 0.5f, y0, fl, outer, back);
        exteriorWall("Left wall" + suffix, false, -HX, -1, -iz, iz, y0, fl, outer, left);
        exteriorWall("Right wall" + suffix, false, HX, +1, -iz, iz, y0, fl, outer, right);
    }

    // Акцентни панели от дървена обшивка върху фасадата.
    box("Wood cladding", ObjectCategory::Wall, {0.7f, 1.5f, HZ + 0.12f}, {2.4f, 3.0f, 0.04f}, m_cladding, 1);
    box("Wood cladding", ObjectCategory::Wall, {2.75f, 4.5f, HZ + 0.12f}, {1.3f, 3.0f, 0.04f}, m_cladding, 2);
    box("Wood cladding", ObjectCategory::Wall, {HX + 0.12f, 4.5f, -2.6f}, {0.04f, 3.0f, 2.4f}, m_cladding, 2);

    // ---------------------- Вътрешни стени ----------------------
    const float hw = INT_T * 0.5f;
    wall("Wall living room/kitchen", false, 0.0f, -iz, iz, 0, INT_H, INT_T, m_intWall, 1, {{2.0f, 1.6f, 0, 2.4f, false}});
    wall("Wall kitchen/hallway", true, 0.0f, hw, ix, 0, INT_H, INT_T, m_intWall, 1, {{1.8f, 1.0f, 0, 2.1f, false}});
    wall("Wall hallway/bathroom", false, 3.0f, -iz, -hw, 0, INT_H, INT_T, m_intWall, 1, {{-2.0f, 0.9f, 0, 2.1f, false}});
    const float y2 = FLOOR_H;
    wall("Wall bedrooms/hallway", false, 0.0f, -iz, iz, y2, INT_H, INT_T, m_intWall, 2,
         {{-0.45f, 0.8f, 0, 2.1f, false}, {2.0f, 0.9f, 0, 2.1f, false}});
    wall("Wall between bedrooms", true, 0.0f, -ix, -hw, y2, INT_H, INT_T, m_intWall, 2, {});
    wall("Wall hallway/study", false, 3.0f, -iz, iz, y2, INT_H, INT_T, m_intWall, 2, {{1.0f, 0.9f, 0, 2.1f, false}});

    // ---------------------- Двукрила входна врата (въртене около пантите) ----------------------
    Material doorMat = mat("Front door (walnut)", rgb(120, 78, 50), "cladding", 1.0f, 0.35f, 48);
    for (int side = -1; side <= 1; side += 2) {
        // Касата е вътре в отвора (x -2.3..-0.7), за да няма съвпадащи стени (z-fighting).
        float hinge = side < 0 ? -2.24f : -0.76f;
        GameObject& d = box("Front door", ObjectCategory::Door, {hinge, 1.22f, HZ}, {0.74f, 2.44f, 0.07f}, doorMat, 1, LIVING);
        d.transform.pivotOffset = {-side * 0.37f, 0.0f, 0.0f};
        d.isDoor = true;
        d.doorOpenAngle = side < 0 ? 95.0f : -95.0f;   // двете крила се отварят навътре
        GameObject& h = box("Door handle", ObjectCategory::Door, {hinge, 1.22f, HZ}, {0.03f, 1.1f, 0.04f}, m_brass, 1, LIVING);
        h.transform.pivotOffset = {-side * 0.66f, 0.0f, 0.08f};
        h.isDoor = true;
        h.doorOpenAngle = d.doorOpenAngle;
    }
    box("Door frame", ObjectCategory::Door, {-2.27f, 1.25f, HZ}, {0.06f, 2.5f, 0.24f}, m_frame, 1);
    box("Door frame", ObjectCategory::Door, {-0.73f, 1.25f, HZ}, {0.06f, 2.5f, 0.24f}, m_frame, 1);
    box("Door frame", ObjectCategory::Door, {-1.5f, 2.47f, HZ}, {1.48f, 0.06f, 0.24f}, m_frame, 1);
}

// ============================================================================
// Вход, козирка-балкон, колони, аплици, саксии
// ============================================================================
void HouseBuilder::buildEntranceAndBalcony() {
    Material deck = mat("Wooden decking", rgb(220, 200, 180), "cladding", 1.0f, 0.15f, 16);
    Material slabWhite = mat("White slab", rgb(246, 245, 241), "plaster", 0.5f, 0.05f, 8);

    // Балконът над входа е и козирка. Принадлежи на 2-ри етаж (скрива се в изглед „Етаж 1“).
    const float x0 = -4.6f, x1 = -0.4f, z0 = HZ + EXT_T * 0.5f, z1 = 5.6f;
    const float cx = (x0 + x1) * 0.5f, cz = (z0 + z1) * 0.5f, w = x1 - x0, d = z1 - z0;
    box("Balcony – slab", ObjectCategory::Floor, {cx, 2.925f, cz}, {w, 0.15f, d}, slabWhite, 2);
    box("Balcony – wooden deck", ObjectCategory::Floor, {cx, 3.015f, cz}, {w - 0.04f, 0.03f, d - 0.02f}, deck, 2);
    box("Canopy – cladding", ObjectCategory::Floor, {cx, 2.84f, cz}, {w - 0.1f, 0.02f, d - 0.05f}, m_cladding, 2);
    box("Glass railing", ObjectCategory::Window, {cx, 3.55f, z1 - 0.03f}, {w, 1.0f, 0.02f}, m_glass, 2);
    box("Glass railing", ObjectCategory::Window, {x0 + 0.03f, 3.55f, cz}, {0.02f, 1.0f, d}, m_glass, 2);
    box("Glass railing", ObjectCategory::Window, {x1 - 0.03f, 3.55f, cz}, {0.02f, 1.0f, d}, m_glass, 2);
    box("Handrail", ObjectCategory::Window, {cx, 4.07f, z1 - 0.03f}, {w + 0.02f, 0.04f, 0.05f}, m_anthracite, 2);
    box("Handrail", ObjectCategory::Window, {x0 + 0.03f, 4.07f, cz}, {0.05f, 0.04f, d}, m_anthracite, 2);
    box("Handrail", ObjectCategory::Window, {x1 - 0.03f, 4.07f, cz}, {0.05f, 0.04f, d}, m_anthracite, 2);
    for (float x : {x0 + 0.1f, x1 - 0.1f})
        round(m_scene.cylinder, "Column", ObjectCategory::Yard, {x, 1.425f, z1 - 0.12f}, {0.14f, 2.85f, 0.14f}, m_anthracite, 0);

    // Стъпало пред входа
    box("Entrance step", ObjectCategory::Yard, {-1.5f, 0.0f, 4.55f}, {2.4f, 0.12f, 0.7f}, m_travertine, 0);

    // Аплици от двете страни на вратата
    Material lampGlass = mat("Wall light", rgb(255, 240, 210), "none", 1, 0.5f, 64);
    for (float x : {-2.65f, -0.35f}) {
        int li = addOutdoorLight(x < -1 ? "Wall light left of entrance" : "Wall light right of entrance",
                                 {x, 2.2f, HZ + 0.4f}, {1.0f, 0.82f, 0.55f}, 1.3f, 6.0f);
        GameObject& a = box("Wall light", ObjectCategory::Lamp, {x, 2.2f, HZ + 0.2f}, {0.12f, 0.35f, 0.12f}, m_anthracite, 1);
        a.lightIndex = li;
        GameObject& g = box("Wall light – glow", ObjectCategory::Lamp, {x, 2.2f, HZ + 0.265f}, {0.08f, 0.28f, 0.01f}, lampGlass, 1);
        g.lightIndex = li;
        g.glow = 1.0f;
    }

    // Саксии с топиари
    for (float x : {-3.2f, 0.2f}) {
        box("Planter", ObjectCategory::Yard, {x, 0.3f, 4.7f}, {0.55f, 0.6f, 0.55f}, m_anthracite, 0);
        round(m_scene.sphere, "Topiary", ObjectCategory::Yard, {x, 0.95f, 4.7f}, glm::vec3(0.7f),
              mat("Boxwood", rgb(70, 115, 55), "grass", 3, 0.05f, 8), 0);
    }
}

// ============================================================================
// Висящо стълбище (10 стъпала по 30 cm) със стъклен парапет
// ============================================================================
void HouseBuilder::buildStairs() {
    Material tread = mat("Step (walnut)", rgb(150, 100, 65), "wood", 1.0f, 0.35f, 48);
    const int steps = 10;
    const float rise = FLOOR_H / steps, run = 0.29f, z0 = -3.85f, x = 0.8f, width = 1.3f;
    for (int i = 0; i < steps; ++i) {
        float top = (i + 1) * rise;
        box("Step " + std::to_string(i + 1), ObjectCategory::Stairs,
            {x, top - 0.04f, z0 + (i + 0.5f) * run}, {width, 0.08f, run - 0.01f}, tread, 1, HALL1);
    }
    // Носеща стена-стрингер покрай стената x = 0
    box("Stair stringer", ObjectCategory::Stairs, {0.1f, 1.45f, -2.4f}, {0.08f, 2.9f, 2.9f}, m_intWall, 1, HALL1);

    // Наклонен стъклен парапет: кутия, завъртяна около оста X на ъгъла на стълбището.
    const float len = std::sqrt(steps * run * steps * run + FLOOR_H * FLOOR_H);
    const float angle = glm::degrees(std::atan2(FLOOR_H, steps * run));
    GameObject& g = box("Glass railing", ObjectCategory::Stairs, {x + width * 0.5f, 1.5f + 0.5f, z0 + steps * run * 0.5f},
                        {0.02f, 0.9f, len}, m_glass, 1, HALL1);
    g.transform.rotation.x = -angle;
    GameObject& r = box("Handrail", ObjectCategory::Stairs, {x + width * 0.5f, 1.5f + 0.97f, z0 + steps * run * 0.5f},
                        {0.05f, 0.04f, len}, m_brass, 1, HALL1);
    r.transform.rotation.x = -angle;

    // Стъклен парапет около отвора на 2-ри етаж
    const float y = FLOOR_H;
    box("Glass railing", ObjectCategory::Stairs, {1.62f, y + 0.5f, -2.35f}, {0.02f, 1.0f, 3.0f}, m_glass, 2, HALL2);
    box("Handrail", ObjectCategory::Stairs, {1.62f, y + 1.02f, -2.35f}, {0.05f, 0.04f, 3.0f}, m_brass, 2, HALL2);
}

// ============================================================================
// Плосък покрив с козирка, капандура (призма), фотоволтаици и комин
// ============================================================================
void HouseBuilder::buildRoof() {
    Material fascia = mat("Roof fascia", rgb(246, 245, 241), "plaster", 0.5f, 0.05f, 8);
    Material membrane = mat("Roof gravel", rgb(150, 148, 142), "stone", 3.0f, 0.05f, 8);
    Material solar = mat("Solar panel", rgb(55, 75, 130), "tiles", 3.0f, 1.0f, 128);

    const float base = 2 * FLOOR_H;
    box("Soffit cladding", ObjectCategory::Roof, {0, base - 0.025f, 0}, {12.8f, 0.05f, 9.0f}, m_cladding, 3);
    box("Roof slab", ObjectCategory::Roof, {0, base + 0.15f, 0}, {12.8f, 0.3f, 9.0f}, fascia, 3);
    box("Roof membrane", ObjectCategory::Roof, {0, base + 0.31f, 0}, {12.4f, 0.02f, 8.6f}, membrane, 3);

    // Капандура над стълбището – стъклена призма, завъртяна на 90° около Y.
    box("Skylight – frame", ObjectCategory::Roof, {0.8f, base + 0.35f, -2.4f}, {1.4f, 0.06f, 2.8f}, m_anthracite, 3);
    GameObject& sky = shape(m_scene.prism, "Skylight", ObjectCategory::Roof, {0.8f, base + 0.63f, -2.4f},
                            {2.7f, 0.5f, 1.3f}, m_glass, 3);
    sky.transform.rotation.y = 90.0f;

    // Фотоволтаични панели, наклонени на 25° към юг (+Z).
    for (float z : {2.2f, -0.6f})
        for (float x : {2.9f, 4.05f, 5.2f}) {
            GameObject& p = box("Solar panel", ObjectCategory::Roof, {x, base + 0.62f, z}, {1.05f, 0.04f, 1.7f}, solar, 3);
            p.transform.rotation.x = 25.0f;
            box("Panel mount", ObjectCategory::Roof, {x, base + 0.52f, z - 0.7f}, {0.9f, 0.4f, 0.04f}, m_anthracite, 3);
        }

    round(m_scene.cylinder, "Fireplace chimney", ObjectCategory::Roof, {-0.4f, base + 0.9f, -1.3f}, {0.32f, 1.2f, 0.32f}, m_anthracite, 3);
    round(m_scene.cylinder, "Chimney cap", ObjectCategory::Roof, {-0.4f, base + 1.52f, -1.3f}, {0.45f, 0.05f, 0.45f}, m_metal, 3);
}

// ============================================================================
// Мебели
// ============================================================================
void HouseBuilder::table(const std::string& name, float x, float z, float y, float w, float d, float h,
                         int floor, int room, const Material& top, const Material& legs) {
    glm::vec3 base(x, y, z);
    part(name, m_scene.cube, base, 0, {0, h - 0.025f, 0}, {w, 0.05f, d}, top, floor, room);
    float lx = w * 0.5f - 0.08f, lz = d * 0.5f - 0.08f;
    for (int i = 0; i < 4; ++i) {
        float sx = (i & 1) ? lx : -lx, sz = (i & 2) ? lz : -lz;
        part("Leg of " + name, m_scene.cube, base, 0, {sx, (h - 0.05f) * 0.5f, sz}, {0.05f, h - 0.05f, 0.05f},
             legs, floor, room);
    }
}

// Тапициран стол – по подразбиране гледа към +Z (облегалката е отзад, при -Z).
void HouseBuilder::chair(float x, float z, float y, float rotY, int floor, int room) {
    glm::vec3 base(x, y, z);
    Material seat = m_fabric;
    seat.color = rgb(215, 205, 185);
    part("Chair", m_scene.cube, base, rotY, {0, 0.46f, 0}, {0.46f, 0.08f, 0.46f}, seat, floor, room);
    part("Chair back", m_scene.cube, base, rotY, {0, 0.76f, -0.2f}, {0.46f, 0.55f, 0.06f}, seat, floor, room);
    for (int i = 0; i < 4; ++i) {
        float sx = (i & 1) ? 0.19f : -0.19f, sz = (i & 2) ? 0.19f : -0.19f;
        part("Chair leg", m_scene.cube, base, rotY, {sx, 0.21f, sz}, {0.04f, 0.42f, 0.04f}, m_walnut, floor, room);
    }
}

void HouseBuilder::stool(float x, float z, float y, int floor, int room) {
    glm::vec3 base(x, y, z);
    part("Bar stool – base", m_scene.cylinder, base, 0, {0, 0.015f, 0}, {0.38f, 0.03f, 0.38f}, m_brass, floor, room);
    part("Bar stool – leg", m_scene.cylinder, base, 0, {0, 0.38f, 0}, {0.05f, 0.72f, 0.05f}, m_brass, floor, room);
    part("Bar stool", m_scene.cylinder, base, 0, {0, 0.76f, 0}, {0.42f, 0.08f, 0.42f}, m_leather, floor, room);
}

// Диван/фотьойл – гледа към +Z.
void HouseBuilder::sofa(const std::string& name, float x, float z, float y, float rotY, float width,
                        glm::vec3 color, int floor, int room, bool pillows) {
    glm::vec3 base(x, y, z);
    Material f = m_fabric;
    f.color = color;
    part(name, m_scene.cube, base, rotY, {0, 0.06f, 0}, {width - 0.1f, 0.12f, 0.8f}, m_walnut, floor, room);
    part(name, m_scene.cube, base, rotY, {0, 0.26f, 0}, {width, 0.28f, 0.9f}, f, floor, room);
    part("Seat – " + name, m_scene.cube, base, rotY, {0, 0.46f, 0.06f}, {width - 0.3f, 0.14f, 0.74f}, f, floor, room);
    part("Backrest – " + name, m_scene.cube, base, rotY, {0, 0.66f, -0.35f}, {width, 0.52f, 0.2f}, f, floor, room);
    for (int s = -1; s <= 1; s += 2)
        part("Armrest – " + name, m_scene.cube, base, rotY, {s * (width * 0.5f - 0.075f), 0.4f, 0},
             {0.15f, 0.56f, 0.9f}, f, floor, room);
    if (pillows && width > 1.5f) {
        Material p = m_fabric;
        p.color = rgb(190, 150, 90);
        for (int s = -1; s <= 1; s += 2)
            part("Cushion", m_scene.cube, base, rotY, {s * (width * 0.5f - 0.4f), 0.68f, -0.18f},
                 {0.42f, 0.38f, 0.14f}, p, floor, room);
    }
}

// Легло с висока тапицирана табла (таблата е към -Z).
void HouseBuilder::bed(const std::string& name, float x, float z, float y, float w, float l, int floor, int room,
                       glm::vec3 blanket, glm::vec3 headboard, float headHeight) {
    glm::vec3 base(x, y, z);
    Material sheet = m_fabric;
    sheet.color = glm::vec3(0.96f);
    Material cover = m_fabric;
    cover.color = blanket;
    Material throwM = m_fabric;
    throwM.color = blanket * 0.55f;
    Material head = m_fabric;
    head.color = headboard;
    part(name, m_scene.cube, base, 0, {0, 0.16f, 0}, {w + 0.1f, 0.32f, l}, m_walnut, floor, room);
    part("Mattress", m_scene.cube, base, 0, {0, 0.42f, 0.02f}, {w - 0.04f, 0.2f, l - 0.12f}, sheet, floor, room);
    part("Duvet", m_scene.cube, base, 0, {0, 0.54f, l * 0.14f}, {w, 0.05f, l * 0.7f}, cover, floor, room);
    part("Bed throw", m_scene.cube, base, 0, {0, 0.57f, l * 0.36f}, {w + 0.02f, 0.05f, l * 0.22f}, throwM, floor, room);
    part("Headboard", m_scene.cube, base, 0, {0, headHeight * 0.5f, -l * 0.5f + 0.05f}, {w + 0.3f, headHeight, 0.1f}, head,
         floor, room);
    int pillows = w > 1.45f ? 2 : 1;
    for (int i = 0; i < pillows; ++i) {
        float px = pillows == 1 ? 0.0f : (i == 0 ? -w * 0.25f : w * 0.25f);
        part("Pillow", m_scene.cube, base, 0, {px, 0.6f, -l * 0.5f + 0.32f}, {w / pillows - 0.15f, 0.14f, 0.38f},
             sheet, floor, room);
    }
}

void HouseBuilder::wardrobe(float x, float z, float y, float sx, float sz, int floor, int room) {
    glm::vec3 base(x, y, z);
    part("Wardrobe", m_scene.cube, base, 0, {0, 1.2f, 0}, {sx, 2.4f, sz}, m_walnut, floor, room);
    // Две вертикални месингови дръжки от страната към стаята.
    bool alongZ = sz > sx;
    float sign = alongZ ? (x < 0 ? 1.0f : -1.0f) : (z < 0 ? 1.0f : -1.0f);
    for (int s = -1; s <= 1; s += 2) {
        glm::vec3 local = alongZ ? glm::vec3(sign * (sx * 0.5f + 0.015f), 1.15f, s * 0.05f)
                                 : glm::vec3(s * 0.05f, 1.15f, sign * (sz * 0.5f + 0.015f));
        part("Handle", m_scene.cube, base, 0, local, {0.02f, 0.5f, 0.02f}, m_brass, floor, room);
    }
}

void HouseBuilder::bookshelf(float x, float z, float y, float sx, float sz, int floor, int room) {
    // Строим в локална система: дължина по X, дълбочина по Z, после завъртаме при нужда.
    bool rotated = sz > sx;
    float len = rotated ? sz : sx, depth = rotated ? sx : sz;
    // Гърбът на библиотеката е към най-близката стена.
    float rot = rotated ? (x > 0 ? -90.0f : 90.0f) : (z > 0 ? 180.0f : 0.0f);
    glm::vec3 base(x, y, z);
    const float h = 2.2f;
    part("Bookshelf", m_scene.cube, base, rot, {0, h * 0.5f, -depth * 0.5f + 0.01f}, {len, h, 0.02f}, m_walnut, floor, room);
    for (int s = -1; s <= 1; s += 2)
        part("Bookshelf", m_scene.cube, base, rot, {s * (len * 0.5f - 0.015f), h * 0.5f, 0}, {0.03f, h, depth}, m_walnut, floor, room);

    const glm::vec3 bookColors[] = {rgb(130, 40, 40), rgb(30, 55, 95), rgb(40, 90, 60), rgb(200, 170, 110),
                                    rgb(70, 45, 90), rgb(230, 225, 210), rgb(30, 30, 35), rgb(160, 90, 50)};
    unsigned seed = static_cast<unsigned>(std::fabs(x * 131 + z * 71)) + 7u;
    auto rnd = [&seed]() { seed = seed * 1103515245u + 12345u; return ((seed >> 16) & 0x7FFF) / 32767.0f; };

    const int shelves = 5;
    for (int i = 0; i <= shelves; ++i) {
        float sy = i * (h - 0.03f) / shelves + 0.015f;
        part("Shelf", m_scene.cube, base, rot, {0, sy, 0}, {len - 0.06f, 0.03f, depth}, m_walnut, floor, room);
        if (i == shelves) break;
        float cursor = -len * 0.5f + 0.05f;
        while (cursor < len * 0.5f - 0.12f) {
            float r = rnd();
            if (r < 0.08f) {   // декоративна ваза
                part("Vase", m_scene.sphere, base, rot, {cursor + 0.07f, sy + 0.13f, 0.02f}, {0.13f, 0.22f, 0.13f}, m_brass, floor, room);
                cursor += 0.18f;
                continue;
            }
            if (r < 0.16f) { cursor += 0.1f; continue; } // празно място
            float bw = 0.03f + 0.035f * rnd();
            float bh = 0.2f + 0.12f * rnd();
            Material bm = mat("Book", bookColors[static_cast<int>(rnd() * 7.99f)], "none", 1, 0.2f, 16);
            part("Book", m_scene.cube, base, rot, {cursor + bw * 0.5f, sy + 0.015f + bh * 0.5f, 0.02f},
                 {bw, bh, depth * 0.75f}, bm, floor, room);
            cursor += bw + 0.004f;
        }
    }
}

void HouseBuilder::plant(float x, float z, float y, float s, int floor, int room) {
    Material leaves = mat("Plant", rgb(55, 120, 55), "grass", 3, 0.1f, 8);
    round(m_scene.cylinder, "Planter", ObjectCategory::Furniture, {x, y + 0.25f * s, z}, glm::vec3(0.38f, 0.5f, 0.38f) * s, m_anthracite, floor, room);
    round(m_scene.sphere, "Plant", ObjectCategory::Furniture, {x, y + 0.85f * s, z}, glm::vec3(0.6f, 0.8f, 0.6f) * s, leaves, floor, room);
}

// Абстрактна картина: рамка, платно и три цветни полета.
void HouseBuilder::painting(bool alongX, float fixed, float facing, float along, float y, float w, float h,
                            int floor, int room, int palette) {
    auto at = [&](float off, float a, float yy, float ww, float hh, float d, const Material& m, const char* n) {
        glm::vec3 pos  = alongX ? glm::vec3(along + a, yy, fixed + facing * off) : glm::vec3(fixed + facing * off, yy, along + a);
        glm::vec3 size = alongX ? glm::vec3(ww, hh, d) : glm::vec3(d, hh, ww);
        box(n, ObjectCategory::Furniture, pos, size, m, floor, room);
    };
    const glm::vec3 palettes[2][3] = {{rgb(190, 120, 60), rgb(40, 55, 90), rgb(200, 180, 140)},
                                      {rgb(60, 110, 120), rgb(220, 200, 170), rgb(150, 60, 50)}};
    const glm::vec3* p = palettes[palette % 2];
    at(0.015f, 0, y, w + 0.08f, h + 0.08f, 0.03f, m_brass, "Picture frame");
    at(0.035f, 0, y, w, h, 0.02f, mat("Canvas", rgb(240, 236, 226), "fabric", 6, 0.02f, 4), "Painting");
    at(0.047f, -w * 0.18f, y + h * 0.12f, w * 0.45f, h * 0.55f, 0.005f, mat("Paint", p[0], "fabric", 6, 0.05f, 8), "Painting");
    at(0.047f, w * 0.22f, y - h * 0.15f, w * 0.35f, h * 0.45f, 0.005f, mat("Paint", p[1], "fabric", 6, 0.05f, 8), "Painting");
    at(0.047f, w * 0.05f, y + h * 0.3f, w * 0.25f, h * 0.15f, 0.005f, mat("Paint", p[2], "fabric", 6, 0.05f, 8), "Painting");
}

// Нощна лампа – абажурът свети заедно с лампата на стаята.
void HouseBuilder::tableLamp(float x, float z, float y, int floor, int room) {
    int li = m_scene.rooms[room].lightIndex;
    Material shade = mat("Lamp shade", rgb(240, 228, 205), "fabric", 4, 0.05f, 8);
    round(m_scene.sphere, "Bedside lamp", ObjectCategory::Lamp, {x, y + 0.14f, z}, {0.18f, 0.26f, 0.18f}, m_brass, floor, room).lightIndex = li;
    GameObject& s = round(m_scene.cylinder, "Lamp shade", ObjectCategory::Lamp, {x, y + 0.38f, z}, {0.3f, 0.22f, 0.3f}, shade, floor, room);
    s.lightIndex = li;
    s.glow = 0.45f;
}

void HouseBuilder::floorLamp(float x, float z, float y, int floor, int room) {
    int li = m_scene.rooms[room].lightIndex;
    Material shade = mat("Lamp shade", rgb(240, 228, 205), "fabric", 4, 0.05f, 8);
    round(m_scene.cylinder, "Floor lamp – base", ObjectCategory::Lamp, {x, y + 0.015f, z}, {0.35f, 0.03f, 0.35f}, m_brass, floor, room).lightIndex = li;
    round(m_scene.cylinder, "Floor lamp", ObjectCategory::Lamp, {x, y + 0.8f, z}, {0.03f, 1.6f, 0.03f}, m_brass, floor, room).lightIndex = li;
    GameObject& s = round(m_scene.cylinder, "Lamp shade", ObjectCategory::Lamp, {x, y + 1.6f, z}, {0.45f, 0.32f, 0.45f}, shade, floor, room);
    s.lightIndex = li;
    s.glow = 0.4f;
}

void HouseBuilder::buildFurniture() {
    const float y1 = 0.02f, y2 = FLOOR_H + 0.02f;
    Material black = mat("Black glass", rgb(15, 15, 18), "none", 1, 0.9f, 128);
    Material rugBeige = mat("Rug", rgb(205, 190, 165), "fabric", 3, 0.02f, 4);
    Material rugGrey = mat("Rug", rgb(120, 125, 130), "fabric", 3, 0.02f, 4);
    Material water = mat("Water", rgb(200, 230, 240), "water", 1.0f, 0.9f, 128);
    Material mirror = mat("Mirror", rgb(205, 218, 228), "none", 1, 1.0f, 256);
    Material flame = mat("Fire", rgb(255, 140, 40), "none", 1, 0.0f, 8);
    Material stoneDark = mat("Dark stone", rgb(55, 55, 58), "stone", 1.0f, 0.3f, 32);
    Material porcelain = mat("Porcelain", rgb(250, 250, 250), "none", 1, 0.6f, 96);

    // --- Таванни лампи (първо, за да могат нощните лампи да се свържат с тях) ---
    const float c1 = INT_H, c2 = FLOOR_H + INT_H;
    addCeilingLamp(LIVING, -2.2f, 2.55f, c1, LampStyle::Chandelier);
    addCeilingLamp(KITCHEN, 3.2f, 2.35f, c1, LampStyle::Trio);
    addCeilingLamp(HALL1, 2.2f, -2.0f, c1, LampStyle::Flush);
    addCeilingLamp(BATH, 4.5f, -1.6f, c1, LampStyle::Flush);
    addCeilingLamp(BED1, -3.0f, 2.3f, c2, LampStyle::Pendant);
    addCeilingLamp(BED2, -3.0f, -1.6f, c2, LampStyle::Pendant);
    addCeilingLamp(HALL2, 2.2f, 1.5f, c2, LampStyle::Flush);
    addCeilingLamp(STUDY, 4.5f, 0.0f, c2, LampStyle::Pendant);

    // ======================= Хол =======================
    // Камина с ТВ над нея на стената към кухнята; зад тях – акцентен панел от дърво.
    box("Accent wall", ObjectCategory::Wall, {-0.08f, 1.45f, -1.3f}, {0.03f, 2.9f, 2.6f}, m_cladding, 1, LIVING);
    box("Fireplace", ObjectCategory::Furniture, {-0.31f, y1 + 0.55f, -1.3f}, {0.5f, 1.1f, 2.2f}, m_marble, 1, LIVING);
    box("Firebox", ObjectCategory::Furniture, {-0.565f, y1 + 0.42f, -1.3f}, {0.02f, 0.42f, 1.3f}, stoneDark, 1, LIVING);
    {
        PointLight fire;
        fire.name = "Fireplace";
        fire.position = {-1.0f, 0.5f, -1.3f};
        fire.color = {1.0f, 0.55f, 0.2f};
        fire.intensity = 1.4f;
        fire.radius = 5.0f;
        fire.roomId = LIVING;
        fire.zoneMin = m_scene.rooms[LIVING].boundsMin - glm::vec3(0.02f);
        fire.zoneMax = m_scene.rooms[LIVING].boundsMax + glm::vec3(0.02f);
        fire.flicker = true;
        m_scene.pointLights.push_back(fire);
        GameObject& f = box("Fire", ObjectCategory::Lamp, {-0.58f, y1 + 0.36f, -1.3f}, {0.01f, 0.22f, 1.1f}, flame, 1, LIVING);
        f.lightIndex = static_cast<int>(m_scene.pointLights.size()) - 1;
        f.glow = 1.2f;
    }
    box("Hearth slab", ObjectCategory::Furniture, {-0.75f, 0.04f, -1.3f}, {0.5f, 0.04f, 2.2f}, stoneDark, 1, LIVING);
    box("TV", ObjectCategory::Furniture, {-0.13f, 1.85f, -1.3f}, {0.04f, 0.8f, 1.4f}, black, 1, LIVING);

    // Ъглов диван, кръгла маса с мраморен плот, фотьойли, килим
    sofa("Corner sofa", -5.45f, -0.3f, y1, 90, 2.6f, rgb(200, 196, 188), 1, LIVING);
    sofa("Corner sofa", -4.22f, -1.95f, y1, 0, 1.6f, rgb(200, 196, 188), 1, LIVING, false);
    round(m_scene.cylinder, "Coffee table", ObjectCategory::Furniture, {-3.9f, y1 + 0.38f, -0.3f}, {1.0f, 0.05f, 1.0f}, m_marble, 1, LIVING);
    round(m_scene.cylinder, "Table base", ObjectCategory::Furniture, {-3.9f, y1 + 0.18f, -0.3f}, {0.35f, 0.36f, 0.35f}, m_brass, 1, LIVING);
    box("Rug", ObjectCategory::Furniture, {-3.8f, 0.028f, -0.3f}, {2.8f, 0.012f, 3.4f}, rugBeige, 1, LIVING);
    sofa("Leather armchair", -2.6f, -2.95f, y1, -45, 0.95f, glm::vec3(0.55f, 0.32f, 0.18f), 1, LIVING);
    floorLamp(-5.55f, -3.4f, y1, 1, LIVING);
    bookshelf(-0.95f, -3.7f, y1, 1.4f, 0.35f, 1, LIVING);
    plant(-5.55f, 3.5f, y1, 1.2f, 1, LIVING);

    // Трапезария за шестима
    table("Dining table", -2.2f, 2.55f, y1, 2.0f, 1.0f, 0.76f, 1, LIVING, m_walnut, m_brass);
    for (float x : {-2.8f, -2.2f, -1.6f}) {
        chair(x, 1.75f, y1, 0, 1, LIVING);
        chair(x, 3.35f, y1, 180, 1, LIVING);
    }

    // ======================= Кухня =======================
    box("Base cabinets", ObjectCategory::Furniture, {5.6f, y1 + 0.44f, 2.0f}, {0.6f, 0.88f, 3.7f}, m_anthracite, 1, KITCHEN);
    box("Marble worktop", ObjectCategory::Furniture, {5.58f, y1 + 0.9f, 2.0f}, {0.64f, 0.04f, 3.72f}, m_marble, 1, KITCHEN);
    box("Cooktop", ObjectCategory::Furniture, {5.6f, y1 + 0.925f, 3.1f}, {0.5f, 0.01f, 0.6f}, black, 1, KITCHEN);
    box("Sink", ObjectCategory::Furniture, {5.6f, y1 + 0.925f, 0.9f}, {0.45f, 0.01f, 0.55f}, m_metal, 1, KITCHEN);
    round(m_scene.cylinder, "Faucet", ObjectCategory::Furniture, {5.82f, y1 + 1.1f, 0.9f}, {0.04f, 0.35f, 0.04f}, m_brass, 1, KITCHEN);
    box("Upper cabinets", ObjectCategory::Furniture, {5.72f, 1.95f, 3.33f}, {0.35f, 0.7f, 1.05f}, m_anthracite, 1, KITCHEN);
    box("Upper cabinets", ObjectCategory::Furniture, {5.72f, 1.95f, 0.68f}, {0.35f, 0.7f, 1.05f}, m_anthracite, 1, KITCHEN);
    box("Tall unit with built-in fridge", ObjectCategory::Furniture, {3.875f, y1 + 1.2f, 0.37f}, {2.75f, 2.4f, 0.62f}, m_anthracite, 1, KITCHEN);
    for (float x : {2.9f, 3.6f, 4.3f, 5.0f})
        box("Handle", ObjectCategory::Furniture, {x, y1 + 1.1f, 0.695f}, {0.02f, 0.7f, 0.02f}, m_brass, 1, KITCHEN);
    // Остров с бар столове
    box("Kitchen island", ObjectCategory::Furniture, {3.2f, y1 + 0.45f, 2.35f}, {2.1f, 0.9f, 0.85f}, m_anthracite, 1, KITCHEN);
    box("Island marble top", ObjectCategory::Furniture, {3.2f, y1 + 0.92f, 2.4f}, {2.3f, 0.04f, 1.05f}, m_marble, 1, KITCHEN);
    for (float x : {2.55f, 3.2f, 3.85f}) stool(x, 3.25f, y1, 1, KITCHEN);
    round(m_scene.sphere, "Fruit bowl", ObjectCategory::Furniture, {3.6f, y1 + 1.0f, 2.35f}, {0.3f, 0.12f, 0.3f}, m_brass, 1, KITCHEN);
    plant(0.45f, 3.5f, y1, 1.0f, 1, KITCHEN);

    // ======================= Коридор (етаж 1) =======================
    box("Console table", ObjectCategory::Furniture, {2.25f, y1 + 0.4f, -3.7f}, {1.1f, 0.06f, 0.35f}, m_walnut, 1, HALL1);
    box("Console – legs", ObjectCategory::Furniture, {2.25f, y1 + 0.2f, -3.7f}, {1.0f, 0.4f, 0.02f}, m_brass, 1, HALL1);
    {
        GameObject& f = round(m_scene.cylinder, "Mirror – frame", ObjectCategory::Furniture, {2.25f, 1.5f, -3.885f}, {0.9f, 0.02f, 0.9f}, m_brass, 1, HALL1);
        f.transform.rotation.x = 90.0f;
        GameObject& m = round(m_scene.cylinder, "Round mirror", ObjectCategory::Furniture, {2.25f, 1.5f, -3.87f}, {0.82f, 0.02f, 0.82f}, mirror, 1, HALL1);
        m.transform.rotation.x = 90.0f;
    }
    plant(2.6f, -0.45f, y1, 0.9f, 1, HALL1);

    // ======================= Баня =======================
    // Свободностояща овална вана (сплескан цилиндър)
    round(m_scene.cylinder, "Freestanding bathtub", ObjectCategory::Furniture, {5.05f, y1 + 0.3f, -3.35f}, {1.6f, 0.6f, 0.8f}, porcelain, 1, BATH);
    round(m_scene.cylinder, "Bath water", ObjectCategory::Furniture, {5.05f, y1 + 0.55f, -3.35f}, {1.45f, 0.02f, 0.66f}, water, 1, BATH);
    round(m_scene.cylinder, "Faucet", ObjectCategory::Furniture, {4.1f, y1 + 0.5f, -3.35f}, {0.05f, 1.0f, 0.05f}, m_brass, 1, BATH);
    // Душ кабина с ъглово стъкло
    box("Shower tray", ObjectCategory::Furniture, {3.55f, y1 + 0.02f, -3.4f}, {0.95f, 0.04f, 0.95f}, stoneDark, 1, BATH);
    box("Shower glass", ObjectCategory::Window, {3.48f, 1.05f, -2.92f}, {0.8f, 2.0f, 0.02f}, m_glass, 1, BATH);
    box("Shower head", ObjectCategory::Furniture, {3.5f, 2.2f, -3.6f}, {0.3f, 0.02f, 0.3f}, m_brass, 1, BATH);
    // Двоен окачен умивалник с мраморен плот и огледало
    box("Vanity unit", ObjectCategory::Furniture, {5.65f, 0.65f, -1.55f}, {0.5f, 0.4f, 1.4f}, m_walnut, 1, BATH);
    box("Marble worktop", ObjectCategory::Furniture, {5.64f, 0.87f, -1.55f}, {0.54f, 0.04f, 1.44f}, m_marble, 1, BATH);
    for (float z : {-1.9f, -1.2f}) {
        round(m_scene.cylinder, "Basin", ObjectCategory::Furniture, {5.6f, 0.94f, z}, {0.36f, 0.1f, 0.36f}, porcelain, 1, BATH);
        round(m_scene.cylinder, "Faucet", ObjectCategory::Furniture, {5.83f, 1.02f, z}, {0.03f, 0.25f, 0.03f}, m_brass, 1, BATH);
    }
    box("Mirror", ObjectCategory::Furniture, {5.88f, 1.65f, -1.55f}, {0.02f, 0.9f, 1.3f}, mirror, 1, BATH);
    box("Wall-hung toilet", ObjectCategory::Furniture, {5.6f, 0.4f, -0.45f}, {0.55f, 0.35f, 0.38f}, porcelain, 1, BATH);
    box("Towel", ObjectCategory::Furniture, {3.1f, 1.2f, -0.9f}, {0.04f, 0.7f, 0.45f},
        mat("Towel", rgb(225, 215, 195), "fabric", 4, 0.02f, 4), 1, BATH);

    // ======================= Спалня 1 =======================
    bed("Double bed", -3.0f, 1.2f, y2, 1.8f, 2.1f, 2, BED1, rgb(235, 228, 212), rgb(120, 135, 140), 1.3f);
    box("Bench", ObjectCategory::Furniture, {-3.0f, y2 + 0.22f, 2.6f}, {1.4f, 0.44f, 0.4f},
        mat("Velvet", rgb(120, 135, 140), "fabric", 4, 0.05f, 8), 2, BED1);
    for (float x : {-4.3f, -1.7f}) {
        box("Nightstand", ObjectCategory::Furniture, {x, y2 + 0.25f, 0.35f}, {0.5f, 0.5f, 0.45f}, m_walnut, 2, BED1);
        tableLamp(x, 0.35f, y2 + 0.5f, 2, BED1);
    }
    wardrobe(-5.6f, 3.35f, y2, 0.6f, 1.0f, 2, BED1);
    box("Rug", ObjectCategory::Furniture, {-3.0f, y2 + 0.008f, 1.6f}, {3.0f, 0.012f, 2.6f}, rugGrey, 2, BED1);
    painting(false, -0.06f, -1.0f, 3.2f, y2 + 1.5f, 1.0f, 0.8f, 2, BED1, 1);

    // ======================= Спалня 2 =======================
    bed("Single bed", -3.0f, -2.85f, y2, 1.4f, 2.0f, 2, BED2, rgb(200, 215, 210), rgb(200, 190, 170), 0.95f);
    box("Nightstand", ObjectCategory::Furniture, {-1.85f, y2 + 0.25f, -3.6f}, {0.45f, 0.5f, 0.45f}, m_walnut, 2, BED2);
    tableLamp(-1.85f, -3.6f, y2 + 0.5f, 2, BED2);
    table("Desk", -5.55f, -0.65f, y2, 0.6f, 1.1f, 0.75f, 2, BED2, m_walnut, m_brass);
    chair(-5.0f, -0.65f, y2, -90, 2, BED2);
    wardrobe(-0.38f, -3.0f, y2, 0.6f, 1.2f, 2, BED2);

    // ======================= Коридор (етаж 2) =======================
    box("Console table", ObjectCategory::Furniture, {2.75f, y2 + 0.4f, 3.2f}, {0.35f, 0.8f, 1.0f}, m_walnut, 2, HALL2);
    painting(false, 2.94f, -1.0f, 3.2f, y2 + 1.6f, 0.9f, 0.7f, 2, HALL2, 0);
    painting(false, 0.06f, 1.0f, 3.3f, y2 + 1.5f, 0.8f, 1.0f, 2, HALL2, 1);
    plant(2.6f, -3.5f, y2, 1.0f, 2, HALL2);

    // ======================= Кабинет =======================
    table("Desk", 4.5f, 3.45f, y2, 1.6f, 0.75f, 0.76f, 2, STUDY, m_walnut, m_anthracite);
    {   // Офис стол с кожена тапицерия
        glm::vec3 b(4.5f, y2, 2.7f);
        part("Office chair – base", m_scene.cylinder, b, 0, {0, 0.03f, 0}, {0.6f, 0.06f, 0.6f}, m_anthracite, 2, STUDY);
        part("Office chair – column", m_scene.cylinder, b, 0, {0, 0.25f, 0}, {0.06f, 0.4f, 0.06f}, m_metal, 2, STUDY);
        part("Office chair", m_scene.cube, b, 0, {0, 0.5f, 0}, {0.55f, 0.1f, 0.55f}, m_leather, 2, STUDY);
        part("Office chair – back", m_scene.cube, b, 0, {0, 0.9f, -0.26f}, {0.55f, 0.75f, 0.08f}, m_leather, 2, STUDY);
    }
    box("Monitor", ObjectCategory::Furniture, {4.5f, y2 + 1.05f, 3.6f}, {0.7f, 0.42f, 0.03f}, black, 2, STUDY);
    box("Monitor stand", ObjectCategory::Furniture, {4.5f, y2 + 0.84f, 3.63f}, {0.06f, 0.16f, 0.06f}, m_metal, 2, STUDY);
    tableLamp(5.1f, 3.5f, y2 + 0.76f, 2, STUDY);
    bookshelf(5.72f, -2.5f, y2, 0.35f, 1.6f, 2, STUDY);
    sofa("Leather armchair", 4.0f, -2.6f, y2, 90, 0.95f, glm::vec3(0.45f, 0.26f, 0.15f), 2, STUDY);
    round(m_scene.cylinder, "Side table", ObjectCategory::Furniture, {4.0f, y2 + 0.5f, -1.6f}, {0.5f, 0.04f, 0.5f}, m_marble, 2, STUDY);
    round(m_scene.cylinder, "Side table – leg", ObjectCategory::Furniture, {4.0f, y2 + 0.25f, -1.6f}, {0.05f, 0.5f, 0.05f}, m_brass, 2, STUDY);
    floorLamp(3.4f, -3.5f, y2, 2, STUDY);
    box("Rug", ObjectCategory::Furniture, {4.4f, y2 + 0.008f, 0.3f}, {2.2f, 0.012f, 3.0f}, rugBeige, 2, STUDY);
    painting(false, 3.06f, 1.0f, -1.8f, y2 + 1.6f, 1.0f, 0.7f, 2, STUDY, 0);
}

// ============================================================================
// Тераса с пергола, басейн, шезлонги
// ============================================================================
void HouseBuilder::lounger(float x, float z, float rotY) {
    glm::vec3 base(x, 0.0f, z);
    Material cushion = m_fabric;
    cushion.color = rgb(240, 238, 230);
    part("Sun lounger", m_scene.cube, base, rotY, {0, 0.15f, 0.1f}, {0.7f, 0.2f, 1.6f}, m_walnut, 0, -1);
    part("Sun lounger – cushion", m_scene.cube, base, rotY, {0, 0.29f, 0.25f}, {0.66f, 0.08f, 1.3f}, cushion, 0, -1);
    GameObject& back = part("Sun lounger – backrest", m_scene.cube, base, rotY, {0, 0.5f, -0.62f}, {0.66f, 0.08f, 0.7f}, cushion, 0, -1);
    back.transform.rotation.x = 40.0f;   // повдигната облегалка (ротация около X, после около Y)
}

void HouseBuilder::buildTerraceAndPool() {
    Material deck = mat("Wooden deck", rgb(215, 190, 160), "cladding", 1.0f, 0.15f, 16);
    Material water = mat("Pool water", glm::vec3(1.0f), "water", 0.35f, 0.5f, 256);
    water.uvScroll = {0.025f, 0.015f};   // текстурата бавно "тече"
    Material outdoorFabric = m_fabric;
    outdoorFabric.color = rgb(225, 222, 214);

    // Дек пред стъклената стена на хола
    box("Terrace", ObjectCategory::Yard, {-2.55f, 0.035f, -6.05f}, {7.1f, 0.17f, 3.9f}, deck, 0);

    // Пергола от антрацитни профили
    for (float x : {-6.0f, 1.0f})
        for (float z : {-4.35f, -7.85f})
            box("Pergola – post", ObjectCategory::Yard, {x, 1.5f, z}, {0.12f, 2.9f, 0.12f}, m_anthracite, 0);
    for (float z : {-4.35f, -7.85f})
        box("Pergola – beam", ObjectCategory::Yard, {-2.5f, 2.9f, z}, {7.15f, 0.16f, 0.12f}, m_anthracite, 0);
    for (float x = -5.7f; x <= 0.75f; x += 0.5f)
        box("Pergola – louvre", ObjectCategory::Yard, {x, 3.04f, -6.1f}, {0.06f, 0.14f, 3.75f}, m_anthracite, 0);

    // LED осветление под перголата
    {
        int li = addOutdoorLight("Pergola light", {-2.5f, 2.6f, -6.1f}, {1.0f, 0.85f, 0.6f}, 1.3f, 6.5f);
        GameObject& led = box("LED strip", ObjectCategory::Lamp, {-2.5f, 2.81f, -6.1f}, {4.0f, 0.03f, 0.08f},
                              mat("LED", rgb(255, 240, 215), "none", 1, 0.2f, 16), 0);
        led.lightIndex = li;
        led.glow = 1.0f;
    }

    // Градински кът
    sofa("Garden sofa", -3.0f, -4.95f, 0.12f, 180, 2.4f, outdoorFabric.color, 0, -1);
    round(m_scene.cylinder, "Garden table", ObjectCategory::Furniture, {-3.0f, 0.42f, -6.2f}, {1.0f, 0.06f, 1.0f}, m_travertine, 0);
    round(m_scene.cylinder, "Table base", ObjectCategory::Furniture, {-3.0f, 0.27f, -6.2f}, {0.4f, 0.3f, 0.4f}, m_anthracite, 0);
    sofa("Garden armchair", -4.5f, -7.2f, 0.12f, 30, 0.95f, outdoorFabric.color, 0, -1);
    sofa("Garden armchair", -1.5f, -7.2f, 0.12f, -30, 0.95f, outdoorFabric.color, 0, -1);

    // Басейн 8 × 4 m с бордюр от травертин и подводна светлина
    const float px = -0.5f, pz = -11.2f, pw = 8.0f, pd = 4.0f, c = 0.5f;
    box("Pool coping", ObjectCategory::Yard, {px, 0.05f, pz - pd * 0.5f - c * 0.5f}, {pw + 2 * c, 0.2f, c}, m_travertine, 0);
    box("Pool coping", ObjectCategory::Yard, {px, 0.05f, pz + pd * 0.5f + c * 0.5f}, {pw + 2 * c, 0.2f, c}, m_travertine, 0);
    box("Pool coping", ObjectCategory::Yard, {px - pw * 0.5f - c * 0.5f, 0.05f, pz}, {c, 0.2f, pd}, m_travertine, 0);
    box("Pool coping", ObjectCategory::Yard, {px + pw * 0.5f + c * 0.5f, 0.05f, pz}, {c, 0.2f, pd}, m_travertine, 0);
    int li = addOutdoorLight("Pool light", {px, 0.3f, pz}, {0.35f, 0.8f, 1.0f}, 1.5f, 7.0f);
    GameObject& w = box("Pool", ObjectCategory::Yard, {px, 0.06f, pz}, {pw, 0.1f, pd}, water, 0);
    w.lightIndex = li;
    w.glow = 0.35f;

    // Шезлонги и чадър
    lounger(5.3f, -10.2f, -90.0f);
    lounger(5.3f, -12.2f, -90.0f);
    round(m_scene.cylinder, "Parasol – pole", ObjectCategory::Furniture, {5.6f, 1.15f, -11.2f}, {0.05f, 2.3f, 0.05f}, m_anthracite, 0);
    round(m_scene.cone, "Parasol", ObjectCategory::Furniture, {5.6f, 2.3f, -11.2f}, {2.6f, 0.45f, 2.6f}, outdoorFabric, 0);
}

// ============================================================================
// Двор: морава, пътека, жив плет, дървета, градински лампи
// ============================================================================
void HouseBuilder::tree(float x, float z, bool conifer, float s) {
    Material bark = mat("Bark", rgb(105, 80, 60), "wood", 2, 0.05f, 8);
    Material leaves = mat("Crown", conifer ? rgb(95, 150, 100) : rgb(140, 190, 105), "grass", 2, 0.05f, 8);
    round(m_scene.cylinder, "Tree – trunk", ObjectCategory::Yard, {x, 1.0f * s, z}, glm::vec3(0.3f, 2.0f, 0.3f) * s, bark, 0);
    if (conifer) {
        for (int i = 0; i < 3; ++i)
            round(m_scene.cone, "Tree – crown", ObjectCategory::Yard, {x, (2.0f + i * 1.0f) * s, z},
                  glm::vec3(2.6f - i * 0.6f, 2.0f, 2.6f - i * 0.6f) * s, leaves, 0);
    } else {
        round(m_scene.sphere, "Tree – crown", ObjectCategory::Yard, {x, 3.0f * s, z}, glm::vec3(3.0f, 2.6f, 3.0f) * s, leaves, 0);
    }
}

void HouseBuilder::cypress(float x, float z, float s) {
    Material leaves = mat("Cypress", rgb(95, 145, 95), "grass", 3, 0.05f, 8);
    round(m_scene.sphere, "Cypress", ObjectCategory::Yard, {x, 1.9f * s, z}, glm::vec3(0.9f, 3.8f, 0.9f) * s, leaves, 0);
}

void HouseBuilder::buildYard() {
    Material grass = mat("Grass", rgb(240, 245, 230), "grass", 0.25f, 0.03f, 4);
    Material stone = mat("Stone paving", rgb(235, 230, 220), "stone", 1.0f, 0.1f, 16);
    Material hedge = mat("Hedge", rgb(150, 185, 130), "grass", 2.0f, 0.05f, 8);

    GameObject& g = box("Lawn", ObjectCategory::Yard, {0, -0.1f, 0}, {400, 0.1f, 400}, grass, 0);
    g.pickable = false;

    box("Path", ObjectCategory::Yard, {-1.5f, -0.035f, 8.4f}, {1.6f, 0.04f, 7.2f}, stone, 0);

    // Жив плет по периметъра с вход между две колони от травертин.
    const float fx = 16.0f, back = -18.0f, front = 12.0f, gate0 = -2.6f, gate1 = -0.4f;
    box("Hedge", ObjectCategory::Yard, {0, 0.65f, back}, {2 * fx + 0.8f, 1.4f, 0.8f}, hedge, 0);
    box("Hedge", ObjectCategory::Yard, {-fx, 0.65f, (back + front) * 0.5f}, {0.8f, 1.4f, front - back}, hedge, 0);
    box("Hedge", ObjectCategory::Yard, {fx, 0.65f, (back + front) * 0.5f}, {0.8f, 1.4f, front - back}, hedge, 0);
    box("Hedge", ObjectCategory::Yard, {(-fx + gate0 - 0.3f) * 0.5f, 0.65f, front}, {gate0 - 0.3f + fx, 1.4f, 0.8f}, hedge, 0);
    box("Hedge", ObjectCategory::Yard, {(fx + gate1 + 0.3f) * 0.5f, 0.65f, front}, {fx - gate1 - 0.3f, 1.4f, 0.8f}, hedge, 0);
    for (float x : {gate0, gate1}) {
        box("Gate pillar", ObjectCategory::Yard, {x, 0.9f, front}, {0.5f, 1.8f, 0.5f}, m_travertine, 0);
        box("Pillar cap", ObjectCategory::Yard, {x, 1.84f, front}, {0.58f, 0.08f, 0.58f}, m_anthracite, 0);
    }

    // Градински лампи покрай пътеката
    Material glowM = mat("Light", rgb(255, 235, 200), "none", 1, 0.3f, 16);
    const glm::vec2 bollards[] = {{-2.65f, 7.0f}, {-0.35f, 9.8f}};
    for (const auto& b : bollards) {
        int li = addOutdoorLight("Garden light", {b.x, 0.7f, b.y}, {1.0f, 0.82f, 0.55f}, 1.2f, 5.0f);
        GameObject& p = round(m_scene.cylinder, "Garden light", ObjectCategory::Lamp, {b.x, 0.35f, b.y}, {0.16f, 0.7f, 0.16f}, m_anthracite, 0);
        p.lightIndex = li;
        GameObject& t = round(m_scene.cylinder, "Garden light – glow", ObjectCategory::Lamp, {b.x, 0.74f, b.y}, {0.17f, 0.08f, 0.17f}, glowM, 0);
        t.lightIndex = li;
        t.glow = 1.0f;
    }

    tree(-12.0f, -8.0f, true, 1.2f);
    tree(12.0f, -6.0f, false, 1.1f);
    tree(-12.5f, 7.0f, false, 1.2f);
    tree(12.5f, 8.0f, true, 1.0f);
    tree(-10.5f, -15.0f, false, 1.0f);
    tree(10.0f, -15.5f, true, 1.1f);
    for (float z : {-2.5f, 0.0f, 2.5f}) { cypress(-7.3f, z, 1.0f); cypress(7.3f, z, 1.0f); }
    for (float x : {-5.2f, -4.0f, 2.2f, 3.4f, 4.6f})
        round(m_scene.sphere, "Shrub", ObjectCategory::Yard, {x, 0.3f, 4.75f}, {0.8f, 0.6f, 0.7f}, hedge, 0);
}

// Всеки обект от етажите, който още не принадлежи на стая (напр. стени),
// получава стаята, в която се намира центърът му (леко изместен навътре).
void HouseBuilder::assignRooms() {
    for (auto& o : m_scene.objects) {
        if (o.roomId >= 0 || (o.floor != 1 && o.floor != 2)) continue;
        glm::vec3 p = o.transform.worldCenter();
        if (std::fabs(p.x) > HX - 0.3f) p.x -= (p.x > 0 ? 0.35f : -0.35f);
        if (std::fabs(p.z) > HZ - 0.3f) p.z -= (p.z > 0 ? 0.35f : -0.35f);
        for (const auto& r : m_scene.rooms) {
            if (r.floor == o.floor && p.x >= r.boundsMin.x && p.x <= r.boundsMax.x &&
                p.z >= r.boundsMin.z && p.z <= r.boundsMax.z) {
                o.roomId = r.id;
                break;
            }
        }
    }
}

void HouseBuilder::build() {
    m_glass      = mat("Glass", rgb(150, 180, 195), "none", 1, 1.0f, 160);
    m_glass.opacity = 0.3f;
    m_frame      = mat("Aluminium frame", rgb(48, 50, 54), "none", 1, 0.5f, 64);
    m_intWall    = mat("Interior plaster", rgb(244, 241, 235), "plaster", 0.7f, 0.05f, 8);
    m_wood       = mat("Wood", glm::vec3(1.0f), "wood", 1.5f, 0.3f, 32);
    m_walnut     = mat("Walnut", rgb(135, 95, 70), "wood", 1.5f, 0.35f, 48);
    m_fabric     = mat("Fabric", glm::vec3(1.0f), "fabric", 3.0f, 0.03f, 4);
    m_white      = mat("White lacquer", rgb(242, 242, 240), "none", 1, 0.5f, 64);
    m_metal      = mat("Stainless steel", rgb(185, 190, 195), "none", 1, 0.8f, 96);
    m_brass      = mat("Brass", rgb(200, 160, 80), "none", 1, 1.0f, 128);
    m_anthracite = mat("Anthracite", rgb(52, 54, 58), "none", 1, 0.4f, 48);
    m_marble     = mat("Marble", glm::vec3(1.0f), "marble", 1.0f, 0.7f, 96);
    m_leather    = mat("Leather", rgb(110, 60, 35), "fabric", 6, 0.35f, 24);
    m_travertine = mat("Travertine", glm::vec3(1.0f), "travertine", 1.0f, 0.1f, 16);
    m_cladding   = mat("Wood cladding", glm::vec3(1.0f), "cladding", 1.0f, 0.15f, 24);

    // Зоната обхваща вътрешността на къщата; външните лампи осветяват всичко извън нея
    // (фасадите са на ±6.1 / ±4.1 m, а входната врата – на 4.035 m).
    m_scene.houseMin = {-HX - 0.02f, -1.0f, -HZ - 0.02f};
    m_scene.houseMax = {HX + 0.02f, 2 * FLOOR_H, HZ + 0.02f};

    buildStructure();
    buildEntranceAndBalcony();
    buildStairs();
    buildRoof();
    buildFurniture();
    buildTerraceAndPool();
    buildYard();
    assignRooms();
}
