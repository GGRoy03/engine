#include <stdint.h>
#include <assert.h>
#include <math.h>
#include <string.h>

#include "utilities.h"
#include "renderer.h"


// ==============================================
// <Drawing> 
// ==============================================


static render_pass *
GetRenderPass(memory_arena *Arena, RenderPassType Type, render_pass_list *PassList)
{
    render_pass_node *Result = PassList->Last;

    if (!Result || Result->Value.Type != Type)
    {
        Result = PushStruct(Arena, render_pass_node);
        if (!Result)
        {
            return NULL;
        }

        Result->Value.Type = Type;
        Result->Next       = NULL;
        memset(&Result->Value.Params, 0, sizeof(Result->Value.Params));

        if (!PassList->First)
        {
            PassList->First = Result;
        }

        if (PassList->Last)
        {
            PassList->Last->Next = Result;
        }

        PassList->Last = Result;
    }

    assert(Result);

    return &Result->Value;
}


static render_batch_node *
AppendNewRenderBatch(memory_arena *Arena, render_batch_list *BatchList, uint64_t InstancePerBatch)
{
    render_batch_node *Result = PushStruct(Arena, render_batch_node);

    if (Result)
    {
        Result->Next = 0;
        Result->Value.ByteCount    = 0;
        Result->Value.ByteCapacity = InstancePerBatch * BatchList->BytesPerInstance;
        Result->Value.Memory       = PushArray(Arena, uint8_t, Result->Value.ByteCapacity);

        if (!BatchList->First)
        {
            BatchList->First = Result;
            BatchList->Last  = Result;
        }
        else
        {
            BatchList->Last->Next = Result;
            BatchList->Last       = Result;
        }
    }

    return Result;
}



void *
PushDataInBatchList(memory_arena *Arena, render_batch_list *BatchList, uint64_t InstancePerBatch)
{
    void *Result = 0;

    render_batch_node *Node = BatchList->Last;
    if (!Node || Node->Value.ByteCount + BatchList->BytesPerInstance > Node->Value.ByteCapacity)
    {
        Node = AppendNewRenderBatch(Arena, BatchList, InstancePerBatch);
    }

    if (Node)
    {
        Result = Node->Value.Memory + Node->Value.ByteCount;

        Node->Value.ByteCount += BatchList->BytesPerInstance;
    }

    return Result;
}



render_batch_list *
PushMeshGroupParams(mesh_group_params *Params, memory_arena *Arena, render_pass_list *PassList)
{
    render_batch_list *Result = 0;

    render_pass *Pass = GetRenderPass(Arena, RenderPass_Mesh, PassList);
    if (Pass)
    {
        render_pass_params_mesh *PassParams = &Pass->Params.Mesh;

        mesh_group_node *Group = PassParams->Last;
        if (!Group || memcmp(&Group->Params, Params, sizeof(mesh_group_params) != 0))
        {
            Group = PushStruct(Arena, mesh_group_node);
            if (Group)
            {
                Group->Next   = 0;
                Group->Params = *Params;
                Group->BatchList.BatchCount       = 0;
                Group->BatchList.First            = 0;
                Group->BatchList.Last             = 0;
                Group->BatchList.BytesPerInstance = sizeof(mesh_instance);
            }

            if (!PassParams->First)
            {
                PassParams->First = Group;
                PassParams->Last  = Group;
            }
            else if (PassParams->Last)
            {
                PassParams->Last->Next = Group;
                PassParams->Last       = Group;
            }
            else
            {
                assert(!"INVALID ENGINE STATE");
            }
        }
    }

    if (Pass)
    {
        Result = &Pass->Params.Mesh.Last->BatchList;
    }

    return Result;
}


void
PushMeshBatchParams(mesh_batch_params *Params, uint64_t InstancePerBatch, memory_arena *Arena, render_batch_list *BatchList)
{
    render_batch_node *Batch = BatchList->Last;
    if (!Batch || memcmp(&Batch->MeshParams, Params, sizeof(mesh_batch_params) != 0))
    {
        Batch = AppendNewRenderBatch(Arena, BatchList, InstancePerBatch);
        if (Batch)
        {
            Batch->MeshParams = *Params;
        }
    }
}


render_batch_list *
PushUIGroupParams(ui_group_params *Params, memory_arena *Arena, render_pass_list *PassList)
{
    render_batch_list *Result = 0;

    render_pass *Pass = GetRenderPass(Arena, RenderPass_UI, PassList);
    if (Pass)
    {
        render_pass_params_ui *PassParams = &Pass->Params.UI;

        ui_group_node *Group = PassParams->Last;
        if (!Group)
        {
            Group = PushStruct(Arena, ui_group_node);
            if (Group)
            {
                Group->Next   = 0;
                Group->Params = *Params;
                Group->BatchList.BatchCount       = 0;
                Group->BatchList.First            = 0;
                Group->BatchList.Last             = 0;
                Group->BatchList.BytesPerInstance = sizeof(gui_rect);
            }

            if (!PassParams->First)
            {
                PassParams->First = Group;
                PassParams->Last  = Group;
            }
            else if (PassParams->Last)
            {
                PassParams->Last->Next = Group;
                PassParams->Last       = Group;
            }
            else
            {
                assert(!"INVALID ENGINE STATE");
            }
        }
    }

    if (Pass)
    {
        Result = &Pass->Params.UI.Last->BatchList;
    }

    return Result;
}

void
PushUIBatchParams(ui_batch_params *Params, uint64_t InstancePerBatch, memory_arena *Arena, render_batch_list *BatchList)
{
    render_batch_node *Batch = BatchList->Last;

    if (!Batch || memcmp(&Batch->UIParams, Params, sizeof(mesh_batch_params) != 0))
    {
        Batch = AppendNewRenderBatch(Arena, BatchList, InstancePerBatch);
        if (Batch)
        {
            Batch->UIParams = *Params;
        }
    }
}


// ==============================================
// <Camera>
// ==============================================


camera
CreateCamera(vec3 Position, float FovY, float AspectRatio)
{
    camera Result =
    {
        .Position    = Position,
        .Forward     = Vec3(0.f, 0.f, 1.f),
        .Up          = Vec3(0.f, 1.f, 0.f),
        .AspectRatio = AspectRatio,
        .NearPlane   = 0.1f,
        .FarPlane    = 1000.f,
        .FovY        = FovY,
    };

    return Result;
}


mat4x4
GetCameraWorldMatrix(camera *Camera)
{
    (void)Camera;

    mat4x4 World = {0};
    World.c0r0 = 1.f;
    World.c1r1 = 1.f;
    World.c2r2 = 1.f;
    World.c3r3 = 1.f;

    return World;
}


mat4x4
GetCameraViewMatrix(camera *Camera)
{
    mat4x4 View = { 0 };

    vec3 Forward = Vec3Normalize(Camera->Forward);
    vec3 Right   = Vec3Normalize(Vec3Cross(Camera->Up, Forward));
    vec3 Up      = Vec3Cross(Forward, Right);

    Camera->Up = Up;

    View.c0r0 = Right.X;
    View.c0r1 = Right.Y;
    View.c0r2 = Right.Z;
    View.c0r3 = 0.f;

    View.c1r0 = Up.X;
    View.c1r1 = Up.Y;
    View.c1r2 = Up.Z;
    View.c1r3 = 0.f;

    View.c2r0 = Forward.X;
    View.c2r1 = Forward.Y;
    View.c2r2 = Forward.Z;
    View.c2r3 = 0.f;

    View.c3r0 = -Vec3Dot(Right, Camera->Position);
    View.c3r1 = -Vec3Dot(Up, Camera->Position);
    View.c3r2 = -Vec3Dot(Forward, Camera->Position);
    View.c3r3 = 1.f;

    return View;
}


mat4x4
GetCameraProjectionMatrix(camera *Camera)
{
    mat4x4 Projection = {0};

    float F = 1.f / tanf(Camera->FovY * 0.5f);

    Projection.c0r0 = F / Camera->AspectRatio;
    Projection.c1r1 = F;
    Projection.c2r2 = (Camera->FarPlane + Camera->NearPlane) / (Camera->FarPlane - Camera->NearPlane);
    Projection.c2r3 = 1.f;
    Projection.c3r2 = (-2.f * Camera->FarPlane * Camera->NearPlane) / (Camera->FarPlane - Camera->NearPlane);

    return Projection;
}