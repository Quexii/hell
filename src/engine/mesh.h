#ifndef ENGINE_MESH_H
#define ENGINE_MESH_H

#include "maths.h"
#include "gpu/gpu.h"

#include <stdint.h>

typedef struct {
    Buffer vb;
    uint32_t vertexCount;
} Mesh;

typedef struct {
    Vec3 pos;
    Vec2 uv;
} MeshVertex;

static const MeshVertex QUAD_VERTICES[6] = {
    { { -0.5f, -0.5f, 0 }, { 0, 0 } },
    { { 0.5f, -0.5f, 0 },  { 1, 0 } },
    { { 0.5f, 0.5f, 0 },   { 1, 1 } },
    { { -0.5f, -0.5f, 0 }, { 0, 0 } },
    { { 0.5f, 0.5f, 0 },   { 1, 1 } },
    { { -0.5f, 0.5f, 0 },  { 0, 1 } },
};

Mesh Mesh_Create(
    Device device,
    CopyPass *pass,
    UploadArena *arena,
    const void *vertices,
    uint32_t vertexCount,
    uint32_t stride
);

Mesh Mesh_CreateQuad(Device device, CopyPass *pass, UploadArena *arena);

void Mesh_Bind(RenderPass *pass, Mesh *mesh);

void Mesh_Draw(RenderPass *pass, Mesh *mesh, uint32_t instances);

void Mesh_Destroy(Mesh *mesh);

#endif /* ifndef ENGINE_MESH_HENGINE_MESH_H */
