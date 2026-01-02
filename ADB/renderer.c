#include <stdint.h>

#include "utilities.h" // Arenas

typedef enum
{
    RenderPass_None = 0,
    RenderPass_UI   = 1,
    RenderPass_Mesh = 2,
} RenderPassType;


// Data Batches

typedef struct
{
    uint8_t  *Memory;
    uint64_t  ByteCount;
    uint64_t  ByteCapacity;
} render_batch;

typedef struct render_batch_node
{
    struct render_batch_node *Next;
    render_batch              Value;
} render_batch_node;

typedef struct
{
    render_batch_node *First;
    render_batch_node *Last;
    uint64_t           BatchCount;
    uint64_t           ByteCount;
    uint64_t           BytesPerInstance;
} render_batch_list;


// Maps to constant buffers.

typedef struct
{
    void *Data;
} ui_group_params;

typedef struct ui_group_node
{
    struct ui_group_node *Next;
    render_batch_list       BatchList;
    ui_group_params       Params;
} ui_group_node;

typedef struct
{
    ui_group_node *First;
    ui_group_node *Last;
    uint32_t       Count;
} render_pass_params_ui;

typedef struct
{
    RenderPassType Type;
    union
    {
        render_pass_params_ui UI;
    } Params;
} render_pass;

typedef struct render_pass_node
{
    struct render_pass_node *Next;
    render_pass              Value;
} render_pass_node;

typedef struct
{
    render_pass_node *First;
    render_pass_node *Last;
} render_pass_list;

void *
PushDataInBatchList(memory_arena *Arena, render_batch_list *BatchList)
{
    void *Result = NULL;

    render_batch_node *Node = BatchList->Last;
    if (!Node || (Node->Value.ByteCount + BatchList->BytesPerInstance > Node->Value.ByteCapacity))
    {
        Node = PushStruct(Arena, render_batch_node);
        if (Node)
        {
            Node->Next = NULL;
            Node->Value.ByteCount    = 0;
            Node->Value.ByteCapacity = KiB(64); // Still an issue!
            Node->Value.Memory       = PushArray(Arena, uint8_t, Node->Value.ByteCapacity);

            if (!BatchList->First)
            {
                BatchList->First = Node;
                BatchList->Last  = Node;
            }
            else
            {
                BatchList->Last->Next = Node; // Warning but the logic should be sound?
                BatchList->Last = Node;
            }
        }
    }

    BatchList->ByteCount += BatchList->BytesPerInstance;
    BatchList->BatchCount += 1;

    if (Node)
    {
        Result = (void *)(Node->Value.Memory + Node->Value.ByteCount);
        Node->Value.ByteCount += BatchList->BytesPerInstance;
    }

    return Result;
}

render_pass *
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
        Result->Value.Params.UI.First = NULL;
        Result->Value.Params.UI.Last = NULL;
        Result->Value.Params.UI.Count = 0;
        Result->Next = NULL;

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

    return &Result->Value;
}
