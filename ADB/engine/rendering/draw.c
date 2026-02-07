#include <stdint.h>
#include <engine/math/vector.h>
#include "utilities.h"
#include "renderer.h"
#include "draw.h"


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


void DrawGizmoCell(vec3 Center, vec3 Color, render_batch_list *BatchList, memory_arena *Arena)
{
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