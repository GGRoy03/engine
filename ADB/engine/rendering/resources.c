// =====================================================
// Header Mess
// =====================================================

#include <assert.h>
#include <stdint.h>

#include "resources.h"
#include "renderer_internal.h"
#include "platform/platform.h"


// =====================================================
// File Specific Constants
// =====================================================

#define INVALID_LINK_SENTINEL   0xFFFFFFFF
#define INVALID_RESOURCE_ENTRY  0xFFFFFFFF
#define INVALID_RESOURCE_HANDLE 0xFFFFFFFF
#define MAX_RENDERER_RESOURCE   128

// =====================================================
// Internal Only Types
// =====================================================

typedef struct resource_reference_entry
{
    resource_uuid   UUID;
    resource_handle Handle;
    uint32_t        NextSameHash;
} resource_reference_entry;


typedef struct resource_reference_table
{
    uint32_t                 HashMask;
    uint32_t                 HashCount;
    uint32_t                 EntryCount;

    uint32_t                 HashTable[32];
    resource_reference_entry Entries[MAX_RENDERER_RESOURCE];
    uint32_t                 FirstFreeEntry;
} resource_reference_table;


typedef struct renderer_resource
{
    RendererResource_Type Type;
    uint32_t              RefCount;
    uint32_t              NextFree;
    uint32_t              NextSameType;
    resource_uuid         UUID;

    union
    {
        renderer_backend_resource Backend;
        renderer_material         Material;
        renderer_static_mesh      StaticMesh;
        renderer_buffer           Buffer;
    };
} renderer_resource;


typedef struct renderer_resource_manager
{
    // Resources

    renderer_resource Resources[MAX_RENDERER_RESOURCE];
    uint32_t          FirstFree;
    uint32_t          FirstByType[RendererResource_Count];
    uint32_t          CountByType[RendererResource_Count];
} renderer_resource_manager;


// =====================================================
// Various Internal Helpers
// =====================================================

static resource_handle
MakeInvalidResourceHandle(void)
{
	resource_handle Result =
	{
		.Type  = RendererResource_None,
		.Value = INVALID_RESOURCE_HANDLE,
	};

	return Result;
}

static resource_reference_entry *
GetEntry(uint32_t Index, resource_reference_table *Table)
{
    assert(Index < Table->EntryCount);

    resource_reference_entry *Result = Table->Entries + Index;
    return Result;
}


static uint32_t *
GetSlotPointer(resource_uuid UUID, resource_reference_table *Table)
{
    uint64_t HashIndex = UUID.Value;
    uint32_t HashSlot = (HashIndex & Table->HashMask);

    assert(HashSlot < Table->HashCount);
    uint32_t *Result = &Table->HashTable[HashSlot];

    return Result;
}


static bool
ResourceUUIDAreEqual(resource_uuid A, resource_uuid B)
{
    bool Result = A.Value == B.Value;
    return Result;
}


static uint32_t
PopFreeEntry(resource_reference_table *Table)
{
    uint32_t Result = Table->FirstFreeEntry;

    if (Result != INVALID_RESOURCE_ENTRY)
    {
        resource_reference_entry *Entry = GetEntry(Result, Table);
        assert(Entry);

        Table->FirstFreeEntry = Entry->NextSameHash;
        Entry->NextSameHash = INVALID_RESOURCE_ENTRY;

    }

    return Result;
}

static void
UpdateResourceReferenceTable(uint32_t Id, resource_handle Handle, resource_reference_table *Table)
{
	resource_reference_entry *Entry = GetEntry(Id, Table);
	assert(Entry);

	Entry->Handle = Handle;
}


static renderer_resource *
GetRendererResource(uint32_t Id, renderer_resource_manager *ResourceManager)
{
	assert(Id < MAX_RENDERER_RESOURCE);
	assert(ResourceManager);

	renderer_resource *Result = ResourceManager->Resources + Id;
	return Result;
}


// =====================================================
// Resource API Implementation
// =====================================================


static resource_uuid
MakeResourceUUID(byte_string PathToResource)
{
    resource_uuid Result = { .Value = HashByteString(PathToResource) };
    return Result;
}


static resource_handle
SearchResourceByUUID(resource_uuid UUID, resource_reference_table *Table)
{
    resource_handle Result = MakeInvalidResourceHandle();

    if (Table)
    {
        uint32_t *Slot       = GetSlotPointer(UUID, Table);
        uint32_t  EntryIndex = *Slot;

        while (EntryIndex != INVALID_RESOURCE_ENTRY)
        {
            resource_reference_entry *Entry = GetEntry(EntryIndex, Table);
            if (ResourceUUIDAreEqual(Entry->UUID, UUID))
            {
                Result = Entry->Handle;
                break;
            }

            EntryIndex = Entry->NextSameHash;
        }
    }

    return Result;
}

static void
InsertResourceReference(resource_uuid UUID, resource_handle Handle, resource_reference_table *Table)
{
    if (Table)
    {
        uint32_t *HashSlot  = GetSlotPointer(UUID, Table);
        uint32_t  FreeEntry = PopFreeEntry(Table);

        if (FreeEntry != INVALID_RESOURCE_ENTRY)
        {
            resource_reference_entry *Entry = GetEntry(FreeEntry, Table);
            Entry->UUID         = UUID;
            Entry->NextSameHash = *HashSlot;
            Entry->Handle       = Handle;

            *HashSlot  = FreeEntry;
        }
    }
}


// Maybe expose this with params?? Same params as the resource_manager basically.

resource_reference_table *
CreateResourceReferenceTable(memory_arena *Arena)
{
    resource_reference_table *Table = PushStruct(Arena, resource_reference_table);

    if (Table)
    {
        Table->HashCount = 32;
        Table->HashMask = 32 - 1;
        Table->EntryCount = MAX_RENDERER_RESOURCE;
        Table->FirstFreeEntry = 0;

        for (uint32_t HashSlot = 0; HashSlot < Table->HashCount; ++HashSlot)
        {
            Table->HashTable[HashSlot] = INVALID_RESOURCE_ENTRY;
        }

        for (uint32_t EntryIdx = 0; EntryIdx < Table->EntryCount; ++EntryIdx)
        {
            Table->Entries[EntryIdx].Handle = (resource_handle){ .Type = RendererResource_None, .Value = INVALID_RESOURCE_HANDLE };
            Table->Entries[EntryIdx].NextSameHash = EntryIdx < Table->EntryCount - 1 ? EntryIdx + 1 : INVALID_RESOURCE_ENTRY;
            Table->Entries[EntryIdx].UUID = (resource_uuid){ .Value = 0 };
        }
    }

    return Table;
}


// TODO: Should we allow that?
resource_handle
CreateResourceHandle(resource_uuid UUID, RendererResource_Type Type, renderer_resource_manager *ResourceManager)
{
    resource_handle Result = { 0 };

    if (Type != RendererResource_None && ResourceManager)
    {
        renderer_resource *Resource = ResourceManager->Resources + ResourceManager->FirstFree;
        Resource->RefCount = 0;
        Resource->Type = Type;
        Resource->UUID = UUID;
        Resource->NextSameType = ResourceManager->FirstByType[Type];

        Result.Value = ResourceManager->FirstFree;
        Result.Type = Type;

        ResourceManager->CountByType[Type] += 1;
        ResourceManager->FirstByType[Type] = ResourceManager->FirstFree;
        ResourceManager->FirstFree = Resource->NextFree;
        Resource->NextFree = INVALID_LINK_SENTINEL;
    }

    return Result;
}


// TODO: Don't think this is useful outside of internal code?
bool
IsValidResourceHandle(resource_handle Handle)
{
    bool Result = Handle.Value != INVALID_RESOURCE_HANDLE && Handle.Type != RendererResource_None;
    return Result;
}


// TODO: I still don't know about this. Is there a simpler way?
resource_handle
BindResourceHandle(resource_handle Handle, renderer_resource_manager *ResourceManager)
{
    if (IsValidResourceHandle(Handle))
    {
        renderer_resource *Resource = GetRendererResource(Handle.Value, ResourceManager);
        assert(Resource);

        ++Resource->RefCount;
    }

    return Handle;
}

// TODO: I still don't know about this. Is there a simpler way?
resource_handle
UnbindResourceHandle(resource_handle Handle, renderer_resource_manager *ResourceManager)
{
    if (IsValidResourceHandle(Handle))
    {
        renderer_resource *Resource = GetRendererResource(Handle.Value, ResourceManager);
        assert(Resource);
        assert(Resource->RefCount > 0);

        --Resource->RefCount;

        if (Resource->RefCount == 0)
        {
            // TODO: Free the resource...
        }
    }

    resource_handle InvalidHandle =
    {
        .Type = RendererResource_None,
        .Value = INVALID_RESOURCE_HANDLE,
    };

    return InvalidHandle;
}


// TODO: Change this to specific queries?
void *
AccessUnderlyingResource(resource_handle Handle, renderer_resource_manager *ResourceManager)
{
    void *Result = 0;

    if (IsValidResourceHandle(Handle))
    {
        renderer_resource *Resource = GetRendererResource(Handle.Value, ResourceManager);
        assert(Resource);

        switch (Handle.Type)
        {

        case RendererResource_Texture2D:
        case RendererResource_TextureView:
        {
            Result = &Resource->Backend;
        } break;

        case RendererResource_VertexBuffer:
        {
            Result = &Resource->Buffer;
        } break;

        case RendererResource_Material:
        {
            Result = &Resource->Material;
        } break;

        case RendererResource_StaticMesh:
        {
            Result = &Resource->StaticMesh;
        } break;

        default:
        {
            assert(!"INVALID ENGINE STATE");
        } break;

        }
    }

    return Result;
}


renderer_resource_manager *
CreateResourceManager(memory_arena *Arena)
{
    renderer_resource_manager *ResourceManager = PushStruct(Arena, renderer_resource_manager);

    if (ResourceManager)
    {
        uint32_t ResourceCount = ArrayCount(ResourceManager->Resources);

        for (uint32_t ResourceIdx = 0; ResourceIdx < ResourceCount; ++ResourceIdx)
        {
            renderer_resource *Resource = ResourceManager->Resources + ResourceIdx;

            Resource->Type = RendererResource_None;
            Resource->RefCount = 0;

            if (ResourceIdx < ResourceCount - 1)
            {
                Resource->NextFree = ResourceIdx + 1;
                Resource->NextSameType = INVALID_LINK_SENTINEL;
            }
            else
            {
                Resource->NextFree = INVALID_LINK_SENTINEL;
                Resource->NextSameType = INVALID_LINK_SENTINEL;
            }
        }

        ResourceManager->FirstFree = 0;

        for (uint32_t ResourceType = RendererResource_Texture2D; ResourceType < RendererResource_Count; ++ResourceType)
        {
            ResourceManager->FirstByType[ResourceType] = INVALID_LINK_SENTINEL;
        }
    }

    return ResourceManager;
}


// =====================================================
// [SECTION] DEFAULT RESOURCES
// =====================================================


resource_handle
GetDefaultMaterial(renderer *Renderer, memory_arena *Arena)
{
	if(!Renderer || !Arena)
	{
		return MakeInvalidResourceHandle();
	}

    resource_uuid   MaterialUUID   = MakeResourceUUID(ByteStringLiteral("default::material"));
    resource_handle MaterialHandle = SearchResourceByUUID(MaterialUUID, Renderer->ReferenceTable);

    if (!IsValidResourceHandle(MaterialHandle))
    {
        MaterialHandle = CreateResourceHandle(MaterialUUID, RendererResource_Material, Renderer->Resources);
        assert(IsValidResourceHandle(MaterialHandle));

        InsertResourceReference(MaterialUUID, MaterialHandle, Renderer->ReferenceTable);

        // TODO: Simplify this code path.

        uint32_t Width         = 1024;
        uint32_t Height        = 1024;
        uint32_t BytesPerPixel = 4;
        uint8_t *Data          = PushArray(Arena, uint8_t, Width * Height * BytesPerPixel);

        if (!Data)
        {
            return MakeInvalidResourceHandle();
        }
        
        // We don't need this stupid type.
        loaded_texture Diffuse =
        {
            .BytesPerPixel = BytesPerPixel,
            .Width         = Width,
            .Height        = Height,
            .Path          = ByteStringLiteral("default::material::diffuse"),
            .Data          = Data,
        };
        
        for (uint32_t Idx = 0; Idx < Width * Height * BytesPerPixel; ++Idx)
        {
            Data[Idx] = 255;
        }

        resource_uuid   TextureUUID   = MakeResourceUUID(ByteStringLiteral("default::material::diffuse"));
        resource_handle TextureHandle = SearchResourceByUUID(TextureUUID, Renderer->ReferenceTable);

        if (!IsValidResourceHandle(TextureHandle))
        {
            TextureHandle = CreateResourceHandle(TextureUUID, RendererResource_TextureView, Renderer->Resources);
            assert(IsValidResourceHandle(TextureHandle));

            InsertResourceReference(TextureUUID, TextureHandle, Renderer->ReferenceTable);

            renderer_backend_resource *BackendResource = AccessUnderlyingResource(TextureHandle, Renderer->Resources);
            BackendResource->Data = RendererCreateTexture(Diffuse, Renderer);

            renderer_material *Material = AccessUnderlyingResource(MaterialHandle, Renderer->Resources);
            Material->Maps[MaterialMap_Albedo] = BindResourceHandle(TextureHandle, Renderer->Resources);
        }
    }

    return MaterialHandle;
}


// =====================================================
// [SECTION] RESOURCE QUERIES
// [HISTORY]
//  -2/7/2026: VertexBuffer Update Call & Basic Getters
// =====================================================


static resource_handle
GetVertexBufferHandle(byte_string Name, memory_arena *Arena, renderer *Renderer)
{
    if (!IsValidByteString(Name) || !Arena || !Renderer)
    {
        return MakeInvalidResourceHandle();
    }

    byte_string     BufferNameParts[2] = { Name, ByteStringLiteral("geometry") };
    byte_string     BufferName = ConcatenateStrings(BufferNameParts, 2, ByteStringLiteral("::"), Arena);
    resource_uuid   BufferUUID = MakeResourceUUID(BufferName);
    resource_handle BufferHandle = SearchResourceByUUID(BufferUUID, Renderer->ReferenceTable);

    if (!IsValidResourceHandle(BufferHandle))
    {
        BufferHandle = CreateResourceHandle(BufferUUID, RendererResource_VertexBuffer, Renderer->Resources);
        assert(IsValidResourceHandle(BufferHandle));

        InsertResourceReference(BufferUUID, BufferHandle, Renderer->ReferenceTable);
    }

    return BufferHandle;
}


renderer_buffer *
GetRendererBufferFromHandle(resource_handle Handle, renderer_resource_manager *ResourceManager)
{
    renderer_buffer *Result = 0;

    if (IsValidResourceHandle(Handle) && ResourceManager)
    {
        renderer_resource *Resource = GetRendererResource(Handle.Value, ResourceManager);
        if (Resource && Resource->Type == RendererResource_VertexBuffer)
        {
            Result = &Resource->Buffer;
        }
    }

    return Result;
}


resource_handle
UpdateVertexBuffer(byte_string BufferName, void *Data, uint64_t Size, memory_arena *Arena, renderer *Renderer)
{
    if (!Arena || !Renderer || !IsValidByteString(BufferName))
    {
        return MakeInvalidResourceHandle();
    }

    resource_handle  BufferHandle = GetVertexBufferHandle(BufferName, Arena, Renderer);
    renderer_buffer *VertexBuffer = AccessUnderlyingResource(BufferHandle, Renderer->Resources);

    if (!VertexBuffer->Backend)
    {
        VertexBuffer->Backend = RendererCreateVertexBuffer(Data, Size, Renderer);
        VertexBuffer->Size    = Size;
    }
    else
    {
        assert(!"NOT IMPLEMENTED");
        // Otherwise we have to update the buffer. What if the size differs?
    }

    return BufferHandle;
}