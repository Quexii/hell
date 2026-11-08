#include "mesh.h"

#include "gpu/gpu.h"

Mesh Mesh_Create(
    Device device,
    CopyPass *pass,
    UploadArena *arena,
    const void *vertices,
    uint32_t vertexCount,
    uint32_t stride
) {
    Mesh mesh = { 0 };

    if (!pass || !pass->handle || !arena || !vertices || !vertexCount || !stride)
        return mesh;

    uint64_t byteSize = (uint64_t) vertexCount * stride;
    if (byteSize > UINT32_MAX)
        return mesh;

    Buffer buffer = GPU_CreateBuffer(
        device, &(BufferDesc){ .usage = BUFFERUSAGE_VERTEX, .size = (uint32_t) byteSize }
    );
    if (!buffer.handle)
        return mesh;

    UploadAllocation allocation;
    if (!UploadArena_Begin(arena)) {
        GPU_ReleaseBuffer(&buffer);
        return mesh;
    }

    bool ok = UploadArena_Write(arena, vertices, (uint32_t) byteSize, 16, &allocation);
    UploadArena_End(arena);

    if (!ok) {
        GPU_ReleaseBuffer(&buffer);
        return mesh;
    }

    if (!UploadArena_UploadBuffer(arena, pass, &buffer, 0, allocation, false)) {
        GPU_ReleaseBuffer(&buffer);
        return mesh;
    }

    return (Mesh){ .vb = buffer, .vertexCount = vertexCount };
}

Mesh Mesh_CreateQuad(Device device, CopyPass *pass, UploadArena *arena) {
    return Mesh_Create(device, pass, arena, QUAD_VERTICES, 6, sizeof(MeshVertex));
}

void Mesh_Bind(RenderPass *pass, Mesh *mesh) {
    GPU_BindVBO(pass, &mesh->vb, 0, 0);
}

void Mesh_Draw(RenderPass *pass, Mesh *mesh, uint32_t instances) {
    GPU_DrawPrimitives(pass, mesh->vertexCount, instances, 0, 0);
}

void Mesh_Destroy(Mesh *mesh) {
    GPU_ReleaseBuffer(&mesh->vb);
    *mesh = (Mesh){ 0 };
}
