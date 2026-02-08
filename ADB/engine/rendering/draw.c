#include "draw.h"

#include <stdint.h>

#include "utilities.h"
#include "renderer.h"
#include "renderer_internal.h"
#include "engine/math/vector.h"


// =====================================================
// Draw Templates
// =====================================================


static gizmo_vertex_data CellTemplate[24] =
{
    {.Position = {-0.50f, -0.50f, 0.0f}, .Color = {1.0f, 1.0f, 1.0f}},
    {.Position = { 0.50f, -0.50f, 0.0f}, .Color = {1.0f, 1.0f, 1.0f}},
    {.Position = { 0.50f, -0.45f, 0.0f}, .Color = {1.0f, 1.0f, 1.0f}},
                                       
    {.Position = {-0.50f, -0.50f, 0.0f}, .Color = {1.0f, 1.0f, 1.0f}},
    {.Position = { 0.50f, -0.45f, 0.0f}, .Color = {1.0f, 1.0f, 1.0f}},
    {.Position = {-0.50f, -0.45f, 0.0f}, .Color = {1.0f, 1.0f, 1.0f}},

    {.Position = {-0.50f,  0.45f, 0.0f}, .Color = {1.0f, 1.0f, 1.0f}},
    {.Position = { 0.50f,  0.45f, 0.0f}, .Color = {1.0f, 1.0f, 1.0f}},
    {.Position = { 0.50f,  0.50f, 0.0f}, .Color = {1.0f, 1.0f, 1.0f}},

    {.Position = {-0.50f,  0.45f, 0.0f}, .Color = {1.0f, 1.0f, 1.0f}},
    {.Position = { 0.50f,  0.50f, 0.0f}, .Color = {1.0f, 1.0f, 1.0f}},
    {.Position = {-0.50f,  0.50f, 0.0f}, .Color = {1.0f, 1.0f, 1.0f}},

    {.Position = {-0.50f, -0.50f, 0.0f}, .Color = {1.0f, 1.0f, 1.0f}},
    {.Position = {-0.45f, -0.50f, 0.0f}, .Color = {1.0f, 1.0f, 1.0f}},
    {.Position = {-0.45f,  0.50f, 0.0f}, .Color = {1.0f, 1.0f, 1.0f}},

    {.Position = {-0.50f, -0.50f, 0.0f}, .Color = {1.0f, 1.0f, 1.0f}},
    {.Position = {-0.45f,  0.50f, 0.0f}, .Color = {1.0f, 1.0f, 1.0f}},
    {.Position = {-0.50f,  0.50f, 0.0f}, .Color = {1.0f, 1.0f, 1.0f}},

    {.Position = { 0.45f, -0.50f, 0.0f}, .Color = {1.0f, 1.0f, 1.0f}},
    {.Position = { 0.50f, -0.50f, 0.0f}, .Color = {1.0f, 1.0f, 1.0f}},
    {.Position = { 0.50f,  0.50f, 0.0f}, .Color = {1.0f, 1.0f, 1.0f}},

    {.Position = { 0.45f, -0.50f, 0.0f}, .Color = {1.0f, 1.0f, 1.0f}},
    {.Position = { 0.50f,  0.50f, 0.0f}, .Color = {1.0f, 1.0f, 1.0f}},
    {.Position = { 0.45f,  0.50f, 0.0f}, .Color = {1.0f, 1.0f, 1.0f}},
};



// =====================================================
// Draw Calls API
// =====================================================


void DrawGizmoCell(vec3 Center, vec3 Color, camera *Camera, renderer *Renderer, memory_arena *Arena)
{
    if (!Camera || !Renderer || !Arena)
    {
        return;
    }

    gizmo_group_params Params =
    {
    	.WorldMatrix      = GetCameraWorldMatrix(Camera),
    	.ViewMatrix       = GetCameraViewMatrix(Camera),
    	.ProjectionMatrix = GetCameraProjectionMatrix(Camera),
    };

	render_batch_list *BatchList = PushGizmoGroupParams(&Params, Arena, &Renderer->PassList);
    if (BatchList)
    {
        // TODO: Just don't have errors bro. Maybe return some buffer struct?
        gizmo_vertex_data *Vertices = PushDataInBatchList(Arena, BatchList, ArrayCount(CellTemplate), GIZMO_INSTANCE_PER_BATCH);
        if (Vertices)
        {
            for (uint32_t Idx = 0; Idx < ArrayCount(CellTemplate); ++Idx)
            {
                gizmo_vertex_data *Vertex = &Vertices[Idx];
                Vertex->Color    = Color;
                Vertex->Position = Vec3Add(CellTemplate[Idx].Position, Center);
            }
        }
    }
}


// =====================================================
// [SECTION] Chunk Draw API
// =====================================================


void
DrawChunkIntance(resource_handle VertexBuffer, uint32_t VertexCount, resource_handle Material, camera *Camera, renderer *Renderer, memory_arena *Arena)
{
    if (!Camera || !Renderer || !Arena)
    {
        return;
    }

    chunk_group_params Params =
    {
        .WorldMatrix      = GetCameraWorldMatrix(Camera),
        .ViewMatrix       = GetCameraViewMatrix(Camera),
        .ProjectionMatrix = GetCameraProjectionMatrix(Camera),
    };

    render_batch_list *BatchList = PushChunkGroupParams(&Params, Arena, &Renderer->PassList);
    if(BatchList)
    {
        chunk_batch_params BatchParams =
        {
            .Material     = Material,
            .VertexBuffer = VertexBuffer,
            .VertexCount  = VertexCount,
        };

        PushChunkBatchParams(&BatchParams, CHUNK_INSTANCE_PER_BATCH, Arena, BatchList);
    }
}