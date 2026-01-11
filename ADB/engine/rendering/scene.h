#pragma once

#define MAX_ENTITY_COUNT 64



typedef struct
{
	resource_handle MeshHandle;
	bool            IsAlive;
	vec3            Position;
} game_entity;


typedef struct
{
	camera       Camera;
	game_entity  Entities[MAX_ENTITY_COUNT];
	uint32_t     EntityCount;
	light_source Lights[MAX_LIGHT_COUNT];
	uint32_t     LightCount;
} game_scene;

game_entity * CreateGameEntity  (byte_string MeshResource, game_scene *Scene, renderer *Renderer);
void          UpdateScene       (game_scene *Scene, engine_memory *EngineMemory, renderer *Renderer);