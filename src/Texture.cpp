#include <string>
#include <pnglite.h>
#include "Common.hpp"

using std::string;

Texture LoadTexture(const char* path)
{
    static bool first = true;
    if (first) {
        png_init(0, 0);
        first = false;
    }

    string fullpath;
    if (path[1] == ':' || path[0] == '/') {
        fullpath = path;
    } else {
        fullpath = string("./assets");
        char trailingChar = fullpath[fullpath.size() - 1];
        if (trailingChar != '/' && trailingChar != '\\')
        {
            fullpath = fullpath + '/';
        }
        fullpath = fullpath + path;
    }

    png_t tex;

    if (PNG_NO_ERROR != png_open_file_read(&tex, fullpath.c_str()))
        printf("Unable to load PNG file: %s", fullpath.c_str());

    unsigned char* data = (unsigned char*) malloc(tex.width * tex.height * tex.bpp);
    png_get_data(&tex, data);

    Texture pod;
    pod.Width = tex.width;
    pod.Height = tex.height;


    GLenum type = GL_UNSIGNED_BYTE;

    GLenum internalFormat, format;
    switch (tex.bpp)
    {
        case 1:
            internalFormat = GL_LUMINANCE;
            format = GL_LUMINANCE;
            break;
        case 2:
            internalFormat = GL_LUMINANCE_ALPHA;
            format = GL_LUMINANCE_ALPHA;
            break;
        case 3:
            internalFormat = GL_RGB;
            format = GL_RGB;
            break;
        case 4:
            internalFormat = GL_RGBA;
            format = GL_RGBA;
            break;
        default:
            break;
    }

    glGenTextures(1, &pod.Handle);
    glBindTexture(GL_TEXTURE_2D, pod.Handle);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, tex.width, tex.height, 0, format, type, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_2D);

    png_close_file(&tex);
    free(data);
    return pod;
}

Surface CreateSurface(int width, int height)
{
    Surface pod;

    // Create a depth texture:
    glGenTextures(1, &pod.DepthTexture);
    glBindTexture(GL_TEXTURE_2D, pod.DepthTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, 0);
    
    // Create a FBO and attach the depth texture:
    glGenFramebuffers(1, &pod.Fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, pod.Fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, pod.DepthTexture, 0);

    // Create a color texture:
    glGenTextures(1, &pod.ColorTexture);
    glBindTexture(GL_TEXTURE_2D, pod.ColorTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

    // Attach the color buffer:
    GLuint colorbuffer;
    glGenRenderbuffers(1, &colorbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, colorbuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pod.ColorTexture, 0);

    return pod;

}
