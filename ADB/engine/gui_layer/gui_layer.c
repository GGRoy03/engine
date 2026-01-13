#include "assert.h"


#include "third_party/gui/gui.h"


#include "utilities.h"
#include "engine/rendering/renderer.h"


void
DrawComputedLayoutTree(gui_layout_tree *Tree, memory_arena *Arena, renderer *Renderer)
{
    ui_group_params    GroupParams = { 0 };
    render_batch_list *BatchList   = PushUIGroupParams(&GroupParams, Arena, &Renderer->PassList);

    gui_render_command *CommandBuffer = 0;
    {
        gui_render_command_params Params = {0};
        Params.Count = GuiGetRenderCommandCount(Params, Tree);

        gui_memory_footprint Footprint = GuiGetRenderCommandsFootprint(Params, Tree);
        gui_memory_block     Block     =
        {
            .SizeInBytes = Footprint.SizeInBytes,
            .Base        = PushArena(Arena, Footprint.SizeInBytes, Footprint.Alignment)
        };

        CommandBuffer = GuiComputeRenderCommands(Params, Tree, Block);
    }

    if (CommandBuffer)
    {
        uint32_t CommandIdx = 0;
        for (gui_render_command *Command = &CommandBuffer[CommandIdx++]; Command->Type != Gui_RenderCommandType_End; Command = &CommandBuffer[CommandIdx++])
        {
            switch (Command->Type)
            {
        
            case Gui_RenderCommandType_Rectangle:
            {
                gui_rect *Rect = PushDataInBatchList(Arena, BatchList, UI_INSTANCE_PER_BATCH);
                if (Rect)
                {
                    Rect->Bounds        = Command->Box;
                    Rect->ColorTL       = Command->Rect.Color;
                    Rect->ColorTR       = Command->Rect.Color;
                    Rect->ColorBR       = Command->Rect.Color;
                    Rect->ColorBL       = Command->Rect.Color;
                    Rect->CornerRadius  = Command->Rect.CornerRadius;
                    Rect->BorderWidth   = 0;
                    Rect->Softness      = 2;
                    Rect->SampleTexture = 0;
                    Rect->TextureSource = (gui_bounding_box){ 0 };
                }
            } break;
        
            case Gui_RenderCommandType_Border:
            {
                gui_rect *Rect = PushDataInBatchList(Arena, BatchList, UI_INSTANCE_PER_BATCH);
                if (Rect)
                {
                    Rect->Bounds        = Command->Box;
                    Rect->ColorTL       = Command->Border.Color;
                    Rect->ColorTR       = Command->Border.Color;
                    Rect->ColorBR       = Command->Border.Color;
                    Rect->ColorBL       = Command->Border.Color;
                    Rect->CornerRadius  = Command->Border.CornerRadius;
                    Rect->BorderWidth   = Command->Border.Width;
                    Rect->Softness      = 2;
                    Rect->SampleTexture = 0;
                    Rect->TextureSource = (gui_bounding_box){ 0 };
                }
            } break;
        
            default:
            {
                assert(!"INVALID ENGINE STATE");
            } break;
        
            }
        }
    }
}