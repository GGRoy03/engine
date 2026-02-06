#include <assert.h>

#include "third_party/gui/gui2.h"

#include "engine.h"
#include "platform/platform.h"
#include "rendering/renderer.h"
#include "rendering/resources.h"


// TODO: Clear the renderer API. Just remove all the bullshit and get something basic working.
// We need a simpler API maybe?

void
UpdateEngine(int WindowWidth, int WindowHeight, gui_input_queue *InputQueue, renderer *Renderer, engine_memory *EngineMemory)
{
    RendererEnterFrame((clear_color) { .R = 0.f, .G = 0.f, .B = 0.f, .A = 1.f }, Renderer);

	// Just testing the 3D path so we can start working
	{
		camera Camera = CreateCamera(Vec3(0.0f, 0.0f, -0.2f), 60.0f, (float)WindowWidth / (float)WindowHeight);

		mesh_group_params GroupParams =
		{
			.WorldMatrix      = GetCameraWorldMatrix(&Camera),
			.ViewMatrix       = GetCameraViewMatrix(&Camera),
			.ProjectionMatrix = GetCameraProjectionMatrix(&Camera),
			.CameraPosition   = Camera.Position,
			.LightCount       = 0,
		};

		render_batch_list *BatchList   = PushMeshGroupParams(&GroupParams, EngineMemory->FrameMemory, &Renderer->PassList);
		mesh_batch_params  BatchParams =
		{
			.Material = GetBuiltinMaterial(Renderer, EngineMemory->FrameMemory),
		};

		PushMeshBatchParams(&BatchParams, MESH_INSTANCE_PER_BATCH, EngineMemory->FrameMemory, BatchList);

		mesh_instance *Instance = PushDataInBatchList(EngineMemory->FrameMemory, BatchList, MESH_INSTANCE_PER_BATCH);
		Instance->MeshHandle   = GetBuiltinQuadMesh(Renderer, EngineMemory->FrameMemory);
		Instance->Transform    = Vec3(0.0f, 0.0f, 0.0f);
		Instance->SubmeshIndex = 0;
	}


	RendererLeaveFrame(WindowWidth, WindowHeight, EngineMemory, Renderer);
} 