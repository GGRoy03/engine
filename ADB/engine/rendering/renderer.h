#pragma once

#include "engine/math/matrix.h"
#include "engine/math/vector.h"
#include "resources.h"


// =====================================================
// Forward declarations
// =====================================================


typedef struct renderer      renderer;
typedef struct engine_memory engine_memory;


// =====================================================
// Camera (likely scene-level later)
// =====================================================

typedef struct camera
{
    vec3 Position;
    vec3 Forward;
    vec3 Up;

    float FovY;
    float AspectRatio;
    float NearPlane;
    float FarPlane;
} camera;

camera CreateCamera(vec3 Position, float FovY, float AspectRatio);

mat4x4 GetCameraWorldMatrix(camera *Camera);
mat4x4 GetCameraViewMatrix(camera *Camera);
mat4x4 GetCameraProjectionMatrix(camera *Camera);


// =====================================================
// Frame control
// =====================================================

typedef struct
{
    float R, G, B, A;
} clear_color;

void RendererEnterFrame  (clear_color Color, renderer *Renderer);
void RendererLeaveFrame  (int Width, int Height, engine_memory *EngineMemory, renderer *Renderer);
