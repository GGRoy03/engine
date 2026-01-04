#include <stdbool.h>

#include "utilities.h"
#include "engine.h"
#include "rendering/renderer.h"

typedef struct renderer renderer;

typedef struct
{
	bool IsInitialized;
} engine_state;

// Temp
#include "parsers/parser_obj.h"

void UpdateEngine(int WindowWidth, int WindowHeight, renderer *Renderer)
{
	static engine_state Engine;

	if (!Engine.IsInitialized)
	{
		mesh_data MeshData = ParseObjFromFile(ByteStringLiteral("data/strawberry.obj"));

		// TODO: From the mesh_data we want to initialize the rendering objects. The idea is to construct a static mesh from the mesh_data.
		// TODO: Render the tree! (Material, Winding Order?, Camera Stuff)

		Engine.IsInitialized = true;
	}

	clear_color Color = (clear_color){.R = 0.f, .G = 0.f, .B = 0.f, .A = 1.f};
	RendererStartFrame(Color, Renderer);

	RendererFlushFrame(Renderer);
}