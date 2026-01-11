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
	uint32_t Window      = GuiCreateNode(0, WindowFlags, Engine.GuiTree); GuiUpdateLayout(Window, 0, Engine.GuiTree);

	if (GuiEnterParent(Window, Engine.GuiTree, PushStruct(EngineMemory->FrameMemory, gui_parent_node)))
	{
		uint32_t Panel0 = GuiCreateNode(1, Gui_NodeFlags_None, Engine.GuiTree); GuiUpdateLayout(Panel0, 0, Engine.GuiTree);
		uint32_t Panel1 = GuiCreateNode(2, Gui_NodeFlags_None, Engine.GuiTree); GuiUpdateLayout(Panel1, 0, Engine.GuiTree);

		GuiLeaveParent(Window, Engine.GuiTree);
	}

	GuiComputeTreeLayout(Engine.GuiTree);

	{
		gui_memory_footprint Footprint = GuiGetRenderCommandsFootprint(Engine.GuiTree);
		gui_memory_block     Block =
		{
			.SizeInBytes = Footprint.SizeInBytes,
			.Base        = PushArena(EngineMemory->FrameMemory, Footprint.SizeInBytes, Footprint.Alignment)
		};

		gui_render_command_list CommandList = GuiComputeRenderCommands(Engine.GuiTree, Block);

		ui_group_params    GroupParams = {0};
		ui_batch_params    BatchParams = {0};
		render_batch_list *BatchList   = PushUIGroupParams(&GroupParams, EngineMemory->FrameMemory, &Renderer->PassList);

		for (uint32_t CommandIdx = 0; CommandIdx < CommandList.Count; ++CommandIdx)
		{
			gui_render_command *Command = &CommandList.Commands[CommandIdx];

			switch (Command->Type)
			{

			case Gui_RenderCommandType_Rectangle:
			{
				gui_rect *Rect = PushDataInBatchList(EngineMemory->FrameMemory, BatchList, UI_INSTANCE_PER_BATCH);
				if(Rect)
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
					Rect->TextureSource = (gui_bounding_box){0};
				}
			} break;

			case Gui_RenderCommandType_Border:
			{
				gui_rect *Rect = PushDataInBatchList(EngineMemory->FrameMemory, BatchList, UI_INSTANCE_PER_BATCH);
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
					Rect->TextureSource = (gui_bounding_box){0};
				}
			} break;

			default:
			{
				assert(!"INVALID ENGINE STATE");
			} break;

			}
		}
	}

	GuiEndFrame();

	RendererLeaveFrame(WindowWidth, WindowHeight, EngineMemory, Renderer);
} 