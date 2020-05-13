/*
  MIT License

  Copyright (c) 2019 rbellek

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

// OpenGL Off-Screen Renderer Header-Only Library v1.0

#ifndef OFFSCREENRENDERER_H
#define OFFSCREENRENDERER_H

#include <GL/glew.h>
#include <GL/GL.h>


#include <functional>
#include <iostream>
#include <fstream>

namespace offscreenrenderer
{
  class OffScreenRenderer
  {
  public:
    OffScreenRenderer()
    {
      m_outputDir = "imgout/";
      m_width = 800;
      m_height = 600;
      m_renderFunction = nullptr;
    }

    ~OffScreenRenderer()
    {
      if (!m_init)
        return;

      glDeleteFramebuffers(1, &m_FBO);
      glDeleteTextures(1, &m_textureID);
      glDeleteRenderbuffers(1, &m_depthBuffer);
    }

    void DumpImage()
    {
      if (!m_init || !m_renderFunction)
        return;

      if (m_rendered)
        return;

      m_rendered = true;

      glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      m_renderFunction();

      glBindFramebuffer(GL_FRAMEBUFFER, 0);

      glBindTexture(GL_TEXTURE_2D, m_textureID);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

      if (GLubyte * pixels = new GLubyte[m_width * m_height * 4])
      {
        glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, pixels);
        char numbering[32];
        std::snprintf(numbering, sizeof(numbering), "%i", m_counter++);
        SaveBitmap(pixels, m_width, m_height, 32, (m_outputDir + "image" + numbering + ".bmp").c_str());

        glBindTexture(GL_TEXTURE_2D, 0);

        delete[] pixels;
      }
      m_rendered = false;
    }

    void Init()
    {
      if (m_init)
        return;

      glewInit();

      glGenTextures(1, &m_textureID);
      glBindTexture(GL_TEXTURE_2D, m_textureID);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
      glBindTexture(GL_TEXTURE_2D, 0);

      glGenRenderbuffers(1, &m_depthBuffer);
      glBindRenderbuffer(GL_RENDERBUFFER, m_depthBuffer);
      glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, m_width, m_height);

      glGenFramebuffers(1, &m_FBO);
      glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_textureID, 0);
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthBuffer);
      glBindFramebuffer(GL_FRAMEBUFFER, 0);

      m_init = true;
    }

    std::string m_outputDir;
    unsigned int m_width;
    unsigned int m_height;
    std::function<void(void)> m_renderFunction;

  private:

#pragma pack(push, 1)
    struct _BITMAPFILEHEADER
    {
      unsigned short bfType;
      unsigned long bfSize;
      unsigned short bfReserved1;
      unsigned short bfReserved2;
      unsigned long bfOffBits;
    };
    struct _BITMAPINFOHEADER
    {
      unsigned long biSize;
      long biWidth;
      long biHeight;
      unsigned short biPlanes;
      unsigned short biBitCount;
      unsigned long biCompression;
      unsigned long biSizeImage;
      long biXPelsPerMeter;
      long biYPelsPerMeter;
      unsigned long biClrUsed;
      unsigned long biClrImportant;
    };
#pragma pack(pop)

    void SaveBitmap(unsigned char* pBitmapBits, const int width, const int height, const int bitsPerPixel, const char* lpszFileName)
    {
      constexpr unsigned long headersSize = sizeof(_BITMAPFILEHEADER) + sizeof(_BITMAPINFOHEADER);
      static_assert(headersSize == 54, "Invalid BMP header struct size!");

      _BITMAPINFOHEADER bmpInfoHeader = { 0 };

      bmpInfoHeader.biSize = sizeof(_BITMAPINFOHEADER);
      bmpInfoHeader.biBitCount = bitsPerPixel;
      bmpInfoHeader.biClrImportant = 0;
      bmpInfoHeader.biClrUsed = 0;
      bmpInfoHeader.biCompression = 0;// BI_RGB;
      bmpInfoHeader.biHeight = height;
      bmpInfoHeader.biWidth = width;
      bmpInfoHeader.biPlanes = 1;
      const unsigned long pixelSize = bmpInfoHeader.biHeight * ((bmpInfoHeader.biWidth * (bmpInfoHeader.biBitCount / 8)));
      bmpInfoHeader.biSizeImage = pixelSize;

      _BITMAPFILEHEADER bfh = { 0 };
      bfh.bfType = 0x4D42;
      bfh.bfOffBits = headersSize;
      bfh.bfSize = headersSize + pixelSize;

      std::ofstream file(lpszFileName, std::ios::binary);

      auto xx_write = [&file](const auto &s, unsigned int size = 0)
      {
        file.write(reinterpret_cast<const char*>(&s), size == 0 ? sizeof(s) : size);
      };

      xx_write(bfh);
      xx_write(bmpInfoHeader);
      xx_write(*pBitmapBits, bmpInfoHeader.biSizeImage);
      
      file.close();
    }

    GLuint m_textureID;
    GLuint m_depthBuffer;
    GLuint m_FBO;
    bool m_rendered = false;
    bool m_init = false;
    unsigned int m_counter = 0;
  };
}

#endif // OFFSCREENRENDERER_H
