#include <stdint.h>
#include <assert.h>
#include <math.h>

#include "third_party/stb_image.h"

#include "utilities.h"         // Arenas
#include "platform/platform.h" // Engine Memory
#include "renderer.h"          // Implementation File


void *
PushDataInBatchList(memory_arena *Arena, render_batch_list *BatchList)
{
    void *Result = NULL;

    render_batch_node *Node = BatchList->Last;
    if (!Node || (Node->Value.ByteCount + BatchList->BytesPerInstance > Node->Value.ByteCapacity))
    {
        Node = PushStruct(Arena, render_batch_node);
        if (Node)
        {
            Node->Next = NULL;
            Node->Value.ByteCount    = 0;
            Node->Value.ByteCapacity = KiB(64); // Still an issue!
            Node->Value.Memory       = PushArray(Arena, uint8_t, Node->Value.ByteCapacity);

            if (!BatchList->First)
            {
                BatchList->First = Node;
                BatchList->Last  = Node;
            }
            else
            {
                BatchList->Last->Next = Node; // Warning but the logic should be sound?
                BatchList->Last = Node;
            }
        }
    }

    BatchList->ByteCount += BatchList->BytesPerInstance;
    BatchList->BatchCount += 1;

    if (Node)
    {
        Result = (void *)(Node->Value.Memory + Node->Value.ByteCount);
        Node->Value.ByteCount += BatchList->BytesPerInstance;
    }

    return Result;
}

render_pass *
GetRenderPass(memory_arena *Arena, RenderPassType Type, render_pass_list *PassList)
{
    render_pass_node *Result = PassList->Last;

    if (!Result || Result->Value.Type != Type)
    {
        Result = PushStruct(Arena, render_pass_node);
        if (!Result)
        {
            return NULL;
        }

        Result->Value.Type = Type;
        Result->Value.Params.UI.First = NULL;
        Result->Value.Params.UI.Last = NULL;
        Result->Value.Params.UI.Count = 0;
        Result->Next = NULL;

        if (!PassList->First)
        {
            PassList->First = Result;
        }

        if (PassList->Last)
        {
            PassList->Last->Next = Result;
        }

        PassList->Last = Result;
    }

    return &Result->Value;
}


// ==============================================
// <Resources> 
// ==============================================


#define INVALID_LINK_SENTINEL 0xFFFFFFFF


static renderer_base_resource *
GetBaseResource(RendererBaseResource_Type Type, void *Backend, renderer_resource_manager *ResourceManager)
{
    renderer_base_resource *Result = 0;

    if (Type != RendererBaseResource_None && Backend && ResourceManager)
    {
        Result = ResourceManager->Resources + ResourceManager->FirstFree;
        Result->Backend       = Backend;
        Result->_RefCount     = 1;
        Result->Type          = Type;
        Result->Next.SameType = ResourceManager->FirstByBaseType[Type];

        ResourceManager->FreeCount            -= 1;
        ResourceManager->FirstByBaseType[Type] = ResourceManager->FirstFree;
        ResourceManager->FirstFree             = Result->Next.Free;
        Result->Next.Free                      = INVALID_LINK_SENTINEL;
    }

    return Result;
}


static void *
GetCompositeResourceByType(RendererCompositeResource_Type Type, renderer_resource_manager *ResourceManager)
{
    void *Result = 0;

    if (Type != RendererCompositeResource_None && ResourceManager)
    {
        uint32_t Free     = ResourceManager->FreeByCompositeType[Type];
        uint32_t NextFree = INVALID_LINK_SENTINEL;

        if (Free != INVALID_LINK_SENTINEL)
        {
            switch (Type)
            {
            
            case RendererCompositeResource_Material:
            {
                renderer_material *Material = &ResourceManager->Materials[Free];
            
                Material->Id            = 0;
                // Material->Maps          = {0};
                Material->Next.SameType = ResourceManager->FirstByCompositeType[Type];

                NextFree            = Material->Next.Free;
                Material->Next.Free = INVALID_LINK_SENTINEL;

                Result = Material;
            } break;
            
            case RendererCompositeResource_StaticMesh:
            {
                renderer_static_mesh *StaticMesh = &ResourceManager->StaticMeshes[Free];
            
                StaticMesh->SubmeshCount     = 0;
                // StaticMesh->Submeshes        = {0};
                StaticMesh->VertexBuffer     = 0;
                StaticMesh->VertexBufferSize = 0;
                StaticMesh->Next.SameType    = ResourceManager->FirstByCompositeType[Type];
                
                NextFree              = StaticMesh->Next.Free;
                StaticMesh->Next.Free = INVALID_LINK_SENTINEL;

                Result = StaticMesh;
            } break;
            
            default:
            {
            
            } break;
            
            }
            
            ResourceManager->FirstByCompositeType[Type]  = Free;
            ResourceManager->CountByCompositeType[Type] += 1;
            ResourceManager->FreeByCompositeType[Type]   = NextFree;
        }
    }

    return Result;
}


void
InitializeResourceManager(renderer_resource_manager *ResourceManager)
{
    uint32_t ResourceCount = ArrayCount(ResourceManager->Resources);

    for (uint32_t ResourceIdx = 0; ResourceIdx < ResourceCount; ++ResourceIdx)
    {
        renderer_base_resource *Resource = ResourceManager->Resources + ResourceIdx;

        Resource->Backend   = 0;
        Resource->Type      = RendererBaseResource_None;
        Resource->_RefCount = 0;

        if (ResourceIdx < ResourceCount - 1)
        {
            Resource->Next.Free     = ResourceIdx + 1;
            Resource->Next.SameType = INVALID_LINK_SENTINEL;
        }
    }

    ResourceManager->FirstFree = 0;
    ResourceManager->FreeCount = ResourceCount;

    for (uint32_t ResourceType = RendererBaseResource_Texture2D; ResourceType < RendererBaseResource_Count; ++ResourceType)
    {
        ResourceManager->FirstByBaseType[ResourceType] = INVALID_LINK_SENTINEL;
    }

    for (uint32_t StaticMeshIdx = 0; StaticMeshIdx < MAX_STATIC_MESH_COUNT; ++StaticMeshIdx)
    {
        renderer_static_mesh *StaticMesh = &ResourceManager->StaticMeshes[StaticMeshIdx];

        if(StaticMeshIdx < MAX_STATIC_MESH_COUNT - 1)
        {
            StaticMesh->Next.Free     = StaticMeshIdx + 1;
            StaticMesh->Next.SameType = INVALID_LINK_SENTINEL;
        }
    }

    for (uint32_t MaterialIdx = 0; MaterialIdx < MAX_MATERIAL_COUNT; ++MaterialIdx)
    {
        renderer_material *Material = &ResourceManager->Materials[MaterialIdx];

        if (MaterialIdx < MAX_MATERIAL_COUNT - 1)
        {
            Material->Next.Free     = MaterialIdx + 1;
            Material->Next.SameType = INVALID_LINK_SENTINEL;
        }
    }

    for (uint32_t Type = RendererCompositeResource_Material; Type < RendererCompositeResource_Count; ++Type)
    {
        ResourceManager->FirstByCompositeType[Type] = INVALID_LINK_SENTINEL;
        ResourceManager->CountByCompositeType[Type] = 0;
        ResourceManager->FreeByCompositeType[Type]  = 0;
    }
}


void
CreateStaticMesh(asset_file_data AssetFile, renderer *Renderer)
{
    renderer_static_mesh *StaticMesh = GetCompositeResourceByType(RendererCompositeResource_StaticMesh, &Renderer->Resources);

    if (StaticMesh)
    {
        for (uint32_t MaterialIdx = 0; MaterialIdx < AssetFile.MaterialCount; ++MaterialIdx)
        {
            renderer_material *Material = GetCompositeResourceByType(RendererCompositeResource_Material, &Renderer->Resources);
        
            for (MaterialMap_Type MapType = MaterialMap_Color; MapType < MaterialMap_Count; ++MapType)
            {
                void *Data = RendererCreateTexture(AssetFile.Materials[MaterialIdx].Textures[MapType], Renderer);
                if (Data)
                {
                    Material->Maps[MapType] = GetBaseResource(RendererBaseResource_TextureView, Data, &Renderer->Resources);
                }

                stbi_image_free(AssetFile.Materials[MaterialIdx].Textures[MapType].Data);
            }
        }
        
        void *VertexBuffer = RendererCreateVertexBuffer(AssetFile.Vertices, AssetFile.VertexCount * sizeof(mesh_vertex_data), Renderer);
        if (VertexBuffer)
        {
            StaticMesh->VertexBuffer     = GetBaseResource(RendererBaseResource_VertexBuffer, VertexBuffer, &Renderer->Resources);
            StaticMesh->VertexBufferSize = AssetFile.VertexCount * sizeof(mesh_vertex_data);
        }

        assert(AssetFile.SubmeshCount < MAX_SUBMESH_COUNT);

        // This is why we need a way to query resources by stable handles I Guess? We have no way of accessing
        // the material resource. We'd just parse to stable handles (paths) and then we could hash or something to retrieve.

        StaticMesh->SubmeshCount = AssetFile.SubmeshCount;

        for (uint32_t SubmeshIdx = 0; SubmeshIdx < AssetFile.SubmeshCount; ++SubmeshIdx)
        {
            submesh_data *SubmeshData = &AssetFile.Submeshes[SubmeshIdx];

            StaticMesh->Submeshes[SubmeshIdx].Material    = &Renderer->Resources.Materials[0]; // Only correct, since we know exactly what we are doing, we'd want to query it.
            StaticMesh->Submeshes[SubmeshIdx].VertexCount = SubmeshData->VertexCount;
            StaticMesh->Submeshes[SubmeshIdx].VertexStart = SubmeshData->VertexOffset;
        }
    }
}


static_mesh_list
RendererGetAllStaticMeshes(engine_memory *EngineMemory, renderer *Renderer)
{
    static_mesh_list Result = {0};

    if (EngineMemory && Renderer)
    {
        renderer_resource_manager *ResourceManager = &Renderer->Resources;

        uint32_t Count = ResourceManager->CountByCompositeType[RendererCompositeResource_StaticMesh];
        uint32_t First = ResourceManager->FirstByCompositeType[RendererCompositeResource_StaticMesh];
        
        renderer_static_mesh **List = PushArray(EngineMemory->FrameMemory, renderer_static_mesh *, Count);
        if (List)
        {
            uint32_t Added = 0;
            while (First != INVALID_LINK_SENTINEL)
            {
                renderer_static_mesh *StaticMesh = &ResourceManager->StaticMeshes[First];
        
                List[Added++] = StaticMesh;
                First         = StaticMesh->Next.SameType;
            }

            Result.Data  = List;
            Result.Count = Count;
        
            assert(Added == Count);
        }
    }

    return Result;
}


// ==============================================
// <Camera>
// ==============================================


camera CreateCamera(vec3 Position, float FovY, float AspectRatio)
{
    camera Result =
    {
        .Position    = Position,
        .Forward     = Vec3(0.f, 0.f, 1.f),
        .Up          = Vec3(0.f, 1.f, 0.f),
        .AspectRatio = AspectRatio,
        .NearPlane   = 0.1f,
        .FarPlane    = 100.f,
        .FovY        = FovY,
    };

    return Result;
}


mat4x4 GetCameraWorldMatrix(camera *Camera)
{
    (void)Camera;

    mat4x4 World = {0};
    World.c0r0 = 1.f;
    World.c1r1 = 1.f;
    World.c2r2 = 1.f;
    World.c3r3 = 1.f;

    return World;
}


mat4x4 GetCameraViewMatrix(camera *Camera)
{
    mat4x4 View = {0};

    vec3 Right = Vec3Normalize(Vec3Cross(Camera->Up, Camera->Forward));
    vec3 Up    = Vec3Cross(Camera->Forward, Right);

    Camera->Up = Up;

    View.c0r0 = Right.X; View.c0r1 = Camera->Up.X; View.c0r2 = Camera->Forward.X; View.c0r3 = 0.f;
    View.c1r0 = Right.Y; View.c1r1 = Camera->Up.Y; View.c1r2 = Camera->Forward.Y; View.c1r3 = 0.f;
    View.c2r0 = Right.Z; View.c2r1 = Camera->Up.Z; View.c2r2 = Camera->Forward.Z; View.c2r3 = 0.f;
    View.c3r0 = -Vec3Dot(Right, Camera->Position);
    View.c3r1 = -Vec3Dot(Camera->Up, Camera->Position);
    View.c3r2 = -Vec3Dot(Camera->Forward, Camera->Position);
    View.c3r3 = 1.f;

    return View;
}


mat4x4 GetCameraProjectionMatrix(camera *Camera)
{
    mat4x4 Projection = {0};

    float FovY        = Camera->FovY;
    float AspectRatio = Camera->AspectRatio;
    float Near        = Camera->NearPlane;
    float Far         = Camera->FarPlane;

    float F = 1.f / tanf(FovY / 2.f);

    Projection.c0r0 = F / AspectRatio; Projection.c0r1 = 0.f; Projection.c0r2 = 0.f;                                Projection.c0r3 = 0.f;
    Projection.c1r0 = 0.f;             Projection.c1r1 = F;   Projection.c1r2 = 0.f;                                Projection.c1r3 = 0.f;
    Projection.c2r0 = 0.f;             Projection.c2r1 = 0.f; Projection.c2r2 = (Far + Near) / (Far - Near);        Projection.c2r3 = 1.f;
    Projection.c3r0 = 0.f;             Projection.c3r1 = 0.f; Projection.c3r2 = (-2.f * Far * Near) / (Far - Near); Projection.c3r3 = 0.f;

    return Projection;
}