#pragma once

typedef struct renderer renderer;
typedef struct engine_memory engine_memory;
typedef struct gui_pointer_event_list gui_pointer_event_list;
void UpdateEngine(int WindowWidth, int WindowHeight, gui_pointer_event_list *PointerEventList, renderer *Renderer, engine_memory *EngineMemory);