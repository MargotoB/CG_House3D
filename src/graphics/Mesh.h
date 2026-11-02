#pragma once
// Mesh – геометрия в паметта на видеокартата (VAO + VBO + EBO).
// Всеки връх съдържа позиция, нормала и текстурни координати.

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

class Mesh {
public:
    Mesh(std::string name, const std::vector<Vertex>& vertices, const std::vector<unsigned>& indices);
    ~Mesh();
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    void draw() const;

    const std::string& name() const { return m_name; }
    size_t triangleCount() const { return m_indexCount / 3; }

private:
    std::string m_name;
    GLuint m_vao = 0, m_vbo = 0, m_ebo = 0;
    GLsizei m_indexCount = 0;
};
