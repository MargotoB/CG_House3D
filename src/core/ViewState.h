#pragma once
// ViewState – текущото състояние на изгледа, споделено между Renderer, UI и InputController.

enum class FloorView { All = 0, Floor1 = 1, Floor2 = 2 };

struct ViewState {
    FloorView floorView = FloorView::All;

    int selectedObject = -1;   // индекс в Scene::objects
    int selectedRoom = -1;     // индекс в Scene::rooms

    bool night = false;        // целево състояние
    float nightFactor = 0.0f;  // 0 – ден, 1 – нощ (плавен преход)

    bool showUI = true;
    bool wireframe = false;
};
