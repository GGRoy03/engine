#pragma once

#include <stdint.h>

#include <utilities.h>
#include <engine/math/vector.h>

typedef struct renderer                  renderer;
typedef struct engine_memory             engine_memory;
typedef struct resource_reference_table  resource_reference_table;
typedef struct renderer_resource_manager renderer_resource_manager;

#define MAX_SUBMESH_COUNT 8

typedef enum
{
    RendererResource_None = 0,

    // Base
    RendererResource_Texture2D,
    RendererResource_TextureView,
    RendererResource_VertexBuffer,

    // Composite
    RendererResource_Material,
    RendererResource_StaticMesh,

    RendererResource_Count,
} RendererResource_Type;


typedef enum
{
    MaterialMap_Albedo = 0,
    MaterialMap_Count = 1,
} MaterialMap_Type;



typedef struct
{
    uint32_t    Width;
    uint32_t    Height;
    uint32_t    BytesPerPixel;
    uint8_t    *Data;
    byte_string Path;
} loaded_texture;


typedef struct
{
    uint32_t              Value;
    RendererResource_Type Type;
} resource_handle;

typedef struct
{
    uint64_t Value;
} resource_uuid;

typedef struct
{
    uint32_t        Id;
    resource_handle Handle;
} resource_state;

// 
// I don't know yet if these types should be exposed. For simplicity, I'll put them here for now.

typedef struct
{
    void *Data;
} renderer_backend_resource;


typedef struct
{
    void  *Backend;
    size_t Size;
} renderer_buffer;


typedef struct
{
    resource_handle Maps[MaterialMap_Count];
} renderer_material;


typedef struct
{
    uint64_t VertexCount;
    uint64_t VertexStart;
} renderer_static_submesh;


typedef struct
{
    resource_handle         VertexBuffer;
    uint64_t                VertexBufferSize;
    renderer_static_submesh Submeshes[MAX_SUBMESH_COUNT];
    uint32_t                SubmeshCount;
} renderer_static_mesh;


resource_uuid               MakeResourceUUID             (byte_string PathToResource);
                                                         
bool                        IsValidResourceHandle        (resource_handle Handle);
resource_handle             CreateResourceHandle         (resource_uuid UUID, RendererResource_Type Type, renderer_resource_manager *ResourceManager);
resource_handle             BindResourceHandle           (resource_handle Handle, renderer_resource_manager *ResourceManager);
resource_handle             UnbindResourceHandle         (resource_handle Handle, renderer_resource_manager *ResourceManager);

renderer_resource_manager * CreateResourceManager        (memory_arena *Arena);
resource_reference_table  * CreateResourceReferenceTable (memory_arena *Arena);

void                      * AccessUnderlyingResource     (resource_handle Handle, renderer_resource_manager *ResourceManager);


// =====================================================
// [SECTION] DEFAULT RESOURCES
// =====================================================


resource_handle GetDefaultMaterial  (renderer *Renderer, memory_arena *Arena);


// =====================================================
// [SECTION] RESOURCE QUERIES
// =====================================================

renderer_buffer * GetRendererBufferFromHandle  (resource_handle Handle, renderer_resource_manager *ResourceManager);

resource_handle   UpdateVertexBuffer           (byte_string BufferName, void *Data, uint64_t Size, memory_arena *Arena, renderer *Renderer);