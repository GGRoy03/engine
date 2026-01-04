#pragma once

#include <stdint.h>

#include "utilities.h"
#include "engine/math/vector.h"

typedef struct
{
	vec3 Position;
	vec2 Texture;
	vec3 Normal;
} mesh_vertex_data;

typedef struct
{
	float       Shininess;
	float       Opacity;

	byte_string Name;
	byte_string ColorTexture;
	byte_string NormalTexture;
	byte_string RoughnessTexture;
} material_data;

typedef struct
{
	byte_string Name;
	uint32_t    MaterialId;
	uint32_t    VertexCount;
	uint32_t    VertexOffset;
} submesh_data;

typedef struct
{
	mesh_vertex_data *Vertices;
	uint32_t          VertexCount;

	submesh_data     *Submeshes;
	uint32_t          SubmeshCount;

	material_data    *Materials;
	uint32_t          MaterialCount;
} mesh_data;