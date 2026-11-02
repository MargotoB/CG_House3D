#include "ui/UIPanel.h"

#include "core/Application.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <filesystem>
#include <string>
#include <vector>

void UIPanel::init(GLFWwindow* window, const std::string& resourceDir) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr; // не записваме imgui.ini

    // Шрифт с кирилица: първо от assets/fonts, след това системни шрифтове.
    const std::vector<std::string> fonts = {
        resourceDir + "/assets/fonts/font.ttf",
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
    };
    bool loaded = false;
    for (const auto& f : fonts) {
        std::error_code ec;
        if (std::filesystem::exists(std::filesystem::u8path(f), ec) && io.Fonts->AddFontFromFileTTF(f.c_str(), 17.0f)) {
            loaded = true;
            break;
        }
    }
    if (!loaded) io.Fonts->AddFontDefault();

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.FrameRounding = 5.0f;
    style.GrabRounding = 5.0f;
    style.WindowBorderSize = 0.0f;
    style.Colors[ImGuiCol_WindowBg].w = 0.90f;
    style.Colors[ImGuiCol_Header] = ImVec4(0.20f, 0.35f, 0.55f, 0.75f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.45f, 0.70f, 0.85f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.20f, 0.33f, 0.50f, 0.85f);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
    m_initialized = true;
}

void UIPanel::shutdown() {
    if (!m_initialized) return;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    m_initialized = false;
}

void UIPanel::draw(Application& app) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(12, 12), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(360, 0), ImGuiCond_Once);
    ImGui::SetNextWindowSizeConstraints(ImVec2(300, 0), ImVec2(600, ImGui::GetIO().DisplaySize.y - 24));
    ImGui::Begin("3D House – Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    drawViewSection(app);
    drawLightingSection(app);
    drawRoomsSection(app);
    drawSelectionSection(app);
    drawHelpSection();

    ImGui::End();

    // Малък информационен надпис в долния десен ъгъл.
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 12, io.DisplaySize.y - 12), ImGuiCond_Always, ImVec2(1, 1));
    ImGui::SetNextWindowBgAlpha(0.55f);
    ImGui::Begin("##stats", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                 ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);
    ImGui::Text("%.0f FPS  |  %d draw calls  |  %zu objects", app.fps(), app.renderer().drawCalls(),
                app.scene().objects.size());
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// ---------------------------------------------------------------------------
void UIPanel::drawViewSection(Application& app) {
    if (!ImGui::CollapsingHeader("View & Camera", ImGuiTreeNodeFlags_DefaultOpen)) return;

    float w = (ImGui::GetContentRegionAvail().x - 2 * ImGui::GetStyle().ItemSpacing.x) / 3.0f;
    for (int i = 0; i < VIEW_COUNT; ++i) {
        if (i % 3 != 0) ImGui::SameLine();
        std::string label = std::to_string(i + 1) + ". " + Application::presetName(i);
        if (ImGui::Button(label.c_str(), ImVec2(w, 0))) app.setViewPreset(i);
    }

    bool free = app.camera().mode() == CameraMode::Free;
    if (ImGui::Checkbox("Free camera (Tab)", &free)) app.toggleFreeCamera();

    ImGui::Spacing();
    ImGui::TextUnformatted("Floors:");
    ImGui::SameLine();
    int fv = static_cast<int>(app.view().floorView);
    if (ImGui::RadioButton("All", fv == 0)) app.setFloorView(FloorView::All);
    ImGui::SameLine();
    if (ImGui::RadioButton("Floor 1", fv == 1)) app.setFloorView(FloorView::Floor1);
    ImGui::SameLine();
    if (ImGui::RadioButton("Floor 2", fv == 2)) app.setFloorView(FloorView::Floor2);

    ImGui::SliderFloat("Field of view", &app.camera().fov, 30.0f, 90.0f, "%.0f°");
    ImGui::Checkbox("Wireframe (P)", &app.view().wireframe);
    ImGui::Spacing();
}

// ---------------------------------------------------------------------------
void UIPanel::drawLightingSection(Application& app) {
    if (!ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_DefaultOpen)) return;
    ViewState& view = app.view();
    Scene& scene = app.scene();

    float w = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) / 2.0f;
    ImGui::BeginDisabled(!view.night);
    if (ImGui::Button("Day", ImVec2(w, 0))) app.setNight(false);
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(view.night);
    if (ImGui::Button("Night", ImVec2(w, 0))) app.setNight(true);
    ImGui::EndDisabled();
    ImGui::ProgressBar(view.nightFactor, ImVec2(-1, 4), "");

    if (ImGui::TreeNode("Lamps")) {
        if (ImGui::SmallButton("All on")) scene.setAllLamps(true);
        ImGui::SameLine();
        if (ImGui::SmallButton("All off")) scene.setAllLamps(false);
        for (size_t i = 0; i < scene.pointLights.size(); ++i) {
            PointLight& l = scene.pointLights[i];
            ImGui::PushID(static_cast<int>(i));
            ImGui::Checkbox(l.name.c_str(), &l.enabled);
            ImGui::PopID();
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Light settings")) {
        ImGui::SliderFloat("Sun / moon", &scene.sun.intensity, 0.0f, 2.0f);
        static float lampIntensity = 1.6f;
        if (ImGui::SliderFloat("Lamp intensity", &lampIntensity, 0.2f, 4.0f))
            for (auto& l : scene.pointLights)
                if (l.roomId >= 0) l.intensity = lampIntensity;
        static float lampColor[3] = {1.0f, 0.85f, 0.6f};
        if (ImGui::ColorEdit3("Lamp colour", lampColor))
            for (auto& l : scene.pointLights) l.color = {lampColor[0], lampColor[1], lampColor[2]};
        ImGui::TextDisabled("Sun intensity resets when switching day/night.");
        ImGui::TreePop();
    }
    ImGui::Spacing();
}

// ---------------------------------------------------------------------------
void UIPanel::drawRoomsSection(Application& app) {
    if (!ImGui::CollapsingHeader("Rooms", ImGuiTreeNodeFlags_DefaultOpen)) return;
    Scene& scene = app.scene();
    for (int floor = 1; floor <= 2; ++floor) {
        ImGui::TextDisabled("Floor %d", floor);
        for (const Room& r : scene.rooms) {
            if (r.floor != floor) continue;
            ImGui::PushID(r.id);
            bool selected = app.view().selectedRoom == r.id;
            std::string label = "   " + r.name;
            if (ImGui::Selectable(label.c_str(), selected)) {
                app.view().selectedObject = -1;
                app.focusRoom(r.id);
            }
            ImGui::PopID();
        }
    }
    ImGui::TextDisabled("Tip: click on a room in the scene.");
    ImGui::Spacing();
}

// ---------------------------------------------------------------------------
void UIPanel::drawSelectionSection(Application& app) {
    Scene& scene = app.scene();
    ViewState& view = app.view();
    if (view.selectedRoom < 0 && view.selectedObject < 0) return;
    if (!ImGui::CollapsingHeader("Information", ImGuiTreeNodeFlags_DefaultOpen)) return;

    // --- Стая ---
    if (view.selectedRoom >= 0) {
        const Room& r = scene.rooms[view.selectedRoom];
        ImGui::TextColored(ImVec4(0.45f, 0.75f, 1.0f, 1.0f), "Room: %s", r.name.c_str());
        ImGui::Text("Floor: %d   Area: %.1f m²", r.floor, r.area());
        ImGui::TextWrapped("%s", r.description.c_str());
        int count = 0;
        for (const auto& o : scene.objects) if (o.roomId == r.id) ++count;
        ImGui::Text("Objects in room: %d", count);
        if (r.lightIndex >= 0) ImGui::Checkbox("Room lamp on", &scene.pointLights[r.lightIndex].enabled);
        ImGui::Separator();
    }

    // --- Обект ---
    if (view.selectedObject < 0) return;
    GameObject& o = scene.objects[view.selectedObject];
    ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "Object: %s", o.name.c_str());
    ImGui::Text("Type: %s", categoryName(o.category));
    if (o.roomId >= 0) ImGui::Text("Room: %s", scene.rooms[o.roomId].name.c_str());
    else ImGui::Text("Location: %s", o.floor == 0 ? "yard" : o.floor == 3 ? "roof" : "facade / balcony");
    glm::vec3 p = o.transform.worldCenter();
    ImGui::Text("Position: (%.2f, %.2f, %.2f) m", p.x, p.y, p.z);
    ImGui::Text("Size: %.2f × %.2f × %.2f m", o.transform.scale.x, o.transform.scale.y, o.transform.scale.z);
    ImGui::Text("Mesh: %s, %zu triangles", o.mesh->name().c_str(), o.mesh->triangleCount());

    if (o.isDoor) {
        if (ImGui::Button(o.doorTarget == 0.0f ? "Open door (O)" : "Close door (O)"))
            scene.toggleDoor(o);
    }
    if (o.lightIndex >= 0) ImGui::Checkbox("Lamp on", &scene.pointLights[o.lightIndex].enabled);

    // --- Материал ---
    // Обзавеждането (мебели, лампи) е с фиксиран материал – само се показва.
    if (!o.canEditMaterial()) {
        ImGui::Text("Material: %s", o.material.name.c_str());
        ImGui::TextDisabled("Furnishings keep their original material.");
    } else if (ImGui::TreeNodeEx("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
        Material& m = o.material;
        ImGui::Text("%s", m.name.c_str());
        ImGui::ColorEdit3("Colour", &m.color.x);

        const auto& textures = scene.textures.all();
        const char* current = m.texture ? m.texture->displayName().c_str() : "No texture";
        if (ImGui::BeginCombo("Texture", current)) {
            for (const auto& t : textures) {
                bool sel = m.texture == t.get();
                if (ImGui::Selectable(t->displayName().c_str(), sel)) m.texture = t.get();
                if (sel) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::SliderFloat("Tiling", &m.tiling, 0.1f, 8.0f, "%.2f");
        ImGui::SliderFloat("Specular", &m.specular, 0.0f, 1.5f);
        ImGui::SliderFloat("Shininess", &m.shininess, 2.0f, 256.0f, "%.0f", ImGuiSliderFlags_Logarithmic);
        if (ImGui::Button("Restore")) m = o.originalMaterial;
        ImGui::SameLine();
        if (ImGui::Button("Apply to whole room") && o.roomId >= 0) {
            // Копира материала върху всички обекти от същия тип в стаята (напр. всички стени).
            for (auto& other : scene.objects)
                if (other.roomId == o.roomId && other.category == o.category && other.floor == o.floor &&
                    other.canEditMaterial())
                    other.material = m;
        }
        ImGui::TreePop();
    }
    if (ImGui::Button("Hide object")) {
        o.visible = false;
        view.selectedObject = -1;
    }
    ImGui::SameLine();
    if (ImGui::Button("Show hidden"))
        for (auto& other : scene.objects) other.visible = true;
    ImGui::Spacing();
}

// ---------------------------------------------------------------------------
void UIPanel::drawHelpSection() {
    if (!ImGui::CollapsingHeader("Controls")) return;
    ImGui::BulletText("Left drag – orbit");
    ImGui::BulletText("Right drag – pan");
    ImGui::BulletText("Mouse wheel – zoom");
    ImGui::BulletText("Click – select object and room");
    ImGui::BulletText("1–6 – preset views");
    ImGui::BulletText("F1 / F2 / F3 – all floors / floor 1 / floor 2");
    ImGui::BulletText("N – day/night,  L – all lamps");
    ImGui::BulletText("O – front door,  P – wireframe");
    ImGui::BulletText("Tab – free camera (WASD, Q/E, Shift)");
    ImGui::BulletText("Esc – clear selection,  H – hide panel");
    ImGui::BulletText("F12 – screenshot");
}
