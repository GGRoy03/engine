#include <stdbool.h>
#include <assert.h>


#include "third_party/gui/gui.h"


#include "engine.h"
#include "platform/platform.h"
#include "rendering/renderer.h"


typedef struct
{
	bool IsInitialized;

	gui_layout_tree    *GuiTree;
	gui_resource_table *GuiResourceTable;
} engine_state;


static gui_layout_properties WindowLayout =
{
	.Size      = {{1000.f, Gui_LayoutSizing_Fixed}, {1000.f, Gui_LayoutSizing_Fixed}},
	.MinSize   = {{1000.f, Gui_LayoutSizing_Fixed}, {1000.f, Gui_LayoutSizing_Fixed}},
	.MaxSize   = {{1000.f, Gui_LayoutSizing_Fixed}, {1000.f, Gui_LayoutSizing_Fixed}},
	.Direction = Gui_LayoutDirection_Horizontal,
	.XAlign    = Gui_Alignment_Start,
	.YAlign    = Gui_Alignment_Center,
	.Padding   = {20.f, 20.f, 20.f, 20.f},
	.Spacing   = 10.f,
};


static gui_paint_properties WindowStyle =
{
	.Color        = {0.0588f, 0.0588f, 0.0627f, 1.0f},
	.BorderColor  = { 0.1804f, 0.1961f, 0.2118f, 1.0f},
	.BorderWidth  = 2.f,
	.Softness     = 2.f,
	.CornerRadius = { 4.f, 4.f, 4.f, 4.f },
};


static gui_layout_properties PanelLayout =
{
	.Size    = {{50.f, Gui_LayoutSizing_Percent}, {100.f, Gui_LayoutSizing_Percent}},
	.MinSize = {{50.f, Gui_LayoutSizing_Percent}, {100.f, Gui_LayoutSizing_Percent}},
	.MaxSize = {{50.f, Gui_LayoutSizing_Percent}, {100.f, Gui_LayoutSizing_Percent}},
	.Padding = {20.f, 20.f, 20.f, 20.f},
};


static gui_paint_properties PanelStyle =
{
	.Color        = {0.1020f, 0.1098f, 0.1176f, 1.0f},
	.BorderColor  = {0.1804f, 0.1961f, 0.2118f, 1.0f},
	.BorderWidth  = 2.f,
	.Softness     = 2.f,
	.CornerRadius = {4.f, 4.f, 4.f, 4.f},
};


void
UpdateEngine(int WindowWidth, int WindowHeight, gui_pointer_event_list *PointerEventList, renderer *Renderer, engine_memory *EngineMemory)
{
	static engine_state Engine;

	if (!Engine.IsInitialized)
	{
		{
			gui_memory_footprint Footprint = GuiGetLayoutTreeFootprint(128);
			gui_memory_block     Block =
			{
				.SizeInBytes = Footprint.SizeInBytes,
				.Base        = PushArena(EngineMemory->StateMemory, Footprint.SizeInBytes, Footprint.Alignment),
			};

			Engine.GuiTree = GuiPlaceLayoutTreeInMemory(128, Block);
		}

		{
			gui_resource_table_params Params =
			{
				.HashSlotCount = 32,
				.EntryCount    = 128,
			};

			gui_memory_footprint Footprint = GuiGetResourceTableFootprint(Params);
			gui_memory_block     Block =
			{
				.SizeInBytes = Footprint.SizeInBytes,
				.Base        = PushArena(EngineMemory->StateMemory, Footprint.SizeInBytes, Footprint.Alignment),
			};

			Engine.GuiResourceTable = GuiPlaceResourceTableInMemory(Params, Block);
		}

		Engine.IsInitialized = true;
	}

	clear_color Color = (clear_color){.R = 0.f, .G = 0.f, .B = 0.f, .A = 1.f};
	RendererEnterFrame(Color, Renderer);

	GuiBeginFrame(PointerEventList, 0);

	uint32_t WindowFlags = Gui_NodeFlags_ClipContent | Gui_NodeFlags_IsDraggable | Gui_NodeFlags_IsResizable;
	uint32_t Window      = GuiCreateNode(0, WindowFlags, Engine.GuiTree); GuiUpdateLayout(Window, &WindowLayout, Engine.GuiTree); GuiUpdateStyle(Window, &WindowStyle, Engine.GuiTree);

	if (GuiEnterParent(Window, Engine.GuiTree, PushStruct(EngineMemory->FrameMemory, gui_parent_node)))
	{
		uint32_t Panel0 = GuiCreateNode(1, Gui_NodeFlags_None, Engine.GuiTree); GuiUpdateLayout(Panel0, &PanelLayout, Engine.GuiTree); GuiUpdateStyle(Panel0, &PanelStyle, Engine.GuiTree);
		uint32_t Panel1 = GuiCreateNode(2, Gui_NodeFlags_None, Engine.GuiTree); GuiUpdateLayout(Panel1, &PanelLayout, Engine.GuiTree); GuiUpdateStyle(Panel1, &PanelStyle, Engine.GuiTree);

		GuiLeaveParent(Window, Engine.GuiTree);
	}

	GuiComputeTreeLayout(Engine.GuiTree);

	DrawComputedLayoutTree(Engine.GuiTree, EngineMemory->FrameMemory, Renderer);

	GuiEndFrame();

	RendererLeaveFrame(WindowWidth, WindowHeight, EngineMemory, Renderer);
} 