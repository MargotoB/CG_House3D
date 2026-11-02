#pragma once
// HouseBuilder – изгражда двуетажната модерна вила, обзавеждането и двора
// изцяло от геометрични примитиви (куб, цилиндър, конус, сфера, призма).

#include "scene/Scene.h"
#include <string>
#include <vector>

class HouseBuilder {
public:
    explicit HouseBuilder(Scene& scene) : m_scene(scene) {}
    void build();

private:
    // Отвор в стена (врата или прозорец). Координатите са по дължината на стената,
    // а bottom/top – височини спрямо основата на стената.
    struct Opening {
        float center, width, bottom, top;
        bool window;
    };

    enum class LampStyle { Pendant, Chandelier, Flush, Trio };

    // --- Базови помощни функции ---
    Material mat(const std::string& name, glm::vec3 color, const std::string& tex,
                 float tiling = 1.0f, float specular = 0.15f, float shininess = 24.0f);
    GameObject& box(const std::string& name, ObjectCategory cat, glm::vec3 center, glm::vec3 size,
                    const Material& m, int floor, int room = -1);
    GameObject& shape(Mesh* mesh, const std::string& name, ObjectCategory cat, glm::vec3 center,
                      glm::vec3 size, const Material& m, int floor, int room = -1);
    // Кръгли тела (цилиндър/сфера/конус) – с UV координати от самата геометрия.
    GameObject& round(Mesh* mesh, const std::string& name, ObjectCategory cat, glm::vec3 center,
                      glm::vec3 size, const Material& m, int floor, int room = -1);
    GameObject& part(const std::string& name, Mesh* mesh, glm::vec3 base, float rotY, glm::vec3 local,
                     glm::vec3 size, const Material& m, int floor, int room);

    // --- Стени ---
    void wall(const std::string& name, bool alongX, float fixed, float a0, float a1, float y0,
              float height, float thickness, const Material& m, int floor, std::vector<Opening> ops);
    void exteriorWall(const std::string& name, bool alongX, float fixed, float outwardSign,
                      float a0, float a1, float y0, int floor, const Material& outer,
                      const std::vector<Opening>& ops);
    void windowFrame(bool alongX, float fixed, float outwardSign, const Opening& op, float y0, int floor);

    // --- Части на къщата ---
    int  addRoom(const std::string& name, int floor, float x0, float x1, float z0, float z1,
                 const Material& floorMat, const std::string& description, bool makeFloor = true);
    int  addCeilingLamp(int room, float x, float z, float ceilingY, LampStyle style);
    int  addOutdoorLight(const std::string& name, glm::vec3 pos, glm::vec3 color, float intensity, float radius);
    void buildStructure();
    void buildEntranceAndBalcony();
    void buildStairs();
    void buildRoof();
    void buildFurniture();
    void buildTerraceAndPool();
    void buildYard();
    void assignRooms();

    // --- Мебели и декорация ---
    void table(const std::string& name, float x, float z, float y, float w, float d, float h, int floor, int room,
               const Material& top, const Material& legs);
    void chair(float x, float z, float y, float rotY, int floor, int room);
    void stool(float x, float z, float y, int floor, int room);
    void sofa(const std::string& name, float x, float z, float y, float rotY, float width,
              glm::vec3 color, int floor, int room, bool pillows = true);
    void bed(const std::string& name, float x, float z, float y, float w, float l, int floor, int room,
             glm::vec3 blanket, glm::vec3 headboard, float headHeight);
    void wardrobe(float x, float z, float y, float sx, float sz, int floor, int room);
    void bookshelf(float x, float z, float y, float sx, float sz, int floor, int room);
    void plant(float x, float z, float y, float scale, int floor, int room);
    void painting(bool alongX, float fixed, float facing, float along, float y, float w, float h,
                  int floor, int room, int palette);
    void tableLamp(float x, float z, float y, int floor, int room);
    void floorLamp(float x, float z, float y, int floor, int room);
    void lounger(float x, float z, float rotY);
    void tree(float x, float z, bool conifer, float scale);
    void cypress(float x, float z, float scale);

    Scene& m_scene;
    Material m_glass, m_frame, m_intWall, m_wood, m_walnut, m_fabric, m_white, m_metal, m_brass,
             m_anthracite, m_marble, m_leather, m_travertine, m_cladding;
};
