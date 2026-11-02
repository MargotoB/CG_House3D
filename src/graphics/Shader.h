#pragma once
// Shader – зарежда, компилира и свързва GLSL програма (vertex + fragment)
// и предоставя удобни методи за задаване на uniform променливи.

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

class Shader {
public:
    Shader() = default;
    ~Shader();
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    // Зарежда двата файла и създава програмата. Връща false при грешка.
    bool load(const std::string& vertexPath, const std::string& fragmentPath);

    void use() const { glUseProgram(m_program); }
    GLuint id() const { return m_program; }

    void setInt(const std::string& name, int v);
    void setFloat(const std::string& name, float v);
    void setVec2(const std::string& name, const glm::vec2& v);
    void setVec3(const std::string& name, const glm::vec3& v);
    void setMat3(const std::string& name, const glm::mat3& m);
    void setMat4(const std::string& name, const glm::mat4& m);

private:
    GLint location(const std::string& name);
    static GLuint compile(GLenum type, const std::string& source, const std::string& path);

    GLuint m_program = 0;
    std::unordered_map<std::string, GLint> m_cache; // кеш на uniform локациите
};
