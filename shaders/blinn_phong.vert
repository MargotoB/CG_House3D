#version 330 core
// Вертекс шейдър: преобразува върховете с моделната, изгледната и проекционната
// матрица и подготвя данните за осветлението във фрагментния шейдър.

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;   // transpose(inverse(mat3(uModel)))

uniform int   uUVMode;        // 0 – UV от геометрията, 1 – кутийна проекция в метри
uniform vec3  uObjectScale;   // мащаб на обекта (за кутийната проекция)
uniform float uTiling;
uniform vec2  uUVOffset;     // отместване във времето (анимирана текстура)

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;

void main() {
    vec4 world = uModel * vec4(aPos, 1.0);
    vWorldPos = world.xyz;
    vNormal = normalize(uNormalMatrix * aNormal);

    if (uUVMode == 1) {
        // Кутийна проекция: избираме двете оси, перпендикулярни на нормалата,
        // и използваме локалната позиция, умножена по мащаба (т.е. в метри).
        // Така текстурата на една стена не се разтяга, колкото и дълга да е тя.
        vec3 p = aPos * uObjectScale;
        vec3 n = abs(aNormal);
        vec2 uv;
        if (n.x > n.y && n.x > n.z)      uv = p.zy;
        else if (n.y > n.z)              uv = p.xz;
        else                             uv = p.xy;
        vUV = uv * uTiling + uUVOffset;
    } else {
        vUV = aUV * uTiling + uUVOffset;
    }

    gl_Position = uProjection * uView * world;
}
