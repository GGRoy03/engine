#ifdef _WIN32

#include <stdbool.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

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
	VirtualFree(At, 0, MEM_RELEASE);
}

#endif // _WIN32