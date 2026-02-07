#include <stdint.h>

#include "utilities.h"
#include "engine/math/vector.h"
#include "engine/rendering/renderer.h"
#include "engine/rendering/resources.h"

#define CHUNK_SIZE_X 16
#define CHUNK_SIZE_Y 16

typedef struct
{
	uint8_t Data;
} tile;

typedef struct
{
	tile             Tiles[CHUNK_SIZE_X * CHUNK_SIZE_Y];
	uint16_t         SizeX;
	uint16_t         SizeY;

	resource_handle  VertexBuffer;
	uint32_t         VertexCount;
	tile_vertex_data VertexData[CHUNK_SIZE_X * CHUNK_SIZE_Y];

	vec3             Origin;
} chunk;


// TODO:
// Mesh-Instance is probably not what we want to push?

chunk CreateChunk(renderer *Renderer, memory_arena *Arena)
{
	chunk Result =
	{
		.SizeX  = CHUNK_SIZE_X,
		.SizeY  = CHUNK_SIZE_Y,
		.Origin = Vec3(0.0f, 0.0f, 0.0f),
	};

	for (uint32_t X = 0; X < Result.SizeX; ++X)
	{
		for (uint32_t Y = 0; Y < Result.SizeY; ++Y)
		{
		}
	}

	// THIS API IS SOMETHING ELSE 
	{
		byte_string     VertexBufferNameParts[2] = {ByteStringLiteral("my_first_chunk"), ByteStringLiteral("geometry")};
		byte_string     VertexBufferResourceName = ConcatenateStrings(VertexBufferNameParts, 2, ByteStringLiteral("::"), Arena);
		resource_uuid   VertexBufferUUID         = MakeResourceUUID(VertexBufferResourceName);
		resource_handle VertexBufferHandle       = CreateResourceHandle(VertexBufferUUID, RendererResource_VertexBuffer, Renderer->Resources);

		renderer_backend_resource *VertexBuffer     = AccessUnderlyingResource(VertexBufferHandle, Renderer->Resources);
		uint64_t                   VertexBufferSize = Result.SizeX * Result.SizeY * sizeof(mesh_instance);

		VertexBuffer->Data  = RendererCreateVertexBuffer(Result.VertexData, VertexBufferSize, Renderer);
		Result.VertexBuffer = BindResourceHandle(VertexBufferHandle, Renderer->Resources);
	}

	// You probably want a chunk-pass? Because you want to push chunk specific stuff?
	// Like parameters and all of that? We want to draw the full buffer, so a chunk pass would be asking for a buffer probably.
	// Also, I don't know if materials are even a thing anymore? Maybe we still draw 3D stuff? Or rather, maybe we still use materials
	// instead of atlas sampling? Just a fun thing to do?

	// So create a chunk pass that takes in a bunch of stuff, right now it's simply give me some geometry buffer and
	// how much vertices there are or whatever. Then just blit that buffer... And the camera goes in the same parameters slot.
	// What is the data inside the chunk though? Raw vertices? Uhm. So

}