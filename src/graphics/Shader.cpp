#include "graphics/Shader.h"

#include <glm/gtc/type_ptr.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

static bool readFile(const std::string& path, std::string& out) {
    std::ifstream file(std::filesystem::u8path(path), std::ios::binary);
    if (!file) return false;
    std::stringstream ss;
    ss << file.rdbuf();
    out = ss.str();
    return true;
}

Shader::~Shader() {
    if (m_program) glDeleteProgram(m_program);
}

GLuint Shader::compile(GLenum type, const std::string& source, const std::string& path) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::cerr << "[Shader] Compile error in " << path << ":\n" << log << std::endl;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool Shader::load(const std::string& vertexPath, const std::string& fragmentPath) {
    std::string vsSrc, fsSrc;
    if (!readFile(vertexPath, vsSrc)) {
        std::cerr << "[Shader] Cannot open " << vertexPath << std::endl;
        return false;
    }
    if (!readFile(fragmentPath, fsSrc)) {
        std::cerr << "[Shader] Cannot open " << fragmentPath << std::endl;
        return false;
    }

    GLuint vs = compile(GL_VERTEX_SHADER, vsSrc, vertexPath);
    GLuint fs = compile(GL_FRAGMENT_SHADER, fsSrc, fragmentPath);
    if (!vs || !fs) return false;

    m_program = glCreateProgram();
    glAttachShader(m_program, vs);
    glAttachShader(m_program, fs);
    glLinkProgram(m_program);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok = 0;
    glGetProgramiv(m_program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetProgramInfoLog(m_program, sizeof(log), nullptr, log);
        std::cerr << "[Shader] Link error:\n" << log << std::endl;
        return false;
    }
    return true;
}

GLint Shader::location(const std::string& name) {
    auto it = m_cache.find(name);
    if (it != m_cache.end()) return it->second;
    GLint loc = glGetUniformLocation(m_program, name.c_str());
    m_cache[name] = loc;
    return loc;
}

void Shader::setInt(const std::string& n, int v)               { glUniform1i(location(n), v); }
void Shader::setFloat(const std::string& n, float v)           { glUniform1f(location(n), v); }
void Shader::setVec2(const std::string& n, const glm::vec2& v) { glUniform2fv(location(n), 1, glm::value_ptr(v)); }
void Shader::setVec3(const std::string& n, const glm::vec3& v) { glUniform3fv(location(n), 1, glm::value_ptr(v)); }
void Shader::setMat3(const std::string& n, const glm::mat3& m) { glUniformMatrix3fv(location(n), 1, GL_FALSE, glm::value_ptr(m)); }
void Shader::setMat4(const std::string& n, const glm::mat4& m) { glUniformMatrix4fv(location(n), 1, GL_FALSE, glm::value_ptr(m)); }
