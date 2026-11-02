#pragma once
// PickingBuffer – невидим framebuffer (FBO), в който всеки обект се рисува
// с уникален цвят. Цветът на пиксела под курсора показва кой обект е избран.

#include <glad/gl.h>

class PickingBuffer {
public:
    ~PickingBuffer();
    void resize(int width, int height);
    void bind() const;
    static void unbind();

    // Връща кодирания идентификатор в пиксела (x, y) (0 = нищо).
    unsigned readId(int x, int y) const;

    int width() const { return m_width; }
    int height() const { return m_height; }

private:
    void release();
    GLuint m_fbo = 0, m_color = 0, m_depth = 0;
    int m_width = 0, m_height = 0;
};
