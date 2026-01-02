#pragma once

#include <stdint.h>

// ==============================================
// <Utility Macros>
// ==============================================

#define KiB(n)  (((uint64_t)(n)) << 10)
#define MiB(n)  (((uint64_t)(n)) << 20)
#define GiB(n)  (((uint64_t)(n)) << 30)

#define AlignPow2(x,b) (((x) + (b) - 1)&(~((b) - 1)))

#define Minimum(A,B) (((A)<(B))?(A):(B))
#define Maximum(A,B) (((A)>(B))?(A):(B))

// ==============================================
// <Memory Arenas>
// ==============================================

typedef struct memory_arena
{
    struct memory_arena *Prev;
    struct memory_arena *Current;

    uint64_t             CommitSize;
    uint64_t             ReserveSize;
    uint64_t             Committed;
    uint64_t             Reserved;

    uint64_t             BasePosition;
    uint64_t             Position;

    const char          *AllocatedFromFile;
    uint32_t             AllocatedFromLine;
} memory_arena;

typedef struct
{
    uint64_t    ReserveSize;
    uint64_t    CommitSize;
    const char *AllocatedFromFile;
    uint32_t    AllocatedFromLine;
} memory_arena_params;

typedef struct
{
    memory_arena *Arena;
    uint64_t      Marker;
} memory_region;

memory_arena * AllocateArena      (memory_arena_params Params);
void           ReleaseArena       (memory_arena *Arena);

void         * PushArena          (memory_arena *Arena, uint64_t Size, uint64_t Alignment);
void           PopArenaTo         (memory_arena *Arena, uint64_t Position);
void           ClearArena         (memory_arena *Arena);

memory_region  EnterMemoryRegion  (memory_arena *Arena);
void           LeaveMemoryRegion  (memory_region Region);

#define PushArrayNoZeroAligned(Arena, Type, Count, Align)  ((Type *)PushArena((Arena), sizeof(Type) * (Count), (Align)))
#define PushArrayAligned(Arena, Type, Count, Align)        PushArrayNoZeroAligned((Arena), Type, (Count), (Align))
#define PushArray(Arena, Type, Count)                      PushArrayAligned((Arena), Type, (Count), ((sizeof(Type) < 8) ? 8 : _Alignof(Type)))
#define PushStruct(Arena, Type)                            PushArray((Arena), Type, 1)