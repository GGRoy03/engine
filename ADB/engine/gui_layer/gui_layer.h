#pragma once


typedef struct gui_render_command gui_render_command;
typedef struct memory_arena       memory_arena;
typedef struct renderer           renderer;


void DrawComputedLayoutTree  (gui_render_command *RenderBuffer, uint32_t RenderBufferSize, memory_arena *Arena, renderer *Renderer);


// Components

typedef struct gui_string
{
	char    *Data;
	uint32_t Size;
} gui_string;

#define GuiName(Name) GuiString((char *)Name, sizeof(Name) - 1)
gui_string GuiString     (char *Data, uint32_t Size);