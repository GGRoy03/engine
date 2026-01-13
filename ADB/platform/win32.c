#ifdef _WIN32

#include <stdbool.h>
#include <assert.h>


#include "third_party/gui/gui.h"


#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Windowsx.h>


#include "utilities.h"
#include "platform.h"
#include "engine/engine.h"
#include "engine/rendering/renderer.h"
#include "engine/rendering/d3d11/d3d11.h"

// ==============================================
// <Memory> : PUBLIC
// ==============================================

void *OSReserve(size_t Size)
{
	void *Result = VirtualAlloc(0, Size, MEM_RESERVE, PAGE_READWRITE);
	return Result;
}

bool OSCommit(void *At, size_t Size)
{
	void *Committed = VirtualAlloc(At, Size, MEM_COMMIT, PAGE_READWRITE);
	bool  Result    = Committed != 0;
	return Result;
}

void OSRelease(void *At, size_t Size)
{
    Unused(Size);
	VirtualFree(At, 0, MEM_RELEASE);
}

// ==============================================
// <Utilities>   : INTERNAL
// ==============================================


static void
Win32Sleep(DWORD Milliseconds)
{
    Sleep(Milliseconds);
}


static void
Win32GetClientSize(HWND Window, int *OutWidth, int *OutHeight)
{
    RECT Client;
    if (GetClientRect(Window, &Client))
    {
        *OutWidth  = Client.right  - Client.left;
        *OutHeight = Client.bottom - Client.top;
    }
}


// ==============================================
// <Threading> : INTERNAL
// =============================================


typedef struct
{
    platform_work_queue_callback *Callback;
    void                         *Data;
} platform_work_queue_entry;


typedef struct platform_work_queue
{
    uint32_t volatile CompletionGoal;
    uint32_t volatile CompletionCount;

    uint32_t volatile NextEntryToWrite;
    uint32_t volatile NextEntryToRead;
    HANDLE            SemaphoreHandle;

    platform_work_queue_entry Entries[128];
} platform_work_queue;


typedef struct
{
    DWORD                ID;
    platform_work_queue *Queue;
} win32_thread_info;


static void
Win32AddEntry(platform_work_queue *Queue, platform_work_queue_callback *Callback, void *Data)
{
    assert((Queue->NextEntryToWrite + 1) % ArrayCount(Queue->Entries) != Queue->NextEntryToRead);

    platform_work_queue_entry *Entry = Queue->Entries + Queue->NextEntryToWrite;
    Entry->Callback = Callback;
    Entry->Data     = Data;

    // Use Array count macro 
    _WriteBarrier();
    _mm_sfence();
    Queue->NextEntryToWrite = (Queue->NextEntryToWrite + 1) % ArrayCount(Queue->Entries);
    Queue->CompletionGoal  += 1;

    ReleaseSemaphore(Queue->SemaphoreHandle, 1, 0);
}


static bool
Win32DoNextWorkQueueEntry(platform_work_queue *Queue)
{
    bool ShouldSleep = false;

    uint32_t OriginalNextEntryToRead = Queue->NextEntryToRead;
    uint32_t NewNextEntryToRead      = (OriginalNextEntryToRead + 1) % ArrayCount(Queue->Entries); // Use array count macro
    if (OriginalNextEntryToRead != Queue->NextEntryToWrite)
    {
        uint32_t Index = InterlockedCompareExchange((LONG volatile *)&Queue->NextEntryToRead, NewNextEntryToRead, OriginalNextEntryToRead);
        if (Index == OriginalNextEntryToRead)
        {
            platform_work_queue_entry Entry = Queue->Entries[Index];
            Entry.Callback(Queue, Entry.Data);
            InterlockedIncrement((LONG volatile *)&Queue->CompletionCount);
        }
    }
    else
    {
        ShouldSleep = true;
    }

    return ShouldSleep;
}


static void
Win32CompleteAllWork(platform_work_queue *Queue)
{
    while (Queue->CompletionGoal != Queue->CompletionCount)
    {
        Win32DoNextWorkQueueEntry(Queue);
    }

    Queue->CompletionGoal  = 0;
    Queue->CompletionCount = 0;
}


static DWORD WINAPI
ThreadProc(LPVOID lpParameter)
{
    win32_thread_info *ThreadInfo = (win32_thread_info *)lpParameter;

    for (;;)
    {
        if (Win32DoNextWorkQueueEntry(ThreadInfo->Queue))
        {
            WaitForSingleObjectEx(ThreadInfo->Queue->SemaphoreHandle, INFINITE, FALSE);
        }
    }
}


// ==============================================
// <Entry Point> : INTERNAL
// ==============================================


typedef struct
{
    gui_pointer_event_list *PointerEventList;
    int                     MouseX;
    int                     MouseY;
    memory_arena           *WriteArena;
} win32_state;


static LRESULT CALLBACK
Win32MessageHandler(HWND Hwnd, UINT Message, WPARAM WParam, LPARAM LParam)
{
    win32_state *Win32 = GetWindowLongPtrA(Hwnd, GWLP_USERDATA);

    switch (Message)
    {

    case WM_MOUSEMOVE:
    {
        gui_point Mouse     = (gui_point){(float)GET_X_LPARAM(LParam), (float)GET_Y_LPARAM(LParam)};
        gui_point LastMouse = (gui_point){(float)Win32->MouseX, (float)Win32->MouseY};

        GuiPushPointerMoveEvent(Mouse, LastMouse, PushStruct(Win32->WriteArena, gui_pointer_event_node), Win32->PointerEventList);

        Win32->MouseX = (int)Mouse.X;
        Win32->MouseY = (int)Mouse.Y;
    } break;

    case WM_LBUTTONDOWN:
    {
        gui_point Mouse = (gui_point){(float)GET_X_LPARAM(LParam), (float)GET_Y_LPARAM(LParam)};

        GuiPushPointerClickEvent(Gui_PointerButton_Primary, Mouse, PushStruct(Win32->WriteArena, gui_pointer_event_node), Win32->PointerEventList);
    } break;

    case WM_LBUTTONUP:
    {
        gui_point Mouse = (gui_point){(float)GET_X_LPARAM(LParam), (float)GET_Y_LPARAM(LParam)};

        GuiPushPointerReleaseEvent(Gui_PointerButton_Primary, Mouse, PushStruct(Win32->WriteArena, gui_pointer_event_node), Win32->PointerEventList);
    } break;

    case WM_DESTROY:
    {
        PostQuitMessage(0);
        return 0;
    } break;

    default: break;

    }

    return DefWindowProcA(Hwnd, Message, WParam, LParam);
}


static HWND
Win32CreateWindow(int Width, int Height, HINSTANCE HInstance, int CmdShow)
{
    HWND Result = {0};

    WNDCLASSA WindowClass =
    {
        .hCursor       = LoadCursor(NULL, IDC_ARROW),
        .lpfnWndProc   = Win32MessageHandler,
        .hInstance     = HInstance,
        .lpszClassName = "Toy Engine",
    };

    if (RegisterClassA(&WindowClass))
    {
        Result = CreateWindowExA(0, WindowClass.lpszClassName, "Engine Window",
                                 WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                                 Width, Height, 0, 0, HInstance, 0);
        if (Result != 0)
        {
            ShowWindow(Result, CmdShow);
        }
    }

    return Result;
}


int WINAPI
WinMain(HINSTANCE HInstance, HINSTANCE PrevInstance, LPSTR CmdLine, int CmdShow)
{
    Unused(CmdLine);
    Unused(PrevInstance);

    HWND WindowHandle = Win32CreateWindow(1920, 1080, HInstance, CmdShow);
    BOOL Running      = true;

    // Threading Stuff
    platform_work_queue WorkQueue = {0};
    {
        SYSTEM_INFO SystemInfo;
        GetSystemInfo(&SystemInfo);

        DWORD               ProcessorCount  = SystemInfo.dwNumberOfProcessors;
        win32_thread_info   ThreadInfos[32] = {0};
        HANDLE              SemaphoreHandle = CreateSemaphoreEx(0, 0, ProcessorCount, 0, 0, SEMAPHORE_ALL_ACCESS);

        assert(ProcessorCount < 32);

        for (DWORD LogicalIndex = 0; LogicalIndex < ProcessorCount; ++LogicalIndex)
        {
            win32_thread_info *ThreadInfo = ThreadInfos + LogicalIndex;
            ThreadInfo->ID                     = LogicalIndex;
            ThreadInfo->Queue                  = &WorkQueue;
            ThreadInfo->Queue->SemaphoreHandle = SemaphoreHandle;

            DWORD ThreadID;
            HANDLE ThreadHandle = CreateThread(0, 0, ThreadProc, ThreadInfo, 0, &ThreadID);
            if (ThreadHandle)
            {
                CloseHandle(ThreadHandle);
            }
        }
    }

    engine_memory EngineMemory = { 0 };
    {
        {
            memory_arena_params Params =
            {
                .AllocatedFromFile = __FILE__,
                .AllocatedFromLine = __LINE__,
                .ReserveSize       = MiB(128),
                .CommitSize        = MiB(16),
            };

            EngineMemory.StateMemory = AllocateArena(Params);
        }

        {
            memory_arena_params Params =
            {
                .AllocatedFromFile = __FILE__,
                .AllocatedFromLine = __LINE__,
                .ReserveSize       = GiB(2),
                .CommitSize        = MiB(32),
            };

            EngineMemory.FrameMemory = AllocateArena(Params);
        }

        EngineMemory.AddEntry     = Win32AddEntry;
        EngineMemory.CompleteWork = Win32CompleteAllWork;
        EngineMemory.WorkQueue    = &WorkQueue;
    }

    win32_state *Win32 = PushStruct(EngineMemory.StateMemory, win32_state);
    if (Win32)
    {
        Win32->MouseX           = 0;
        Win32->MouseY           = 0;
        Win32->PointerEventList = PushStruct(EngineMemory.StateMemory, gui_pointer_event_list);
        Win32->WriteArena       = EngineMemory.FrameMemory;

        SetWindowLongPtrA(WindowHandle, GWLP_USERDATA, (LONG_PTR)Win32);
    }

    int ClientWidth, ClientHeight;
    Win32GetClientSize(WindowHandle, &ClientWidth, &ClientHeight);

    renderer *Renderer = PushStruct(EngineMemory.StateMemory, renderer);
    Renderer->Backend        = D3D11Initialize(WindowHandle, ClientWidth, ClientHeight, EngineMemory.StateMemory);
    Renderer->Resources      = CreateResourceManager(EngineMemory.StateMemory);
    Renderer->ReferenceTable = CreateResourceReferenceTable(EngineMemory.StateMemory);

    while (Running)
    {
        MSG Message;
        while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
        {
            if (Message.message == WM_QUIT)
            {
                Running = FALSE;
            }

            TranslateMessage(&Message);
            DispatchMessageA(&Message);
        }

        Win32GetClientSize(WindowHandle, &ClientWidth, &ClientHeight);

        UpdateEngine(ClientWidth, ClientHeight, Win32->PointerEventList, Renderer, &EngineMemory);

        // Frame Cleanup
        {
            PopArenaTo(EngineMemory.FrameMemory, 0);
        }

        Win32Sleep(8);
    }

    return 0;
}

#endif // _WIN32