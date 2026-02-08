#pragma once

#include <stdint.h>

#include "resources.h"
#include <engine/math/vector.h>


// =====================================================
// [SECTION] Forward Declarations
// =====================================================


typedef struct tile_vertex_data tile_vertex_data;
typedef struct memory_arena     memory_arena;
typedef struct camera           camera;
typedef struct renderer         renderer;


// =====================================================
// [SECTION] Gizmo Draw API
// =====================================================


void DrawGizmoCell  (vec3 Center, vec3 Color, camera *Camera, renderer *Renderer, memory_arena *Arena);


// =====================================================
// [SECTION] Chunk Draw API
// =====================================================


void     DrawChunkIntance    (resource_handle VertexBuffer, uint32_t VertexCount, resource_handle Material, camera *Camera, renderer *Renderer, memory_arena *Arena);