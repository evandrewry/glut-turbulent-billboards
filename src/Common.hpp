#pragma once
#include <vector>
#include <vmath.hpp>
#include <GL/glew.h>
#if defined(_WIN32)
#   include <GL/wglew.h>
#endif

struct Particle {
    float Px;  // Position X
    float Py;  // Position Y
    float Pz;  // Position Z
    float ToB; // Time of Birth
    float Vx;  // Velocity X
    float Vy;  // Velocity Y
    float Vz;  // Velocity Z
};

typedef std::vector<Particle> ParticleList;

enum AttributeSlot {
    POSITION_SLOT,
    TEX_COORD_SLOT,
    NORMAL_SLOT,
    BIRTH_TIME_SLOT,
    VELOCITY_SLOT,
};

struct ITrackball {
    virtual void MouseDown(int x, int y) = 0;
    virtual void MouseUp(int x, int y) = 0;
    virtual void MouseMove(int x, int y) = 0;
    virtual void ReturnHome() = 0;
    virtual vmath::Matrix3 GetRotation() const = 0;
    virtual void Update(unsigned int microseconds) = 0;
};

struct Mesh {
    GLuint IndexBuffer;
    GLuint PositionsBuffer;
    GLuint NormalsBuffer;
    GLuint TexCoordsBuffer;
    GLsizei IndexCount;
    GLsizei VertexCount;
};

struct Texture {
    GLuint Handle;
    GLsizei Width;
    GLsizei Height;
};

struct Surface {
    GLuint Fbo;
    GLuint ColorTexture;
    GLuint DepthTexture;
};

// Trackball.cpp
ITrackball* CreateTrackball(float width, float height, float radius);

// Shader.cpp
GLuint loadProgram(const char* vsKey, const char* gsKey, const char* fsKey);
void setUniform(const char* name, int value);
void setUniform(const char* name, float value);
void setUniform(const char* name, float x, float y);
void setUniform(const char* name, vmath::Matrix4 value);
void setUniform(const char* name, vmath::Matrix3 value);
void setUniform(const char* name, vmath::Vector3 value);
void setUniform(const char* name, vmath::Vector4 value);

// Mesh.cpp
Mesh CreateQuad(float left, float top, float right, float bottom);
Mesh CreateQuad();
Mesh LoadMesh(const char* path);
void RenderMesh(Mesh mesh);

// Texture.cpp
Texture LoadTexture(const char* path);
Surface CreateSurface(int width, int height);

// Particles.cpp
void AdvanceTime(ParticleList& particleList, float dt, float timeStep);
Texture VisualizePotential(GLsizei texWidth, GLsizei texHeight);
