#include <stdbool.h>

#include "utilities.h"
#include "engine.h"
#include "rendering/renderer.h"
#include "rendering/scene.h"
#include "platform/platform.h"

typedef struct renderer renderer;

typedef struct
{
	bool IsInitialized;
} engine_state;

// Temp
#include "parsers/parser_obj.h"

void
UpdateEngine(int WindowWidth, int WindowHeight, renderer *Renderer, engine_memory *EngineMemory)
{
	static engine_state Engine;
	static game_scene   Scene;

	if (!Engine.IsInitialized)
	{
		asset_file_data AssetData = ParseObjFromFile(ByteStringLiteral("data/strawberry.obj"), EngineMemory);

		LoadAssetFileData(AssetData, EngineMemory->FrameMemory, Renderer);

		Scene.Camera      = CreateCamera(Vec3(0.f, 0.f, -20.f), 3.14159f / 4.f, 1901.f / 1041.f);
		Scene.EntityCount = 0;
		CreateGameEntity(ByteStringLiteral("data/strawberry::strawberry"), &Scene, Renderer);
		

		// TODO: From the asset_file_data we want to initialize the rendering objects. The idea is to construct a static mesh from the mesh_data. Just do a naive implementation.
		// TODO: Render the tree! (Material, Winding Order?, Camera Stuff)

		Engine.IsInitialized = true;
	}

	clear_color Color = (clear_color){.R = 0.f, .G = 0.f, .B = 0.f, .A = 1.f};
	RendererStartFrame(Color, Renderer);

	UpdateScene(&Scene, EngineMemory, Renderer);

	RendererDrawFrame(WindowWidth, WindowHeight, EngineMemory, Renderer);


	RendererFlushFrame(Renderer);
} 