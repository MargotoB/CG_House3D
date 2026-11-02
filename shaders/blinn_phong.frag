#version 330 core
// Фрагментен шейдър: модел на осветление на Blinn–Phong с
//   – една насочена светлина (слънце / луна),
//   – до MAX_LIGHTS точкови светлини (лампи) със затихване,
//   – полусферична околна (ambient) светлина,
//   – собствено светене (emissive) и гама корекция.

#define MAX_LIGHTS 16

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;

out vec4 FragColor;

struct PointLight {
    vec3  position;
    vec3  color;
    float intensity;
    float radius;
    vec3  zoneMin;
    vec3  zoneMax;
    int   invertZone;
};

// Материал
uniform vec3      uBaseColor;
uniform sampler2D uDiffuseMap;
uniform int       uHasTexture;
uniform float     uSpecular;
uniform float     uShininess;
uniform vec3      uEmissive;
uniform float     uOpacity;

// Камера и светлини
uniform vec3  uViewPos;
uniform vec3  uSunDirection;   // посока КЪМ слънцето
uniform vec3  uSunColor;
uniform float uSunIntensity;
uniform vec3  uAmbientSky;
uniform vec3  uAmbientGround;
uniform int   uNumLights;
uniform PointLight uLights[MAX_LIGHTS];

// Подсветка на избрания обект / стая
uniform vec3  uFogColor;       // цвят на хоризонта – далечните обекти се сливат с небето
uniform float uFogDensity;

uniform vec3  uHighlightColor;
uniform float uHighlightAmount;

const float GAMMA = 2.2;

vec3 toLinear(vec3 c) { return pow(c, vec3(GAMMA)); }

// Компонентите на Blinn–Phong за една светлина с посока L.
vec3 blinnPhong(vec3 N, vec3 V, vec3 L, vec3 albedo, vec3 lightColor) {
    float diff = max(dot(N, L), 0.0);
    vec3  H = normalize(L + V);                       // полувектор
    float spec = pow(max(dot(N, H), 0.0), uShininess);
    // Нормализиращ множител – по-лъскавите повърхности дават по-ярък, но по-малък отблясък.
    float norm = (uShininess + 8.0) / 25.0;
    return lightColor * (albedo * diff + uSpecular * spec * norm * step(0.0, diff));
}

bool insideZone(vec3 p, vec3 mn, vec3 mx) {
    return all(greaterThanEqual(p, mn)) && all(lessThanEqual(p, mx));
}

void main() {
    vec3 N = normalize(vNormal);
    if (!gl_FrontFacing) N = -N;                       // двустранни повърхности
    vec3 V = normalize(uViewPos - vWorldPos);

    vec3 albedo = toLinear(uBaseColor);
    if (uHasTexture == 1)
        albedo *= toLinear(texture(uDiffuseMap, vUV).rgb);

    // 1) Околна светлина – смесване между "небе" и "земя" според нормалата.
    vec3 ambient = mix(uAmbientGround, uAmbientSky, N.y * 0.5 + 0.5) * albedo;

    // 2) Насочена светлина (слънце/луна).
    vec3 color = ambient + blinnPhong(N, V, normalize(uSunDirection), albedo, uSunColor * uSunIntensity);

    // 3) Точкови светлини. Всяка свети само в своята зона (стаята си),
    //    понеже сцената няма сенки и иначе светлината би минавала през стените.
    for (int i = 0; i < uNumLights; ++i) {
        bool inside = insideZone(vWorldPos, uLights[i].zoneMin, uLights[i].zoneMax);
        if (inside == (uLights[i].invertZone == 1)) continue;

        vec3  toLight = uLights[i].position - vWorldPos;
        float d = length(toLight);
        if (d > uLights[i].radius) continue;
        vec3 L = toLight / d;

        // Физично мотивирано затихване 1/(1 + k1·d + k2·d²), плавно занулено до радиуса.
        float atten = 1.0 / (1.0 + 0.35 * d + 0.25 * d * d);
        float fade = clamp(1.0 - pow(d / uLights[i].radius, 4.0), 0.0, 1.0);
        color += blinnPhong(N, V, L, albedo, uLights[i].color * uLights[i].intensity) * atten * fade * 2.0;
    }

    color += toLinear(uEmissive);

    // Тонално преобразуване (Reinhard) и гама корекция към sRGB.
    color = color / (1.0 + color * 0.25);
    color = pow(color, vec3(1.0 / GAMMA));

    // Експоненциална мъгла по разстоянието до камерата.
    float dist = length(uViewPos - vWorldPos);
    float fog = 1.0 - exp(-pow(dist * uFogDensity, 2.0));
    color = mix(color, uFogColor, clamp(fog, 0.0, 1.0));

    color = mix(color, uHighlightColor, uHighlightAmount);
    FragColor = vec4(color, uOpacity);
}
