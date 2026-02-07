#include <assert.h>

#include "third_party/gui/gui2.h"

#include "engine.h"
#include "platform/platform.h"
#include "rendering/renderer.h"
#include "rendering/resources.h"
#include "rendering/draw.h"
#include "math/vector.h"
#include <stdint.h>


// TODO: Clear the renderer API. Just remove all the bullshit and get something basic working.
// We need a simpler API maybe?

void
UpdateEngine(int WindowWidth, int WindowHeight, gui_input_queue *InputQueue, renderer *Renderer, engine_memory *EngineMemory)
{
    RendererEnterFrame((clear_color) { .R = 0.f, .G = 0.f, .B = 0.f, .A = 1.f }, Renderer);

	{
		camera Camera = CreateCamera(Vec3(0.0f, 0.0f, -5.0f), 60.0f, (float)WindowWidth / (float)WindowHeight);

		gizmo_group_params Params =
		{
			.WorldMatrix      = GetCameraWorldMatrix(&Camera),
			.ViewMatrix       = GetCameraViewMatrix(&Camera),
			.ProjectionMatrix = GetCameraProjectionMatrix(&Camera),
		};

		render_batch_list *BatchList = PushGizmoGroupParams(&Params, EngineMemory->FrameMemory, &Renderer->PassList);
		DrawGizmoCell(Vec3(0.0f, 0.0f, 0.0f), Vec3(1.0f, 1.0f, 1.0f), BatchList, EngineMemory->FrameMemory);
		DrawGizmoCell(Vec3(1.0f, 1.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f), BatchList, EngineMemory->FrameMemory);
		DrawGizmoCell(Vec3(2.0f, 2.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f), BatchList, EngineMemory->FrameMemory);
	}


	RendererLeaveFrame(WindowWidth, WindowHeight, EngineMemory, Renderer);
} 