#include <stdint.h>
#include <stdbool.h>

#include "utilities.h"         // Implementation Header

#include "platform/platform.h" // Allocation

memory_arena *
AllocateArena(memory_arena_params Params)
{
    uint64_t ReserveSize = AlignPow2(Params.ReserveSize, KiB(4));
    uint64_t CommitSize  = AlignPow2(Params.CommitSize , KiB(4));

    if (CommitSize > ReserveSize)
    {
        CommitSize = ReserveSize;
    }

    void *HeapBase     = OSReserve(ReserveSize);
    bool  CommitResult = OSCommit(HeapBase, CommitSize);
    if (!HeapBase || !CommitResult)
    {
        return 0;
    }

    memory_arena *Arena = (memory_arena *)HeapBase;
    Arena->Prev              = 0;
    Arena->Current           = Arena;
    Arena->CommitSize        = Params.CommitSize;
    Arena->ReserveSize       = Params.ReserveSize;
    Arena->Committed         = CommitSize;
    Arena->Reserved          = ReserveSize;
    Arena->BasePosition      = 0;
    Arena->Position          = sizeof(memory_arena);
    Arena->AllocatedFromFile = Params.AllocatedFromFile;
    Arena->AllocatedFromLine = Params.AllocatedFromLine;

    return Arena;
}

void
ReleaseArena(memory_arena *Arena)
{
    memory_arena *Prev = 0;
    for (memory_arena *Node = Arena->Current; Node != 0; Node = Prev)
    {
        Prev = Node->Prev;
        OSRelease(Node, Node->Reserved);
    }
}

void *
PushArena(memory_arena *Arena, uint64_t Size, uint64_t Alignment)
{
    memory_arena *Active       = Arena->Current;
    uint64_t      PrePosition  = AlignPow2(Active->Position, Alignment);
    uint64_t      PostPosition = PrePosition + Size;

    if (Active->Reserved < PostPosition)
    {
        uint64_t ReserveSize = Active->ReserveSize;
        uint64_t CommitSize  = Active->CommitSize;

        if (Size + sizeof(memory_arena) > ReserveSize)
        {
            ReserveSize = AlignPow2(Size + sizeof(memory_arena), Alignment);
            CommitSize  = AlignPow2(Size + sizeof(memory_arena), Alignment);
        }

        memory_arena_params Params;
        Params.CommitSize        = CommitSize;
        Params.ReserveSize       = ReserveSize;
        Params.AllocatedFromFile = Active->AllocatedFromFile;
        Params.AllocatedFromLine = Active->AllocatedFromLine;

        memory_arena *NewArena = AllocateArena(Params);
        NewArena->BasePosition = Active->BasePosition + Active->ReserveSize;
        NewArena->Prev         = Active;

        Arena->Current = NewArena;
        Active = NewArena;

        PrePosition  = AlignPow2(Active->Position, Alignment);
        PostPosition = PrePosition + Size;
    }

    if (Active->Committed < PostPosition)
    {
        uint64_t CommitPostAligned = PostPosition + Active->CommitSize - 1;
        CommitPostAligned         -= CommitPostAligned % Active->CommitSize;

        uint64_t CommitPostClamped = Minimum(CommitPostAligned, Active->Reserved);
        uint64_t CommitSize        = CommitPostClamped - Active->Committed;
        uint8_t *CommitPointer     = (uint8_t *)Active + Active->Committed;

        bool CommitResult = OSCommit(CommitPointer, CommitSize);
        if (!CommitResult)
        {
            return 0;
        }

        Active->Committed = CommitPostClamped;
    }

    void *Result = 0;
    if (Active->Committed >= PostPosition)
    {
        Result = (uint8_t *)Active + PrePosition;
        Active->Position = PostPosition;
    }

    return Result;
}

uint64_t
GetArenaPosition(memory_arena *Arena)
{
    memory_arena *Active = Arena->Current;
    return Active->BasePosition + Active->Position;
}

void
PopArenaTo(memory_arena *Arena, uint64_t Position)
{
    memory_arena *Active    = Arena->Current;
    uint64_t      PoppedPos = Maximum(Position, sizeof(memory_arena));

    for (memory_arena *Prev = 0; Active->BasePosition >= PoppedPos; Active = Prev)
    {
        Prev = Active->Prev;
        OSRelease(Active, Active->Reserved);
    }

    Arena->Current           = Active;
    Arena->Current->Position = PoppedPos - Arena->Current->BasePosition;
}

void
PopArena(memory_arena *Arena, uint64_t Amount)
{
    uint64_t OldPosition = GetArenaPosition(Arena);
    uint64_t NewPosition = OldPosition;

    if (Amount < OldPosition)
    {
        NewPosition = OldPosition - Amount;
    }

    PopArenaTo(Arena, NewPosition);
}

void
ClearArena(memory_arena *Arena)
{
    PopArenaTo(Arena, 0);
}

memory_region
EnterMemoryRegion(memory_arena *Arena)
{
    memory_region Result =
    {
        .Arena  = Arena,
        .Marker = GetArenaPosition(Arena),
    };

    return Result;
}

void
LeaveMemoryRegion(memory_region Region)
{
    PopArenaTo(Region.Arena, Region.Marker);
}

#define PushArrayNoZeroAligned(Arena, Type, Count, Align) \
    ((Type *)PushArena((Arena), sizeof(Type) * (Count), (Align)))

#define PushArrayAligned(Arena, Type, Count, Align) \
    PushArrayNoZeroAligned((Arena), Type, (Count), (Align))

#define PushArray(Arena, Type, Count) \
    PushArrayAligned((Arena), Type, (Count), ((sizeof(Type) < 8) ? 8 : _Alignof(Type)))

#define PushStruct(Arena, Type) \
    PushArray((Arena), Type, 1)