#pragma once
// Texture – 2D текстура в OpenGL. Текстурите се генерират процедурно
// (тухли, паркет, керемиди и т.н.), а ако в assets/textures/ има PNG/JPG
// файл със същото име, вместо това се зарежда той.

#include <glad/gl.h>
#include <memory>
#include <string>
#include <vector>

class Texture {
public:
    Texture(std::string key, std::string displayName, GLuint id)
        : m_key(std::move(key)), m_displayName(std::move(displayName)), m_id(id) {}
    ~Texture() { glDeleteTextures(1, &m_id); }
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    void bind(int unit) const {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, m_id);
    }

    const std::string& key() const { return m_key; }
    const std::string& displayName() const { return m_displayName; }
    GLuint id() const { return m_id; }

private:
    std::string m_key, m_displayName;
    GLuint m_id;
};

// Библиотека със всички текстури в сцената.
class TextureLibrary {
public:
    // Създава всички текстури. resourceDir е папката, в която се търси assets/textures/.
    void createAll(const std::string& resourceDir);

    Texture* get(const std::string& key) const;
    const std::vector<std::unique_ptr<Texture>>& all() const { return m_textures; }

private:
    void add(const std::string& key, const std::string& name, int size,
             void (*generator)(std::vector<unsigned char>&, int));

    std::string m_resourceDir;
    std::vector<std::unique_ptr<Texture>> m_textures;
};
