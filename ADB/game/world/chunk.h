#pragma once

#include <engine/math/vector.h>

#include "engine/rendering/resources.h"
#include "engine/rendering/renderer_internal.h"


typedef struct camera       camera;
typedef struct renderer     renderer;
typedef struct memory_arena memory_arena;


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

	resource_handle  Material;
	resource_handle  VertexBuffer;
	uint32_t         VertexCount;

	vec3             Origin;
} chunk;

chunk CreateChunk(renderer *Renderer, memory_arena *Arena);
void DrawChunk(camera *Camera, renderer *Renderer, memory_arena *Arena, chunk *Chunk);