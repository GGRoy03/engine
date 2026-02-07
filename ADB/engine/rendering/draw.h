#pragma once


typedef struct render_batch_list render_batch_list;
typedef struct memory_arena      memory_arena;


void DrawGizmoCell  (vec3 Center, vec3 Color, render_batch_list *Batch, memory_arena *Arena);