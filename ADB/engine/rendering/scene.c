#include <stdbool.h>
#include <assert.h>
#include <string.h>

#include "platform/platform.h"
#include "renderer.h"

#include "scene.h"


game_entity *
CreateGameEntity(byte_string MeshResource, game_scene *Scene, renderer *Renderer)
{
	game_entity *Entity = 0;

	if (IsValidByteString(MeshResource) && Scene)
	{
		// This is actually not the correct path since this will try to create the resource
		// if it does not exist yet. This is fine for prototyping, but should be fixed.

		resource_uuid            MeshUUID  = MakeResourceUUID(MeshResource);
		resource_reference_state MeshState = FindResourceByUUID(MeshUUID, Renderer->ReferenceTable);

		// Obviously this is stupid, but fine for prototyping.
		Entity = &Scene->Entities[0];
		Entity->MeshHandle = BindResourceHandle(MeshState.Handle, Renderer->Resources);
		Entity->IsAlive    = true;

		++Scene->EntityCount;
	}

	return Entity;
}


void
UpdateScene(game_scene *Scene, engine_memory *EngineMemory, renderer *Renderer)
{
	mesh_group_params GroupParams =
	{
		.WorldMatrix      = GetCameraWorldMatrix(&Scene->Camera),
		.ViewMatrix       = GetCameraViewMatrix(&Scene->Camera),
		.ProjectionMatrix = GetCameraProjectionMatrix(&Scene->Camera),
		.CameraPosition   = Scene->Camera.Position,
		.LightCount       = 1,
	};

	// Experimental.
	Scene->Lights[0].Color     = Vec3(1.f, 1.f, 1.f);
	Scene->Lights[0].Intensity = 1.f;
	Scene->Lights[0].Position  = Vec3(0.f, 5.f, -15.f);

	memcpy(GroupParams.Lights, Scene->Lights, 1 * sizeof(light_source));

	render_batch_list *BatchList = PushMeshGroupParams(&GroupParams, EngineMemory->FrameMemory, &Renderer->PassList);

	for (uint32_t Idx = 0; Idx < Scene->EntityCount; ++Idx)
	{
		game_entity *Entity = &Scene->Entities[Idx];

		if (Entity->IsAlive)
		{
			renderer_static_mesh *Mesh = AccessUnderlyingResource(Entity->MeshHandle, Renderer->Resources);
			assert(Mesh);

			for (uint32_t MeshIdx = 0; MeshIdx < Mesh->SubmeshCount; ++MeshIdx)
			{
				mesh_batch_params BatchParams =
				{
					.Material = Mesh->Submeshes[MeshIdx].Material,
				};

				PushMeshBatchParams(&BatchParams, MESH_INSTANCE_PER_BATCH, EngineMemory->FrameMemory, BatchList);

				render_command *Command = PushDataInBatchList(EngineMemory->FrameMemory, BatchList, MESH_INSTANCE_PER_BATCH);
				if (Command)
				{
					Command->Type = RenderCommand_StaticGeometry;
					Command->StaticGeometry.MeshHandle = Entity->MeshHandle;
				}
			}
		}
	}
}