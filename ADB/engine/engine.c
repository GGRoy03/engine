#include <assert.h>

#include "third_party/gui/gui2.h"

#include "engine.h"
#include "platform/platform.h"
#include "rendering/renderer.h"
#include "rendering/draw.h"
#include "math/vector.h"
#include "game/world/chunk.h"


// TODO: Clear the renderer API. Just remove all the bullshit and get something basic working.
// We need a simpler API maybe?

void
UpdateEngine(int WindowWidth, int WindowHeight, gui_input_queue *InputQueue, renderer *Renderer, engine_memory *EngineMemory)
{
    RendererEnterFrame((clear_color) { .R = 0.f, .G = 0.f, .B = 0.f, .A = 1.f }, Renderer);

	{
		camera Camera = CreateCamera(Vec3(0.0f, 0.0f, -10.0f), 60.0f, (float)WindowWidth / (float)WindowHeight);

		//for (float X = -5; X < 5; ++X)
		//{
		//	for (float Y = -5; Y < 5; ++Y)
		//	{
		//		DrawGizmoCell(Vec3(X, Y, 0.0f), Vec3(1.0f, 1.0f, 1.0f), &Camera, Renderer, EngineMemory->FrameMemory);
		//	}
		//}

		static bool  FirstFrame = true;
		static chunk Chunk     = {0};
		if (FirstFrame)
		{
			Chunk = CreateChunk(Renderer, EngineMemory->FrameMemory);

			FirstFrame = false;
		}

		DrawChunk(&Camera, Renderer, EngineMemory->FrameMemory, &Chunk);
	}


	RendererLeaveFrame(WindowWidth, WindowHeight, EngineMemory, Renderer);
} 