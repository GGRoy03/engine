#pragma once

#include "assets.h"
#include "engine/math/matrix.h"
#include <engine/math/vector.h>

typedef struct renderer renderer;




// ==============================================
// <Resources> 
// ==============================================


typedef struct renderer_resource_manager renderer_resource_manager;
typedef struct resource_reference_table  resource_reference_table;


#define MAX_SUBMESH_COUNT 8


typedef enum
{
    RendererResource_None = 0,

    // Base

    RendererResource_Texture2D = 1,
    RendererResource_TextureView = 2,
    RendererResource_VertexBuffer = 3,

    // Composite

    RendererResource_Material = 4,
    RendererResource_StaticMesh = 5,

    RendererResource_Count = 6,
} RendererResource_Type;


typedef struct
{
    uint32_t              Value;
    RendererResource_Type Type;
} resource_handle;


typedef struct
{
    void *Data;
} renderer_backend_resource;


typedef struct
{
    resource_handle Maps[MaterialMap_Count];
} renderer_material;


typedef struct
{
    uint64_t        VertexCount;
    uint64_t        VertexStart;
    resource_handle Material;
} renderer_static_submesh;


typedef struct
{
    resource_handle         VertexBuffer;
    uint64_t                VertexBufferSize;
    renderer_static_submesh Submeshes[MAX_SUBMESH_COUNT];
    uint32_t                SubmeshCount;
} renderer_static_mesh;


typedef struct
{
    uint64_t Value;
} resource_uuid;


typedef struct
{
    uint32_t        Id;
    resource_handle Handle;
} resource_reference_state;


void                        LoadAssetFileData             (asset_file_data AssetFile, memory_arena *Arena, renderer *Renderer);

resource_uuid               MakeResourceUUID              (byte_string PathToResource);
resource_reference_state    FindResourceByUUID            (resource_uuid UUID, resource_reference_table *Table);

resource_handle             BindResourceHandle            (resource_handle Handle, renderer_resource_manager *ResourceManager);
resource_handle             UnbindResourceHandle          (resource_handle Handle, renderer_resource_manager *ResourceManager);


renderer_resource_manager * CreateResourceManager         (memory_arena *Arena);
resource_reference_table  * CreateResourceReferenceTable  (memory_arena *Arena);

void * AccessUnderlyingResource  (resource_handle Handle, renderer_resource_manager *ResourceManager);


// 
// Backend specific functions that must be implemented by every backend
//


void * RendererCreateTexture       (loaded_texture Texture, renderer *Renderer);
void * RendererCreateVertexBuffer  (void *Data, uint64_t Size, renderer *Renderer);

// ==============================================
// <Camera>
// ==============================================

// Probably move this to scene.

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

camera CreateCamera               (vec3 Position, float FovY, float AspectRatio);

mat4x4 GetCameraWorldMatrix       (camera *Camera);
mat4x4 GetCameraViewMatrix        (camera *Camera);
mat4x4 GetCameraProjectionMatrix  (camera *Camera);


// ==============================================
// <Drawing>
// ==============================================


#define MAX_RENDER_COMMAND_COUNT 64

typedef enum
{
    RenderPass_None = 0,
    RenderPass_Mesh = 1,
} RenderPassType;


typedef enum
{
    RenderCommand_None = 0,

    RenderCommand_StaticGeometry = 1,
} RenderCommand_Type;


typedef struct
{
    resource_handle MeshHandle;
} static_geometry_command;


typedef struct
{
    RenderCommand_Type Type;

    union
    {
        static_geometry_command StaticGeometry;
    };
} render_command;


typedef struct
{
    render_command *Commands;
    uint32_t        Count;
    uint32_t        Capacity;
} render_command_batch;


typedef struct
{
    resource_handle Material;
} mesh_batch_params;


typedef struct render_command_batch_node render_command_batch_node;
struct render_command_batch_node
{
    render_command_batch_node *Next;
    render_command_batch       Value;

    union
    {
        mesh_batch_params MeshParams;
    };
};


typedef struct
{
    render_command_batch_node *First;
    render_command_batch_node *Last;
    uint64_t                   BatchCount;
} render_command_batch_list;


typedef struct
{
    mat4x4 WorldMatrix;
    mat4x4 ViewMatrix;
    mat4x4 ProjectionMatrix;
} mesh_group_params;


typedef struct mesh_group_node mesh_group_node;
struct mesh_group_node
{
    mesh_group_node          *Next;
    render_command_batch_list BatchList;
    mesh_group_params         Params;
};


typedef struct
{
    mesh_group_node *First;
    mesh_group_node *Last;
    uint32_t         Count;
} render_pass_params_mesh;


typedef struct
{
    RenderPassType Type;
    union
    {
        render_pass_params_mesh Mesh;
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
} render_command_pass_list;


// Experimental API

render_command            * PushRenderCommand    (render_command_batch *Batch);

render_command_batch_list * PushMeshGroupParams  (mesh_group_params *Params, memory_arena *Arena, render_command_pass_list *PassList);
render_command_batch      * PushMeshBatchParams  (mesh_batch_params *Params, memory_arena *Arena, render_command_batch_list *BatchList);


typedef struct
{
    float R, G, B, A;
} clear_color;


void RendererStartFrame  (clear_color Color, renderer *Renderer);
void RendererDrawFrame   (int Width, int Height, engine_memory *EngineMemory, renderer *Renderer);
void RendererFlushFrame  (renderer *Renderer);

// ==============================================
// <> 
// ==============================================


typedef struct renderer
{
    void                      *Backend;
    render_command_pass_list   PassList;
    renderer_resource_manager *Resources;
    resource_reference_table  *ReferenceTable;
} renderer;