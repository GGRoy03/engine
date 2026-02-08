#include "renderer_internal.h"

#include <assert.h>
#include <string.h>
#include <stdbool.h>

// ==============================================
// Drawing & Batching
// ==============================================

//
//typedef enum RenderPass_Type
//{
//    RenderPass_None = 0,
//    RenderPass_Mesh = 1,
//    RenderPass_UI = 2,
//    RenderPass_Gizmo = 3,
//    RenderPass_Chunk = 4,
//} RenderPass_Type;
//
//
//typedef struct
//{
//    uint8_t *Memory;
//    uint64_t ByteCount;
//    uint64_t ByteCapacity;
//} render_batch;
//
//
//typedef struct
//{
//    resource_handle Material;
//} mesh_batch_params;
//
//
//typedef struct
//{
//    void *Data;
//} ui_batch_params;
//
//
//typedef struct render_batch_node render_batch_node;
//struct render_batch_node
//{
//    render_batch_node *Next;
//    render_batch       Value;
//
//    union
//    {
//        mesh_batch_params MeshParams;
//        ui_batch_params   UIParams;
//    };
//};
//
//
//typedef struct render_batch_list
//{
//    render_batch_node *First;
//    render_batch_node *Last;
//    uint64_t           BatchCount;
//    uint64_t           BytesPerInstance;
//} render_batch_list;
//
//
//typedef struct
//{
//    mat4x4 WorldMatrix;
//    mat4x4 ViewMatrix;
//    mat4x4 ProjectionMatrix;
//} mesh_group_params;
//
//
//typedef struct
//{
//    void *Data;
//} ui_group_params;
//
//
//typedef struct
//{
//    mat4x4 WorldMatrix;
//    mat4x4 ViewMatrix;
//    mat4x4 ProjectionMatrix;
//} gizmo_group_params;
//
//
//typedef struct
//{
//    mat4x4 WorldMatrix;
//    mat4x4 ViewMatrix;
//    mat4x4 ProjectionMatrix;
//} chunk_group_params;
//
//
//typedef struct render_group_node render_group_node;
//struct render_group_node
//{
//    render_group_node *Next;
//    render_batch_list  BatchList;
//
//    union
//    {
//        mesh_group_params  Mesh;
//        ui_group_params    UI;
//        gizmo_group_params Gizmo;
//        chunk_group_params Chunk;
//    } Params;
//};
//
//
//typedef struct
//{
//    RenderPass_Type    Type;
//    render_group_node *First;
//    render_group_node *Last;
//} render_pass;
//
//
//typedef struct render_pass_node render_pass_node;
//struct render_pass_node
//{
//    render_pass_node *Next;
//    render_pass       Value;
//};
//
//
//typedef struct render_pass_list
//{
//    render_pass_node *First;
//    render_pass_node *Last;
//} render_pass_list;


static render_pass *
GetRenderPass(memory_arena *Arena, RenderPass_Type Type, render_pass_list *PassList)
{
    render_pass_node *Result = PassList->Last;

    if (!Result || Result->Value.Type != Type)
    {
        Result = PushStruct(Arena, render_pass_node);
        if (!Result)
        {
            return NULL;
        }


        Result->Next        = NULL;
        Result->Value.Type  = Type;
        Result->Value.First = 0;
        Result->Value.Last  = 0;

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
        Result->Value.ByteCount = 0;
        Result->Value.ByteCapacity = InstancePerBatch * BatchList->BytesPerInstance;
        Result->Value.Memory = PushArray(Arena, uint8_t, Result->Value.ByteCapacity);

        if (!BatchList->First)
        {
            BatchList->First = Result;
            BatchList->Last = Result;
        }
        else
        {
            BatchList->Last->Next = Result;
            BatchList->Last = Result;
        }
    }

    return Result;
}


// TODO: I think we might need safeguards here. Because we could push an instance count higher than the instance per batch
// which would be a OOB write.

void *
PushDataInBatchList(memory_arena *Arena, render_batch_list *BatchList, uint32_t InstanceCount, uint64_t InstancePerBatch)
{
    void *Result = 0;

    render_batch_node *Node = BatchList->Last;
    uint64_t           PushedDataSize = InstanceCount * BatchList->BytesPerInstance;

    if (!Node || Node->Value.ByteCount + PushedDataSize > Node->Value.ByteCapacity)
    {
        render_batch_node *NewNode = AppendNewRenderBatch(Arena, BatchList, InstancePerBatch);

        if (NewNode && Node)
        {
            NewNode->Params.Mesh = Node->Params.Mesh;
        }

        Node = NewNode;
    }

    if (Node)
    {
        Result = Node->Value.Memory + Node->Value.ByteCount;

        Node->Value.ByteCount += InstanceCount * BatchList->BytesPerInstance;
    }

    return Result;
}

static render_group_node *
GetGroupNode(bool ParametersDoNotMatch, memory_arena *Arena, render_pass *Pass, render_pass_list *PassList)
{
    assert(Arena);
    assert(Pass);
    assert(PassList);

    render_group_node *Result = Pass->Last;

    if (!Result || ParametersDoNotMatch)
    {
        Result = PushStruct(Arena, render_group_node);
        Result->Next                 = 0;
        Result->BatchList.BatchCount = 0;
        Result->BatchList.First      = 0;
        Result->BatchList.Last       = 0;

        if (!Pass->First)
        {
            Pass->First = Result;
            Pass->Last  = Result;
        }
        else if (Pass->Last)
        {
            Pass->Last->Next = Result;
            Pass->Last       = Result;
        }
        else
        {
            assert(!"INVALID ENGINE STATE");
        }
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
        bool               SameParameters = Pass->Last ? (memcmp(&Pass->Last->Params.Mesh, Params, sizeof(mesh_group_params)) == 0) : false;
        render_group_node *GroupNode      = GetGroupNode(SameParameters, Arena, Pass, PassList);

        GroupNode->BatchList.BytesPerInstance = sizeof(mesh_instance);

        Result = &GroupNode->BatchList;
    }

    return Result;
}


void
PushMeshBatchParams(mesh_batch_params *Params, uint64_t InstancePerBatch, memory_arena *Arena, render_batch_list *BatchList)
{
    render_batch_node *Batch = BatchList->Last;
    if (!Batch || memcmp(&Batch->Params.Mesh, Params, sizeof(mesh_batch_params) != 0))
    {
        Batch = AppendNewRenderBatch(Arena, BatchList, InstancePerBatch);
        if (Batch)
        {
            Batch->Params.Mesh = *Params;
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
        bool               SameParameters = Pass->Last ? (memcmp(&Pass->Last->Params.UI, Params, sizeof(ui_group_params)) == 0) : false;
        render_group_node *GroupNode      = GetGroupNode(SameParameters, Arena, Pass, PassList);

        GroupNode->BatchList.BytesPerInstance = sizeof(ui_vertex_data);
        GroupNode->Params.UI                  = *Params;

        Result = &GroupNode->BatchList;
    }

    return Result;
}

void
PushUIBatchParams(ui_batch_params *Params, uint64_t InstancePerBatch, memory_arena *Arena, render_batch_list *BatchList)
{
    render_batch_node *Batch = BatchList->Last;

    if (!Batch || memcmp(&Batch->Params.UI, Params, sizeof(mesh_batch_params) != 0))
    {
        Batch = AppendNewRenderBatch(Arena, BatchList, InstancePerBatch);
        if (Batch)
        {
            Batch->Params.UI = *Params;
        }
    }
}


render_batch_list *
PushGizmoGroupParams(gizmo_group_params *Params, memory_arena *Arena, render_pass_list *PassList)
{
    render_batch_list *Result = 0;

    render_pass *Pass = GetRenderPass(Arena, RenderPass_Gizmo, PassList);
    if (Pass)
    {
        bool               SameParameters = Pass->Last ? (memcmp(&Pass->Last->Params.Gizmo, Params, sizeof(gizmo_group_params)) == 0) : false;
        render_group_node *GroupNode      = GetGroupNode(SameParameters, Arena, Pass, PassList);

        GroupNode->BatchList.BytesPerInstance = sizeof(gizmo_vertex_data);
        GroupNode->Params.Gizmo               = *Params;

        Result = &GroupNode->BatchList;
    }

    return Result;
}


render_batch_list *
PushChunkGroupParams(chunk_group_params *Params, memory_arena *Arena, render_pass_list *PassList)
{
    render_batch_list *Result = 0;

    render_pass *Pass = GetRenderPass(Arena, RenderPass_Chunk, PassList);
    if (Pass)
    {
        bool               SameParameters = Pass->Last ? (memcmp(&Pass->Last->Params.Chunk, Params, sizeof(chunk_group_params) == 0)) : false;
        render_group_node *GroupNode      = GetGroupNode(SameParameters, Arena, Pass, PassList);

        GroupNode->BatchList.BytesPerInstance = 0;
        GroupNode->Params.Chunk               = *Params;

        Result = &GroupNode->BatchList;
    }

    return Result;
}

void
PushChunkBatchParams(chunk_batch_params *Params, uint64_t InstancePerBatch, memory_arena *Arena, render_batch_list *BatchList)
{
    render_batch_node *Batch = BatchList->Last;

    if (!Batch || memcmp(&Batch->Params.Chunk, Params, sizeof(chunk_batch_params) != 0))
    {
        Batch = AppendNewRenderBatch(Arena, BatchList, InstancePerBatch);
        if (Batch)
        {
            Batch->Params.Chunk = *Params;
        }
    }
}