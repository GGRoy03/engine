#pragma once

typedef struct gui_layout_tree gui_layout_tree;
typedef struct memory_arena    memory_arena;
typedef struct renderer        renderer;

void DrawComputedLayoutTree  (gui_layout_tree *Tree, memory_arena *Arena, renderer *Renderer);