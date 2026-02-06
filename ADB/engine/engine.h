#pragma once

typedef struct renderer      renderer;
typedef struct engine_memory engine_memory;
typedef struct gui_context   gui_context;


void UpdateEngine  (int WindowWidth, int WindowHeight, gui_input_queue *InputQueue, renderer *Renderer, engine_memory *EngineMemory);