#include "graphics/PickingBuffer.h"

#include <iostream>

PickingBuffer::~PickingBuffer() { release(); }

void PickingBuffer::release() {
    if (m_depth) glDeleteRenderbuffers(1, &m_depth);
    if (m_color) glDeleteTextures(1, &m_color);
    if (m_fbo) glDeleteFramebuffers(1, &m_fbo);
    m_fbo = m_color = m_depth = 0;
}

void PickingBuffer::resize(int width, int height) {
    if (width == m_width && height == m_height && m_fbo) return;
    release();
    m_width = width;
    m_height = height;

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    // Цветово приложение: 8 бита на канал – без филтриране, за да не се смесват id-тата.
    glGenTextures(1, &m_color);
    glBindTexture(GL_TEXTURE_2D, m_color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_color, 0);

    // Буфер за дълбочина – нужен е, за да "печели" най-близкият обект.
    glGenRenderbuffers(1, &m_depth);
    glBindRenderbuffer(GL_RENDERBUFFER, m_depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depth);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "[PickingBuffer] Framebuffer is incomplete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PickingBuffer::bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_width, m_height);
}

void PickingBuffer::unbind() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

unsigned PickingBuffer::readId(int x, int y) const {
    if (x < 0 || y < 0 || x >= m_width || y >= m_height) return 0;
    unsigned char px[4] = {0, 0, 0, 0};
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, px);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    return px[0] | (px[1] << 8) | (px[2] << 16);
}
