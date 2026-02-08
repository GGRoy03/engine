#pragma once

#include <stdint.h>

#include "engine/math/vector.h"
#include "engine/math/matrix.h"

#include "resources.h"

// =====================================================
// Forward declaration
// =====================================================

typedef struct memory_arena memory_arena;

// =====================================================
// [SECTION] Core Vertex Formats
// =====================================================

typedef struct
{
    vec3 Position;
    vec2 Texture;
    vec3 Normal;
} mesh_vertex_data;


typedef struct tile_vertex_data
{
    vec3 Position;
    vec2 UV;
} tile_vertex_data;


typedef struct gizmo_vertex_data
{
    vec3 Position;
    vec3 Color;
} gizmo_vertex_data;


typedef struct bounding_box
{
    float Left, Top, Right, Bottom;
} bounding_box;


typedef struct color
{
    float R, G, B, A;
} color;


typedef struct
{
    bounding_box Bounds;
    bounding_box TextureSource;
    color        ColorTL;
    color        ColorBL;
    color        ColorTR;
    color        ColorBR;
    bounding_box CornerRadius;
    float        BorderWidth;
    float        Softness;
    float        SampleTexture;
    float        _Padding0;
} ui_vertex_data;


// =====================================================
// [SECTION] Core Instance Formats
// [DESCRIP]
//   An instance represents a logical data unit that may
//   or may not be retained by some subsystem. It is
//   not a vertices, but represents a group of vertices.
// =====================================================

typedef struct
{
    tile_vertex_data Data[6];
    vec2             UV;
} tile_instance;

typedef struct
{
    resource_handle MeshHandle;
    uint32_t        SubmeshIndex;
    vec3            Transform;
} mesh_instance;


// =====================================================
// Render Data & Batching
// =====================================================


typedef enum RenderPass_Type
{
    RenderPass_None = 0,
    RenderPass_Mesh = 1,
    RenderPass_UI = 2,
    RenderPass_Gizmo = 3,
    RenderPass_Chunk = 4,
} RenderPass_Type;


typedef struct
{
    uint8_t *Memory;
    uint64_t ByteCount;
    uint64_t ByteCapacity;
} render_batch;


typedef struct
{
    resource_handle Material;
} mesh_batch_params;


typedef struct
{
    void *Data;
} ui_batch_params;


typedef struct
{
    uint64_t        VertexCount;
    resource_handle VertexBuffer;
    resource_handle Material;
} chunk_batch_params;


typedef struct render_batch_node render_batch_node;
struct render_batch_node
{
    render_batch_node *Next;
    render_batch       Value;

    union
    {
        mesh_batch_params  Mesh;
        ui_batch_params    UI;
        chunk_batch_params Chunk;
    } Params;
};


typedef struct render_batch_list
{
    render_batch_node *First;
    render_batch_node *Last;
    uint64_t           BatchCount;
    uint64_t           BytesPerInstance;
} render_batch_list;


typedef struct
{
    mat4x4 WorldMatrix;
    mat4x4 ViewMatrix;
    mat4x4 ProjectionMatrix;
} mesh_group_params;


typedef struct
{
    void *Data;
} ui_group_params;


typedef struct
{
    mat4x4 WorldMatrix;
    mat4x4 ViewMatrix;
    mat4x4 ProjectionMatrix;
} gizmo_group_params;


typedef struct
{
    mat4x4 WorldMatrix;
    mat4x4 ViewMatrix;
    mat4x4 ProjectionMatrix;
} chunk_group_params;


typedef struct render_group_node render_group_node;
struct render_group_node
{
    render_group_node *Next;
    render_batch_list  BatchList;

    union
    {
        mesh_group_params  Mesh;
        ui_group_params    UI;
        gizmo_group_params Gizmo;
        chunk_group_params Chunk;
    } Params;
};


typedef struct
{
    RenderPass_Type    Type;
    render_group_node *First;
    render_group_node *Last;
} render_pass;


typedef struct render_pass_node render_pass_node;
struct render_pass_node
{
    render_pass_node *Next;
    render_pass       Value;
};


typedef struct render_pass_list
{
    render_pass_node *First;
    render_pass_node *Last;
} render_pass_list;


#define MESH_INSTANCE_PER_BATCH  10
#define UI_INSTANCE_PER_BATCH    50
#define GIZMO_INSTANCE_PER_BATCH 1024
#define CHUNK_INSTANCE_PER_BATCH 32


void              * PushDataInBatchList   (memory_arena *Arena, render_batch_list *BatchList, uint32_t InstanceCount, uint64_t InstancePerBatch);
                    
render_batch_list * PushUIGroupParams     (ui_group_params *Params, memory_arena *Arena, render_pass_list *PassList);
render_batch_list * PushMeshGroupParams   (mesh_group_params *Params, memory_arena *Arena, render_pass_list *PassList);
render_batch_list * PushGizmoGroupParams  (gizmo_group_params *Params, memory_arena *Arena, render_pass_list *PassList);
render_batch_list * PushChunkGroupParams  (chunk_group_params *Params, memory_arena *Arena, render_pass_list *PassList);
                    
void                PushMeshBatchParams   (mesh_batch_params *Params, uint64_t InstancePerBatch, memory_arena *Arena, render_batch_list *BatchList);
void                PushUIBatchParams     (ui_batch_params *Params, uint64_t InstancePerBatch, memory_arena *Arena, render_batch_list *BatchList);
void                PushChunkBatchParams  (chunk_batch_params *Params, uint64_t InstancePerBatch, memory_arena *Arena, render_batch_list *BatchList);


// =====================================================
// Backend hooks (implemented per renderer backend)
// =====================================================


void *RendererCreateTexture(loaded_texture Texture, renderer *Renderer);
void *RendererCreateVertexBuffer(void *Data, uint64_t Size, renderer *Renderer);


// =====================================================
// Core Structure
// =====================================================

typedef struct render_pass_list render_pass_list;
typedef struct renderer
{
    void                      *Backend;
    render_pass_list           PassList;
    renderer_resource_manager *Resources;
    resource_reference_table  *ReferenceTable;
} renderer;