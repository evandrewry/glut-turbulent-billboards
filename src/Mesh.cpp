#include <string>
#include <openctm.h>
#include "Common.hpp"

using std::string;

Mesh CreateQuad(float left, float top, float right, float bottom)
{
    Mesh pod = {0};
    pod.VertexCount = 4;
    pod.IndexCount = 6;

    float positions[] = {
        left, top, 0,
        right, top, 0,
        right, bottom, 0,
        left, bottom, 0,
    };

    glGenBuffers(1, &pod.PositionsBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, pod.PositionsBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);

    float texcoords[] = {
        0, 0,
        1, 0,
        1, 1,
        0, 1,
    };

    glGenBuffers(1, &pod.TexCoordsBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, pod.TexCoordsBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(texcoords), texcoords, GL_STATIC_DRAW);

    int faces[] = { 3, 2, 1, 1, 0, 3 };

    glGenBuffers(1, &pod.IndexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pod.IndexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(faces), faces, GL_STATIC_DRAW);

    return pod;
}

Mesh CreateQuad()
{
    return CreateQuad(-1, +1, +1, -1);
}

void RenderMesh(Mesh mesh)
{
    glBindBuffer(GL_ARRAY_BUFFER, mesh.PositionsBuffer);
    glVertexAttribPointer(POSITION_SLOT, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
    glEnableVertexAttribArray(POSITION_SLOT);

    if (mesh.NormalsBuffer) {
        glBindBuffer(GL_ARRAY_BUFFER, mesh.NormalsBuffer);
        glVertexAttribPointer(NORMAL_SLOT, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
        glEnableVertexAttribArray(NORMAL_SLOT);
    }

    if (mesh.TexCoordsBuffer) {
        glBindBuffer(GL_ARRAY_BUFFER, mesh.TexCoordsBuffer);
        glVertexAttribPointer(TEX_COORD_SLOT, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);
        glEnableVertexAttribArray(TEX_COORD_SLOT);
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.IndexBuffer);
    glDrawElements(GL_TRIANGLES, mesh.IndexCount, GL_UNSIGNED_INT, 0);

    glDisableVertexAttribArray(POSITION_SLOT);
    glDisableVertexAttribArray(NORMAL_SLOT);
    glDisableVertexAttribArray(TEX_COORD_SLOT);
}

Mesh LoadMesh(const char* path)
{
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

    // Open the CTM file:
    CTMcontext ctmContext = ctmNewContext(CTM_IMPORT);
    ctmLoad(ctmContext, fullpath.c_str());

    Mesh pod = {0};
    pod.VertexCount = ctmGetInteger(ctmContext, CTM_VERTEX_COUNT);
    pod.IndexCount = 3 * ctmGetInteger(ctmContext, CTM_TRIANGLE_COUNT);

    // Create the VBO for positions:
    const CTMfloat* positions = ctmGetFloatArray(ctmContext, CTM_VERTICES);
    GLsizeiptr size = pod.VertexCount * sizeof(float) * 3;
    glGenBuffers(1, &pod.PositionsBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, pod.PositionsBuffer);
    glBufferData(GL_ARRAY_BUFFER, size, positions, GL_STATIC_DRAW);
    glEnableVertexAttribArray(POSITION_SLOT);
    glVertexAttribPointer(POSITION_SLOT, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0);

    // Create the VBO for normals:
    const CTMfloat* normals = ctmGetFloatArray(ctmContext, CTM_NORMALS);
    if (normals) {
        GLsizeiptr size = pod.VertexCount * sizeof(float) * 3;
        glGenBuffers(1, &pod.NormalsBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, pod.NormalsBuffer);
        glBufferData(GL_ARRAY_BUFFER, size, normals, GL_STATIC_DRAW);
        glEnableVertexAttribArray(NORMAL_SLOT);
        glVertexAttribPointer(NORMAL_SLOT, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0);
    }

    // Create the VBO for texcoords:
    const CTMfloat* texcoords = ctmGetFloatArray(ctmContext, CTM_UV_MAP_1);
    if (texcoords) {
        GLsizeiptr size = pod.VertexCount * sizeof(float) * 2;
        glGenBuffers(1, &pod.TexCoordsBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, pod.TexCoordsBuffer);
        glBufferData(GL_ARRAY_BUFFER, size, texcoords, GL_STATIC_DRAW);
        glEnableVertexAttribArray(TEX_COORD_SLOT);
        glVertexAttribPointer(TEX_COORD_SLOT, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);
    }

    // Create the VBO for indices:
    const CTMuint* indices = ctmGetIntegerArray(ctmContext, CTM_INDICES);
    size = pod.IndexCount * sizeof(CTMuint);
    glGenBuffers(1, &pod.IndexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pod.IndexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices, GL_STATIC_DRAW);

    ctmFreeContext(ctmContext);

    return pod;
}
