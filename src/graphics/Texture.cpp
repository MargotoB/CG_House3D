#include "graphics/Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_WINDOWS_UTF8
#include <stb_image.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>

// ============================================================================
// Помощни функции за процедурен шум (value noise + fBm).
// Шумът е периодичен, за да могат текстурите да се повтарят без шевове.
// ============================================================================
namespace {

float hash2(int x, int y, int seed = 0) {
    unsigned h = static_cast<unsigned>(x) * 374761393u + static_cast<unsigned>(y) * 668265263u
               + static_cast<unsigned>(seed) * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (h & 0xFFFFFF) / static_cast<float>(0xFFFFFF);
}

float smooth(float t) { return t * t * (3.0f - 2.0f * t); }

// Периодичен value noise; periodX/periodY – брой клетки по всяка ос.
float valueNoise(float x, float y, int periodX, int periodY, int seed) {
    int xi = static_cast<int>(std::floor(x)), yi = static_cast<int>(std::floor(y));
    float fx = x - xi, fy = y - yi;
    auto wx = [periodX](int a) { return ((a % periodX) + periodX) % periodX; };
    auto wy = [periodY](int a) { return ((a % periodY) + periodY) % periodY; };
    float a = hash2(wx(xi), wy(yi), seed),     b = hash2(wx(xi + 1), wy(yi), seed);
    float c = hash2(wx(xi), wy(yi + 1), seed), d = hash2(wx(xi + 1), wy(yi + 1), seed);
    float u = smooth(fx), v = smooth(fy);
    return a + (b - a) * u + (c - a) * v + (a - b - c + d) * u * v;
}

float valueNoise(float x, float y, int period, int seed) { return valueNoise(x, y, period, period, seed); }

// Фрактален шум (сума от няколко октави).
float fbm(float u, float v, int basePeriod, int octaves, int seed) {
    float sum = 0, amp = 0.5f, norm = 0;
    int period = basePeriod;
    for (int i = 0; i < octaves; ++i) {
        sum += amp * valueNoise(u * period, v * period, period, seed + i * 17);
        norm += amp;
        amp *= 0.5f;
        period *= 2;
    }
    return sum / norm;
}

float fract(float x) { return x - std::floor(x); }

void put(std::vector<unsigned char>& px, int size, int x, int y, float r, float g, float b) {
    size_t i = (static_cast<size_t>(y) * size + x) * 4;
    px[i + 0] = static_cast<unsigned char>(std::clamp(r, 0.0f, 1.0f) * 255.0f);
    px[i + 1] = static_cast<unsigned char>(std::clamp(g, 0.0f, 1.0f) * 255.0f);
    px[i + 2] = static_cast<unsigned char>(std::clamp(b, 0.0f, 1.0f) * 255.0f);
    px[i + 3] = 255;
}

// ============================================================================
// Генератори. Една повторяема плочка от текстурата отговаря на 1 m × 1 m.
// ============================================================================

void genWhite(std::vector<unsigned char>& px, int s) {
    for (int y = 0; y < s; ++y)
        for (int x = 0; x < s; ++x) put(px, s, x, y, 1, 1, 1);
}

void genBrick(std::vector<unsigned char>& px, int s) {
    const int rows = 8, cols = 4;
    for (int y = 0; y < s; ++y) {
        for (int x = 0; x < s; ++x) {
            float u = (x + 0.5f) / s, v = (y + 0.5f) / s;
            int row = static_cast<int>(v * rows);
            float shifted = u * cols + (row % 2) * 0.5f;
            int col = static_cast<int>(std::floor(shifted)) % cols;
            float fu = fract(shifted), fv = fract(v * rows);
            bool mortar = fu < 0.04f || fv < 0.08f;
            float n = fbm(u, v, 16, 4, 3);
            if (mortar) {
                float m = 0.72f + 0.1f * n;
                put(px, s, x, y, m, m * 0.98f, m * 0.94f);
            } else {
                float k = 0.78f + 0.35f * hash2(col, row, 7);
                float grain = 0.85f + 0.3f * n;
                put(px, s, x, y, 0.66f * k * grain, 0.30f * k * grain, 0.21f * k * grain);
            }
        }
    }
}

void genPlaster(std::vector<unsigned char>& px, int s) {
    for (int y = 0; y < s; ++y)
        for (int x = 0; x < s; ++x) {
            float u = (x + 0.5f) / s, v = (y + 0.5f) / s;
            float n = 0.93f + 0.07f * fbm(u, v, 8, 5, 11);
            put(px, s, x, y, n, n, n * 0.98f);
        }
}

void genWoodFloor(std::vector<unsigned char>& px, int s) {
    const int planks = 5;
    for (int y = 0; y < s; ++y)
        for (int x = 0; x < s; ++x) {
            float u = (x + 0.5f) / s, v = (y + 0.5f) / s;
            int plank = static_cast<int>(u * planks);
            float pv = fract(v + hash2(plank, 0, 21));
            bool seam = fract(u * planks) < 0.03f || pv < 0.01f;
            float grain = fbm(u, v, 4, 4, 5 + plank) * 0.5f + 0.5f * valueNoise(u * 64, v * 4, 64, 4, 9);
            float k = 0.8f + 0.3f * hash2(plank, 3, 4);
            float g = 0.88f + 0.16f * std::sin(grain * 18.0f);
            if (seam) put(px, s, x, y, 0.35f, 0.24f, 0.15f);
            else put(px, s, x, y, 0.78f * k * g, 0.58f * k * g, 0.38f * k * g);
        }
}

void genTiles(std::vector<unsigned char>& px, int s) {
    const int n = 4;
    for (int y = 0; y < s; ++y)
        for (int x = 0; x < s; ++x) {
            float u = (x + 0.5f) / s, v = (y + 0.5f) / s;
            bool grout = fract(u * n) < 0.03f || fract(v * n) < 0.03f;
            int tx = static_cast<int>(u * n), ty = static_cast<int>(v * n);
            float k = 0.95f + 0.05f * hash2(tx, ty, 2) + 0.03f * fbm(u, v, 8, 3, 8);
            if (grout) put(px, s, x, y, 0.55f, 0.55f, 0.55f);
            else put(px, s, x, y, 0.90f * k, 0.92f * k, 0.94f * k);
        }
}

void genRoofTiles(std::vector<unsigned char>& px, int s) {
    const int rows = 5, cols = 5;
    for (int y = 0; y < s; ++y)
        for (int x = 0; x < s; ++x) {
            float u = (x + 0.5f) / s, v = (y + 0.5f) / s;
            int row = static_cast<int>(v * rows);
            float shifted = u * cols + (row % 2) * 0.5f;
            int col = static_cast<int>(std::floor(shifted)) % cols;
            float fu = fract(shifted), fv = fract(v * rows);
            // Всяка керемида е заоблена напречно и по-тъмна към долния си ръб.
            float curve = 0.75f + 0.25f * std::sin(fu * 3.14159f);
            float shade = 0.65f + 0.35f * fv;
            float k = (0.85f + 0.25f * hash2(col, row, 13)) * curve * shade * (0.9f + 0.1f * fbm(u, v, 16, 3, 1));
            if (fu < 0.03f) k *= 0.5f;
            put(px, s, x, y, 0.70f * k, 0.30f * k, 0.18f * k);
        }
}

void genGrass(std::vector<unsigned char>& px, int s) {
    for (int y = 0; y < s; ++y)
        for (int x = 0; x < s; ++x) {
            float u = (x + 0.5f) / s, v = (y + 0.5f) / s;
            float big = fbm(u, v, 4, 4, 31);
            float fine = valueNoise(u * 128, v * 128, 128, 77);
            float k = 0.7f + 0.4f * big;
            float f = 0.8f + 0.35f * fine;
            put(px, s, x, y, 0.30f * k * f, 0.50f * k * f, 0.19f * k * f);
        }
}

void genStone(std::vector<unsigned char>& px, int s) {
    const int n = 3;
    for (int y = 0; y < s; ++y)
        for (int x = 0; x < s; ++x) {
            float u = (x + 0.5f) / s, v = (y + 0.5f) / s;
            int row = static_cast<int>(v * n);
            float shifted = u * n + (row % 2) * 0.5f;
            int col = static_cast<int>(std::floor(shifted)) % n;
            float fu = fract(shifted), fv = fract(v * n);
            bool gap = fu < 0.04f || fv < 0.04f;
            float k = 0.75f + 0.3f * hash2(col, row, 19);
            float t = 0.85f + 0.2f * fbm(u, v, 8, 4, 41);
            if (gap) put(px, s, x, y, 0.25f, 0.24f, 0.22f);
            else put(px, s, x, y, 0.62f * k * t, 0.60f * k * t, 0.56f * k * t);
        }
}

void genWood(std::vector<unsigned char>& px, int s) {
    for (int y = 0; y < s; ++y)
        for (int x = 0; x < s; ++x) {
            float u = (x + 0.5f) / s, v = (y + 0.5f) / s;
            float n = fbm(u, v, 4, 4, 55);
            float rings = 0.5f + 0.5f * std::sin((u * 12.0f + n * 3.0f) * 6.2832f);
            float k = 0.75f + 0.25f * rings;
            put(px, s, x, y, 0.60f * k, 0.40f * k, 0.22f * k);
        }
}

void genFabric(std::vector<unsigned char>& px, int s) {
    for (int y = 0; y < s; ++y)
        for (int x = 0; x < s; ++x) {
            float u = (x + 0.5f) / s, v = (y + 0.5f) / s;
            float weave = 0.5f + 0.25f * std::sin(u * 6.2832f * 64) + 0.25f * std::sin(v * 6.2832f * 64);
            float k = 0.8f + 0.15f * weave + 0.08f * fbm(u, v, 8, 3, 63);
            put(px, s, x, y, k, k, k);
        }
}

// Мрамор: бял камък с меки сиви жилки, на плочи 50×50 cm.
void genMarble(std::vector<unsigned char>& px, int s) {
    for (int y = 0; y < s; ++y)
        for (int x = 0; x < s; ++x) {
            float u = (x + 0.5f) / s, v = (y + 0.5f) / s;
            float n = fbm(u, v, 3, 6, 101);
            float t = (u + v) * 2.0f + n * 3.0f;                       // периодично по u и v
            float vein = 1.0f - std::min(1.0f, std::fabs(std::sin(t * 3.14159f)) * 4.0f);
            float fine = 1.0f - std::min(1.0f, std::fabs(std::sin((u * 4.0f + n * 6.0f) * 3.14159f)) * 12.0f);
            float k = 0.97f - 0.16f * vein * vein - 0.06f * fine - 0.04f * fbm(u, v, 16, 3, 7);
            bool grout = fract(u * 2) < 0.004f || fract(v * 2) < 0.004f;
            if (grout) k *= 0.8f;
            put(px, s, x, y, 0.95f * k, 0.94f * k, 0.92f * k);
        }
}

// Травертин: топъл бежов камък на плочи 50×33 cm с хоризонтални пори.
void genTravertine(std::vector<unsigned char>& px, int s) {
    const int rows = 3, cols = 2;
    for (int y = 0; y < s; ++y)
        for (int x = 0; x < s; ++x) {
            float u = (x + 0.5f) / s, v = (y + 0.5f) / s;
            int row = static_cast<int>(v * rows);
            float shifted = u * cols + (row % 2) * 0.5f;
            int col = static_cast<int>(std::floor(shifted)) % cols;
            bool joint = fract(shifted) < 0.008f || fract(v * rows) < 0.012f;
            float pores = valueNoise(u * 12, v * 96, 12, 96, 55);
            float k = (0.92f + 0.08f * hash2(col, row, 3)) * (0.93f + 0.07f * fbm(u, v, 4, 4, 9));
            if (pores > 0.82f) k *= 0.86f;
            if (joint) k *= 0.82f;
            put(px, s, x, y, 0.90f * k, 0.84f * k, 0.74f * k);
        }
}

// Дървена обшивка: вертикални дъски по 12.5 cm с тъмни фуги.
void genCladding(std::vector<unsigned char>& px, int s) {
    const int boards = 8;
    for (int y = 0; y < s; ++y)
        for (int x = 0; x < s; ++x) {
            float u = (x + 0.5f) / s, v = (y + 0.5f) / s;
            int b = static_cast<int>(u * boards);
            bool gap = fract(u * boards) < 0.07f;
            float grain = valueNoise(u * 128, v * 6, 128, 6, 31 + b);
            float k = (0.82f + 0.3f * hash2(b, 1, 77)) * (0.85f + 0.25f * grain);
            if (gap) put(px, s, x, y, 0.08f, 0.06f, 0.05f);
            else put(px, s, x, y, 0.55f * k, 0.36f * k, 0.22f * k);
        }
}

// Вода: тюркоазено синьо със "каустики" – светли вълнообразни линии.
void genWater(std::vector<unsigned char>& px, int s) {
    for (int y = 0; y < s; ++y)
        for (int x = 0; x < s; ++x) {
            float u = (x + 0.5f) / s, v = (y + 0.5f) / s;
            float n1 = fbm(u, v, 4, 4, 91), n2 = fbm(u, v, 4, 4, 133);
            float c1 = std::pow(1.0f - std::fabs(std::sin(n1 * 3.14159f * 7.0f)), 8.0f);
            float c2 = std::pow(1.0f - std::fabs(std::sin(n2 * 3.14159f * 5.0f)), 8.0f);
            float c = std::min(1.0f, c1 + c2 * 0.7f);
            put(px, s, x, y, 0.10f + 0.55f * c, 0.50f + 0.40f * c, 0.68f + 0.30f * c);
        }
}

} // namespace

// ============================================================================
// TextureLibrary
// ============================================================================

static GLuint uploadRGBA(const unsigned char* data, int w, int h) {
    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (GLAD_GL_EXT_texture_filter_anisotropic) {
        GLfloat maxAniso = 1.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, std::min(8.0f, maxAniso));
    }
    return id;
}

void TextureLibrary::add(const std::string& key, const std::string& name, int size,
                         void (*generator)(std::vector<unsigned char>&, int)) {
    // 1) Опит за зареждане на файл assets/textures/<key>.png или .jpg
    for (const char* ext : {".png", ".jpg"}) {
        std::string path = m_resourceDir + "/assets/textures/" + key + ext;
        std::error_code ec;
        if (!std::filesystem::exists(std::filesystem::u8path(path), ec)) continue;
        int w, h, c;
        stbi_set_flip_vertically_on_load(1);
        unsigned char* data = stbi_load(path.c_str(), &w, &h, &c, 4);
        if (data) {
            m_textures.push_back(std::make_unique<Texture>(key, name, uploadRGBA(data, w, h)));
            stbi_image_free(data);
            std::cout << "[Texture] Loaded file " << path << std::endl;
            return;
        }
    }
    // 2) Процедурно генериране
    std::vector<unsigned char> pixels(static_cast<size_t>(size) * size * 4);
    generator(pixels, size);
    m_textures.push_back(std::make_unique<Texture>(key, name, uploadRGBA(pixels.data(), size, size)));
}

void TextureLibrary::createAll(const std::string& resourceDir) {
    m_resourceDir = resourceDir;
    add("none",    "No texture",     4,   genWhite);
    add("brick",   "Bricks",            512, genBrick);
    add("plaster", "Plaster",          256, genPlaster);
    add("parquet", "Parquet",           512, genWoodFloor);
    add("tiles",   "Tiles",           256, genTiles);
    add("roof",    "Roof tiles",         512, genRoofTiles);
    add("grass",   "Grass",            512, genGrass);
    add("stone",   "Stone paving", 256, genStone);
    add("wood",    "Wood",            256, genWood);
    add("fabric",  "Fabric",             256, genFabric);
    add("marble",  "Marble",           512, genMarble);
    add("travertine", "Travertine",     512, genTravertine);
    add("cladding", "Wood cladding", 512, genCladding);
    add("water",   "Water",             256, genWater);
}

Texture* TextureLibrary::get(const std::string& key) const {
    for (const auto& t : m_textures)
        if (t->key() == key) return t.get();
    return m_textures.empty() ? nullptr : m_textures.front().get();
}
