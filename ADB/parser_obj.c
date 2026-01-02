#pragma once

#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include "utilities.h" // Arenas

typedef struct
{
    uint8_t *Data;
    size_t   Size;
    size_t   At;
} buffer;


static bool
IsBufferValid(buffer *Buffer)
{
    bool Result = (Buffer && Buffer->Data && Buffer->Size);
    return Result;
}


static bool
IsBufferInBounds(buffer *Buffer)
{
    assert(IsBufferValid(Buffer));

    bool Result = Buffer->At < Buffer->Size;
    return Result;
}

static uint8_t
GetNextToken(buffer *Buffer)
{
    assert(IsBufferValid(Buffer) && IsBufferInBounds(Buffer));

    uint8_t Result = Buffer->Data[Buffer->At++];
    return Result;
}


static uint8_t
PeekBuffer(buffer *Buffer)
{
    assert(IsBufferValid(Buffer) && IsBufferInBounds(Buffer));

    uint8_t Result = Buffer->Data[Buffer->At];
    return Result;
}


static bool
IsNewLine(uint8_t Token)
{
    bool Result = Token == '\r' || Token == '\n';
    return Result;
}


static buffer
ReadFileInBuffer(const char *Path, memory_arena *Arena)
{
    buffer Result = {0};

    FILE *File = fopen(Path, "rb");
    if (File)
    {
        fseek(File, 0, SEEK_END);
        long FileSize = ftell(File);
        fseek(File, 0, SEEK_SET);

        if (FileSize > 0)
        {
            uint8_t *Buffer = PushArray(Arena, uint8_t, FileSize + 1);
            if (Buffer)
            {
                size_t BytesRead = fread(Buffer, 1, (size_t)FileSize, File);

                Result.Data       = Buffer;
                Result.At         = 0;
                Result.Size       = BytesRead;
                Buffer[BytesRead] = '\0';
            }
        }

        fclose(File);
    }

    return Result;
}


static void
SkipWhitespaces(buffer *Buffer)
{
    assert(IsBufferValid(Buffer));

    while (IsBufferInBounds(Buffer) && (PeekBuffer(Buffer) == ' ' || PeekBuffer(Buffer) ==  '\t'))
    {
        ++Buffer->At;
    }
}


static float
ParseToSign(buffer *Buffer)
{
    float Result = 1.f;

    if (IsBufferInBounds(Buffer) && PeekBuffer(Buffer) ==  '-')
    {
        Result = -1.f;
        ++Buffer->At;
    }

    return Result;
}


static float
ParseToNumber(buffer *Buffer)
{
    assert(IsBufferValid(Buffer));

    float Result = 0.f;

    while (IsBufferInBounds(Buffer))
    {
        uint8_t Token    = GetNextToken(Buffer);
        uint8_t AsNumber = Token - (uint8_t)'0';

        if (AsNumber < 10)
        {
            Result = 10.f * Result + (float)AsNumber;
        }
        else
        {
            --Buffer->At;
            break;
        }
    }

    return Result;
}


static float
ParseToFloat(buffer *Buffer)
{
    assert(IsBufferValid(Buffer));

    float Result = 0.f;

    float Sign   = ParseToSign(Buffer);
    float Number = ParseToNumber(Buffer);

    if (IsBufferInBounds(Buffer) && PeekBuffer(Buffer) ==  '.')
    {
        ++Buffer->At;

        float C = 1.f / 10.f;
        while (IsBufferInBounds(Buffer))
        {
            uint8_t AsNumber = Buffer->Data[Buffer->At] - (uint8_t)'0';
            if (AsNumber < 10)
            {
                Number = Number + C * (float)AsNumber;
                C     *= 1.f / 10.f;

                ++Buffer->At;
            }
            else
            {
                break;
            }
        }
    }

    if (IsBufferInBounds(Buffer) && PeekBuffer(Buffer) == 'e' || PeekBuffer(Buffer) == 'E')
    {
        ++Buffer->At;

        if (IsBufferInBounds(Buffer) && PeekBuffer(Buffer) == '+')
        {
            ++Buffer->At;
        }

        float ExponentSign = ParseToSign(Buffer);
        float Exponent     = ExponentSign * ParseToFloat(Buffer);

        Number *= powf(10.f, Exponent);
    }

    Result = Sign * Number;

    return Result;
}

// OBJ Specific.

#define OBJ_MAX_VALUE_PER_VERTEX  4
#define OBJ_MAX_VALUE_PER_NORMAL  3
#define OBJ_MAX_VALUE_PER_TEXTURE 2

typedef struct
{
    union
    {
        struct
        {
            float X, Y, Z, W;
        };
        float AsBuffer[OBJ_MAX_VALUE_PER_VERTEX];
    };
} obj_position;


typedef struct
{
    union
    {
        struct
        {
            float X, Y, Z;
        };
        float AsBuffer[OBJ_MAX_VALUE_PER_NORMAL];
    };
} obj_normal;


typedef struct
{
    union
    {
        struct
        {
            float X, Y;
        };
        float AsBuffer[OBJ_MAX_VALUE_PER_TEXTURE];
    };
} obj_texture;


typedef struct
{
    obj_position  Vertex;
    obj_texture Texture;
    obj_normal  Normal;
} obj_vertex;


typedef struct
{
    int Vertex;
    int Texture;
    int Normal;
} obj_face_vertex;


typedef struct
{
    obj_vertex *VertexBuffer;
    uint64_t    VertexBufferSize;
} obj_submesh;


typedef struct
{
    struct obj_submesh_node *Next;
    obj_submesh              Value;
} obj_submesh_node;


typedef struct
{
    obj_submesh_node *First;
    obj_submesh_node *Last;
    uint32_t          Count;
} obj_submesh_list;


typedef struct
{
    obj_submesh_list Submeshes;
} obj_mesh;


typedef struct
{
    obj_mesh      *Mesh;
    memory_arena  *ParseArena;
    memory_arena  *OutputArena;
    uint64_t      SubmeshStart;
} obj_parser_state;


static void
ClearParserState(obj_parser_state *State)
{
    if (State)
    {
        if (!State->ParseArena)
        {
            memory_arena_params Params =
            {
                .AllocatedFromFile = __FILE__,
                .AllocatedFromLine = __LINE__,
                .CommitSize        = MiB(64),
                .ReserveSize       = GiB(1),
            };

            State->ParseArena = AllocateArena(Params);
        }

        if (!State->OutputArena)
        {
            memory_arena_params Params =
            {
                .AllocatedFromFile = __FILE__,
                .AllocatedFromLine = __LINE__,
                .CommitSize        = MiB(64),
                .ReserveSize       = GiB(1),
            };

            State->OutputArena = AllocateArena(Params);
        }

        ClearArena(State->ParseArena);
        ClearArena(State->OutputArena);
    }
}


void
ParseObjFromFile(const char *Path)
{
    obj_parser_state State = {0};
    ClearParserState(&State);

    // The overall allocation strategy is this:
    // [MeshList|SubMesh0|Submesh0.Data|Submesh1|Submesh1.Data|Submesh2|Submesh2.Data] -> Output Arena
    // We return the MeshList which contains the arena to prevent leaks.

    buffer Buffer = ReadFileInBuffer(Path, State.ParseArena);
    if (IsBufferValid(&Buffer))
    {
        // The overall allocation strategy is this:
        // [PositionBuffer0|NormalBuffer0|TextureBuffer0|PositionBuffer1|TextureBuffer1] -> Parse Arena
        // Re-Allocate into the arena as we need. Need some minor logic to find the current block to read from.
        // We'll keep it on the stack for now.

        obj_position PositionBuffer[512] = { 0 };
        uint32_t     PositionIndex       = 0;
        obj_normal   NormalBuffer[512]   = { 0 };
        uint32_t     NormalIndex         = 0;
        obj_texture  TextureBuffer[512]  = {0};
        uint32_t     TextureIndex        = 0;

        while (IsBufferValid(&Buffer) && IsBufferInBounds(&Buffer))
        {
            char Token = GetNextToken(&Buffer);

            switch (Token)
            {

            case 'v':
            case 'V':
            {
                SkipWhitespaces(&Buffer);

                uint32_t Limit       = 0;
                float   *FloatBuffer = 0;

                if (PeekBuffer(&Buffer) == 'n' || PeekBuffer(&Buffer) == (uint8_t)'N')
                {
                    Limit       = OBJ_MAX_VALUE_PER_NORMAL;
                    FloatBuffer = NormalBuffer[NormalIndex++].AsBuffer;

                    ++Buffer.At;
                } else
                if (PeekBuffer(&Buffer) == 't' || PeekBuffer(&Buffer) == (uint8_t)'T')
                {
                    Limit       = OBJ_MAX_VALUE_PER_TEXTURE;
                    FloatBuffer = TextureBuffer[TextureIndex++].AsBuffer;

                    ++Buffer.At;
                }
                else
                {
                    Limit       = OBJ_MAX_VALUE_PER_VERTEX;
                    FloatBuffer = PositionBuffer[PositionIndex++].AsBuffer;
                }

                if (Limit && FloatBuffer)
                {
                    for (uint32_t Idx = 0; Idx < Limit; ++Idx)
                    {
                        SkipWhitespaces(&Buffer);

                        if (IsBufferInBounds(&Buffer))
                        {
                            FloatBuffer[Idx] = ParseToFloat(&Buffer);
                        }
                        else
                        {
                            break;
                        }
                    }
                }
                else
                {
                    assert(!"Handle Error!");
                }
            } break;


            case 'f':
            case 'F':
            {
                SkipWhitespaces(&Buffer);

                while (!IsNewLine(PeekBuffer(&Buffer)))
                {
                    obj_face_vertex Face = { 0, 0, 0 };

                    Face.Vertex = (int)ParseToNumber(&Buffer);

                    if (PeekBuffer(&Buffer) == '/')
                    {
                        ++Buffer.At;

                        if (PeekBuffer(&Buffer) != '/')
                        {
                            Face.Texture = (int)ParseToNumber(&Buffer);
                        }

                        if (PeekBuffer(&Buffer) == '/')
                        {
                            ++Buffer.At;

                            Face.Normal = (int)ParseToNumber(&Buffer);
                        }
                    }

                    // TODO: Resolve Indices and write to the output arena.
                }

                // TODO: Maybe triangulate? I think we need a bit more processing to be correct.
            } break;

            case 'o':
            {
                SkipWhitespaces(&Buffer);

                // TODO: Figure out this object linked list stuff.

                while (!IsNewLine(PeekBuffer(&Buffer)))
                {
                    ++Buffer.At;
                }
            } break;

            case 'u':
            {
                // TODO: usemtl parsing, this creates a submesh.
            } break;

            case 'm':
            {
                // TODO: mtllib parsing.
            } break;

            case '#':
            case 's':
            case 'g':
            {
                while (IsBufferInBounds(&Buffer) && !IsNewLine(PeekBuffer(&Buffer)))
                {
                    ++Buffer.At;
                }
            } break;

            case '\r':
            case '\n':
            {
                ++Buffer.At;
            } break;

            default:
            {
                assert(!"Invalid Token");
            } break;

            }
        }

        assert(!"Handle the rest.");
    }
}
