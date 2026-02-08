#include <stdint.h>
#include <assert.h>
#include <math.h>
#include <string.h>

#include "utilities.h"
#include "renderer.h"





// ==============================================
// <Camera>
// ==============================================


camera
CreateCamera(vec3 Position, float FovY, float AspectRatio)
{
    camera Result =
    {
        .Position    = Position,
        .Forward     = Vec3(0.f, 0.f, 1.f),
        .Up          = Vec3(0.f, 1.f, 0.f),
        .AspectRatio = AspectRatio,
        .NearPlane   = 0.1f,
        .FarPlane    = 1000.f,
        .FovY        = FovY,
    };

    return Result;
}


mat4x4
GetCameraWorldMatrix(camera *Camera)
{
    (void)Camera;

    mat4x4 World = {0};
    World.c0r0 = 1.f;
    World.c1r1 = 1.f;
    World.c2r2 = 1.f;
    World.c3r3 = 1.f;

    return World;
}


mat4x4
GetCameraViewMatrix(camera *Camera)
{
    mat4x4 View = { 0 };

    vec3 Forward = Vec3Normalize(Camera->Forward);
    vec3 Right   = Vec3Normalize(Vec3Cross(Camera->Up, Forward));
    vec3 Up      = Vec3Cross(Forward, Right);

    Camera->Up = Up;

    View.c0r0 = Right.X;
    View.c0r1 = Right.Y;
    View.c0r2 = Right.Z;
    View.c0r3 = 0.f;

    View.c1r0 = Up.X;
    View.c1r1 = Up.Y;
    View.c1r2 = Up.Z;
    View.c1r3 = 0.f;

    View.c2r0 = Forward.X;
    View.c2r1 = Forward.Y;
    View.c2r2 = Forward.Z;
    View.c2r3 = 0.f;

    View.c3r0 = -Vec3Dot(Right, Camera->Position);
    View.c3r1 = -Vec3Dot(Up, Camera->Position);
    View.c3r2 = -Vec3Dot(Forward, Camera->Position);
    View.c3r3 = 1.f;

    return View;
}


mat4x4
GetCameraProjectionMatrix(camera *Camera)
{
    mat4x4 Projection = {0};

    float FovYRadians = Camera->FovY * (3.1416 / 180.0f);
    float F           = 1.f / tanf(FovYRadians * 0.5f);

    Projection.c0r0 = F / Camera->AspectRatio;
    Projection.c1r1 = F;
    Projection.c2r2 = (Camera->FarPlane + Camera->NearPlane) / (Camera->FarPlane - Camera->NearPlane);
    Projection.c2r3 = 1.f;
    Projection.c3r2 = (-2.f * Camera->FarPlane * Camera->NearPlane) / (Camera->FarPlane - Camera->NearPlane);

    return Projection;
}