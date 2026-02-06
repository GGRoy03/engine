#pragma once

#include "engine/math/matrix.h"
#include "engine/math/vector.h"

// Probably Temporary
#include "resources.h"

// =====================================================
// Forward declarations
// =====================================================

typedef struct renderer      renderer;
typedef struct engine_memory engine_memory;

// =====================================================
// TEMP / shared vertex formats
// =====================================================

typedef struct mesh_vertex
{
    vec3 Position;
    vec2 TexCoord;
    vec3 Normal;
} mesh_vertex;


// -----------------------------------------------------
// Backend hooks (implemented per renderer backend)
// -----------------------------------------------------

void *RendererCreateTexture       (loaded_texture Texture, renderer *Renderer);
void *RendererCreateVertexBuffer  (void *Data, uint64_t Size, renderer *Renderer);

// =====================================================
// Camera (likely scene-level later)
// =====================================================

typedef struct
{
    vec3 Position;
    vec3 Forward;
    vec3 Up;

    float FovY;
    float AspectRatio;
    float NearPlane;
    float FarPlane;
} camera;

camera CreateCamera(vec3 Position, float FovY, float AspectRatio);

mat4x4 GetCameraWorldMatrix(camera *Camera);
mat4x4 GetCameraViewMatrix(camera *Camera);
mat4x4 GetCameraProjectionMatrix(camera *Camera);

// =====================================================
// Lighting
// =====================================================

typedef struct
{
    vec3  Position;
    float Intensity;
    vec3  Color;
    float _Pad0;
} light_source;

// =====================================================
// Drawing / submission
// =====================================================

#define MAX_LIGHT_COUNT 8

typedef enum
{
    RenderPass_None = 0,
    RenderPass_Mesh,
    RenderPass_UI,
} RenderPassType;

typedef enum
{
    RenderCommand_None = 0,
    RenderCommand_StaticGeometry,
} RenderCommand_Type;

// -----------------------------------------------------
// Shared utility types (maybe move later)
// -----------------------------------------------------

typedef struct bounding_box
{
    float Left, Top, Right, Bottom;
} bounding_box;

typedef struct color
{
    float R, G, B, A;
} color;

// -----------------------------------------------------
// Render primitives
// -----------------------------------------------------

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
} gui_rect;


typedef struct
{
    resource_handle MeshHandle;
    uint32_t        SubmeshIndex;
    vec3            Transform;
} mesh_instance;


// -----------------------------------------------------
// Batching
// -----------------------------------------------------

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

typedef struct render_batch_node render_batch_node;
struct render_batch_node
{
    render_batch_node *Next;
    render_batch       Value;

    union
    {
        mesh_batch_params MeshParams;
        ui_batch_params   UIParams;
    };
};

typedef struct
{
    render_batch_node *First;
    render_batch_node *Last;
    uint64_t           BatchCount;
    uint64_t           BytesPerInstance;
} render_batch_list;

// -----------------------------------------------------
// Mesh pass grouping
// -----------------------------------------------------

typedef struct
{
    mat4x4       WorldMatrix;
    mat4x4       ViewMatrix;
    mat4x4       ProjectionMatrix;
    vec3         CameraPosition;

    light_source Lights[MAX_LIGHT_COUNT];
    uint32_t     LightCount;
} mesh_group_params;

typedef struct mesh_group_node mesh_group_node;
struct mesh_group_node
{
    mesh_group_node *Next;
    render_batch_list BatchList;
    mesh_group_params Params;
};

typedef struct
{
    mesh_group_node *First;
    mesh_group_node *Last;
    uint32_t         Count;
} render_pass_params_mesh;

// -----------------------------------------------------
// UI pass grouping
// -----------------------------------------------------

typedef struct
{
    bounding_box Clip;
} ui_group_params;

typedef struct ui_group_node ui_group_node;
struct ui_group_node
{
    ui_group_node *Next;
    render_batch_list BatchList;
    ui_group_params   Params;
};

typedef struct
{
    ui_group_node *First;
    ui_group_node *Last;
    uint32_t       Count;
} render_pass_params_ui;

// -----------------------------------------------------
// Render passes
// -----------------------------------------------------

typedef struct
{
    RenderPassType Type;
    union
    {
        render_pass_params_mesh Mesh;
        render_pass_params_ui   UI;
    } Params;
} render_pass;

typedef struct render_pass_node render_pass_node;
struct render_pass_node
{
    render_pass_node *Next;
    render_pass       Value;
};

typedef struct
{
    render_pass_node *First;
    render_pass_node *Last;
} render_pass_list;

// -----------------------------------------------------
// Submission helpers
// -----------------------------------------------------

#define MESH_INSTANCE_PER_BATCH 10
#define UI_INSTANCE_PER_BATCH   50

void              * PushDataInBatchList(memory_arena *Arena, render_batch_list *BatchList, uint64_t InstancePerBatch);
                    
render_batch_list * PushMeshGroupParams(mesh_group_params *Params, memory_arena *Arena, render_pass_list *PassList);
render_batch_list * PushUIGroupParams(ui_group_params *Params, memory_arena *Arena, render_pass_list *PassList);
                    
void                PushMeshBatchParams(mesh_batch_params *Params, uint64_t InstancePerBatch, memory_arena *Arena, render_batch_list *BatchList);
void                PushUIBatchParams(ui_batch_params *Params, uint64_t InstancePerBatch, memory_arena *Arena, render_batch_list *BatchList);

// =====================================================
// Frame control
// =====================================================

typedef struct
{
    float R, G, B, A;
} clear_color;

void RendererEnterFrame(clear_color Color, renderer *Renderer);
void RendererLeaveFrame(int Width, int Height, engine_memory *EngineMemory, renderer *Renderer);

// =====================================================
// Renderer root object
// =====================================================

typedef struct renderer
{
    void *Backend;
    render_pass_list           PassList;
    renderer_resource_manager *Resources;
    resource_reference_table *ReferenceTable;
} renderer;