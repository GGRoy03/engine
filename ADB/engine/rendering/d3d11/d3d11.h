#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

typedef struct d3d11_renderer d3d11_renderer;
d3d11_renderer * D3D11Initialize(HWND HWindow, int Width, int Height, memory_arena *Arena);