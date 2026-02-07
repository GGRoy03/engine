#include "assert.h"


#include "third_party/gui/gui3.h"


#include "utilities.h"
#include "engine/rendering/renderer.h"

#include "gui_layer.h"


static bounding_box
BoundingBoxFromCommand(gui_render_command *Command)
{
    bounding_box Result =
    {
        .Left   = Command->PosX,
        .Top    = Command->PosY,
        .Right  = Command->PosX + Command->Width,
        .Bottom = Command->PosY + Command->Height,
    };

    return Result;
}

static color
RGBAToNormalized(uint32_t Color)
{
    color Result =
    {
        .R = (Color >> 24 & 0xFF) / 255.f,
        .G = (Color >> 16 & 0xFF) / 255.f,
        .B = (Color >> 8  & 0xFF) / 255.f,
        .A = (Color       & 0xFF) / 255.f,
    };

    return Result;
}


void
DrawComputedLayoutTree(gui_render_command *CommandBuffer, uint32_t CommandBufferSize, memory_arena *Arena, renderer *Renderer)
{
    ui_group_params    GroupParams = {0};
    render_batch_list *BatchList   = PushUIGroupParams(&GroupParams, Arena, &Renderer->PassList);

    if (CommandBuffer)
    {
        for (uint32_t CommandIdx = 0; CommandIdx < CommandBufferSize; ++CommandIdx)
        {
            gui_render_command *Command = &CommandBuffer[CommandIdx];

            switch (Command->Type)
            {
        
            case Gui_RenderCommand_Rectangle:
            {
                gui_rect *Rect = PushDataInBatchList(Arena, BatchList, 1, UI_INSTANCE_PER_BATCH);
                if (Rect)
                {
                    Rect->Bounds        = BoundingBoxFromCommand(Command);
                    Rect->ColorTL       = RGBAToNormalized(Command->Rectangle.Color);
                    Rect->ColorTR       = RGBAToNormalized(Command->Rectangle.Color);
                    Rect->ColorBR       = RGBAToNormalized(Command->Rectangle.Color);
                    Rect->ColorBL       = RGBAToNormalized(Command->Rectangle.Color);
                    Rect->CornerRadius  = (bounding_box){2, 2, 2, 2};
                    Rect->BorderWidth   = 0;
                    Rect->Softness      = 1;
                    Rect->SampleTexture = 0;
                    Rect->TextureSource = (bounding_box){0};
                }
            } break;
        
            case Gui_RenderCommand_Border:
            {
                gui_rect *Rect = PushDataInBatchList(Arena, BatchList, 1, UI_INSTANCE_PER_BATCH);
                if (Rect)
                {
                    Rect->Bounds        = BoundingBoxFromCommand(Command);
                    Rect->ColorTL       = RGBAToNormalized(Command->Border.Color);
                    Rect->ColorTR       = RGBAToNormalized(Command->Border.Color);
                    Rect->ColorBR       = RGBAToNormalized(Command->Border.Color);
                    Rect->ColorBL       = RGBAToNormalized(Command->Border.Color);
                    Rect->CornerRadius  = (bounding_box){0};
                    Rect->BorderWidth   = Command->Border.Width;
                    Rect->Softness      = 0;
                    Rect->SampleTexture = 0;
                    Rect->TextureSource = (bounding_box){0};
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

// TODO: Return Value...
// TODO: Default behavior handling?
// TODO: Text
// TODO: Images

//void
//GuiWindow(const char *Name, gui_component_free_list *FreeList)
//{
//    gui_component *TitleBar = GuiGetFreeComponent(FreeList);
//    {
//        GuiSetFixedH(50.0f, TitleBar);
//        GuiSetFixedW(280.0f, TitleBar);  // TODO: This should be 100% of parent
//        GuiSetColor(Gui_Color_Background, GuiGray850, TitleBar);
//    }
//
//    gui_component *ContentArea = GuiGetFreeComponent(FreeList);
//    {
//        GuiSetFixedH(534.0f, ContentArea);  // TODO: Should be remaining space
//        GuiSetFixedW(280.0f, ContentArea);  // TODO: This should be 100% of parent
//        GuiSetColor(Gui_Color_Background, GuiGray850, ContentArea);
//    }
//
//    gui_component *CloseButton = GuiGetFreeComponent(FreeList);
//    {
//        // TODO: This should depend on the window's name. We simply hardcode it for now.
//        CloseButton->Id         = GuiHashString("MyWindow_CloseButton");
//        CloseButton->EventMask |= Gui_Event_Clicked;
//
//        GuiSetFixedW(30.0f, CloseButton);
//        GuiSetFixedH(30.0f, CloseButton);
//        GuiSetColor(Gui_Color_Background, GuiCyan500, CloseButton);
//    }
//
//    // Title bar layout
//    {
//        gui_layout TitleBarLayout = GuiCreateLayout(Gui_Layout_HorizontalBox, TitleBar);
//        GuiLayoutAlignmentX(Gui_Alignment_End   , &TitleBarLayout);
//        GuiLayoutAlignmentY(Gui_Alignment_Center, &TitleBarLayout);
//        GuiLayoutPaddingRight(10.0f, &TitleBarLayout);
//
//        GuiAddComponent(CloseButton, &TitleBarLayout);
//    }
//
//    // Main Window Container
//    {
//        gui_component *Window = GuiGetFreeComponent(FreeList);
//        GuiSetFixedW(300.0f, Window);
//        GuiSetFixedH(600.0f, Window);
//        GuiSetColor(Gui_Color_Background, GuiGray900, Window);
//        GuiSetColor(Gui_Color_Border, GuiPurple300, Window);
//
//        gui_layout WindowLayout = GuiCreateLayout(Gui_Layout_VerticalBox, Window);
//        {
//            GuiLayoutPaddingLeft(10, &WindowLayout);
//            GuiLayoutPaddingRight(10, &WindowLayout);
//            GuiLayoutPaddingTop(8, &WindowLayout);
//            GuiLayoutPaddingBottom(8, &WindowLayout);
//        }
//        {
//            GuiAddComponent(TitleBar, &WindowLayout);
//            GuiAddComponent(ContentArea, &WindowLayout);
//        }
//
//        // TODO: These...
//        Window->BorderWidth = 2.0f;
//        Window->Id          = GuiHashString(Name);
//    }
//}


//{
//    GuiFeedInputToSubtree(&ComponentBuffer[3], InputQueue);

//    gui_event_stream Stream = {.Current = &ComponentBuffer[3], .CheckedMask = 0xFFFFFFFF};
//    gui_event        Event  = {0};

//    while (GuiGetNextEvent(&Event, &Stream))
//    {
//        uint64_t WindowCloseButton = GuiHashString("MyWindow_CloseButton");
//        if (Event.Source->Id == WindowCloseButton)
//        {
//            switch (Event.Type)
//            {

//            case Gui_Event_Clicked:
//            {
//            } break;

//            }
//        }

//        GuiHandleEventDefault(Event);
//    }

//    CommandCount = 0;

//    gui_tree_iterator Iterator  = GuiBeginTreeIterator(&ComponentBuffer[3]);
//    gui_component    *Component = 0;
//    while ((Component = GuiTreeIteratorGetNext(&Iterator)))
//    {
//        CommandBuffer[CommandCount++] = GuiGetRectangleCommand(Component);
//        CommandBuffer[CommandCount++] = GuiGetBorderCommand(Component);
//    }

//    GuiComputeLayout(&ComponentBuffer[3]);
//    DrawComputedLayoutTree(CommandBuffer, CommandCount, EngineMemory->FrameMemory, Renderer);
//}