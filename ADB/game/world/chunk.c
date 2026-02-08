#include "chunk.h"

#include <stdint.h>

#include "utilities.h"
#include "engine/math/vector.h"
#include "engine/rendering/draw.h"
#include "engine/rendering/resources.h"
#include "engine/rendering/renderer_internal.h"


// TODO: This could be in the draw code, because I doubt we want to handle most of this code here.

static const tile_vertex_data TileQuad[6] =
{
	// Triangle 1
	{.Position = {0, 0, 0}, .UV = {0, 0} },
	{.Position = {1, 0, 0}, .UV = {1, 0} },
	{.Position = {1, 1, 0}, .UV = {1, 1} },

	// Triangle 2
	{.Position = {0, 0, 0}, .UV = {0, 0} },
	{.Position = {1, 1, 0}, .UV = {1, 1} },
	{.Position = {0, 1, 0}, .UV = {0, 1} },
};


static tile *
GetTile(uint32_t X, uint32_t Y, chunk *Chunk)
{
	tile    *Result = 0;
	uint32_t Index  = Y * Chunk->SizeY + X;

	if (Index < Chunk->SizeX * Chunk->SizeY)
	{
		Result = &Chunk->Tiles[Index];
	}


	return Result;
}

static tile_vertex_data *
GetChunkMeshData(chunk *Chunk, memory_arena *Arena)
{
	uint32_t          Count    = 0;
	tile_vertex_data *Vertices = PushArray(Arena, tile_vertex_data, Chunk->SizeX * Chunk->SizeY * 6);

	for (float Y = 0; Y < Chunk->SizeY; ++Y)
	{
		for (float X = 0; X < Chunk->SizeX; ++X)
		{
			vec3 Offset = Vec3(X, Y, 0);

			for (uint32_t Vertex = 0; Vertex < 6; ++Vertex)
			{
				Vertices[Count]          = TileQuad[Vertex];
				Vertices[Count].Position = Vec3Add(Vertices[Count].Position, Offset);

				++Count;
			}
		}
	}

	Chunk->VertexCount = Count;

	return Vertices;
}


chunk CreateChunk(renderer *Renderer, memory_arena *Arena)
{
	chunk Chunk =
	{
		.SizeX    = CHUNK_SIZE_X,
		.SizeY    = CHUNK_SIZE_Y,
		.Material = GetDefaultMaterial(Renderer, Arena),
	};


	tile_vertex_data *VertexData     = GetChunkMeshData(&Chunk, Arena);
	uint64_t          VertexDataSize = Chunk.SizeX * Chunk.SizeY * 6 * sizeof(tile_vertex_data);

	resource_handle VertexBuffer = UpdateVertexBuffer(ByteStringLiteral("chunk_geometry"), VertexData, VertexDataSize, Arena, Renderer);
	Chunk.VertexBuffer = BindResourceHandle(VertexBuffer, Renderer->Resources);

	return Chunk;
}


void DrawChunk(camera *Camera, renderer *Renderer, memory_arena *Arena, chunk *Chunk)
{
	DrawChunkIntance(Chunk->VertexBuffer, Chunk->VertexCount, Chunk->Material, Camera, Renderer, Arena);
}