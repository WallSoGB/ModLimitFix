#include "SafeWrite.hpp"
#include <memoryapi.h>

#pragma optimize("y", on)

class MemoryUnlock {
public:
	MemoryUnlock(size_t _addr, size_t _size = sizeof(size_t)) : addr(_addr), size(_size) {
		VirtualProtect((void*)addr, size, PAGE_EXECUTE_READWRITE, &oldProtect);
	}
	~MemoryUnlock() {
		VirtualProtect((void*)addr, size, oldProtect, &oldProtect);
	}

private:
	const size_t addr;
	const size_t size;
	DWORD oldProtect;
};

void __fastcall SafeWrite8(size_t addr, size_t data)
{
	MemoryUnlock unlock(addr);
	*reinterpret_cast<uint8_t*>(addr) = data;
}

void __fastcall SafeWrite16(size_t addr, size_t data)
{
	MemoryUnlock unlock(addr);
	*reinterpret_cast<uint16_t*>(addr) = data;
}

void __fastcall SafeWrite32(size_t addr, size_t data)
{
	MemoryUnlock unlock(addr);
	*reinterpret_cast<uint32_t*>(addr) = data;
}

void __fastcall SafeWriteBuf(size_t addr, const void *data, size_t len)
{
	MemoryUnlock unlock(addr, len);
	memcpy(reinterpret_cast<void*>(addr), data, len);
}

void __fastcall WriteRelJump(size_t jumpSrc, size_t jumpTgt)
{
	MemoryUnlock unlock(jumpSrc, 5);
	*reinterpret_cast<uint8_t*>(jumpSrc) = 0xE9;
	*reinterpret_cast<uint32_t*>(jumpSrc + 1) = jumpTgt - jumpSrc - 1 - 4;
}

void __fastcall WriteRelCall(size_t jumpSrc, size_t jumpTgt)
{
	MemoryUnlock unlock(jumpSrc, 5);
	*reinterpret_cast<uint8_t*>(jumpSrc) = 0xE8;
	*reinterpret_cast<uint32_t*>(jumpSrc + 1) = jumpTgt - jumpSrc - 1 - 4;
}

void __fastcall ReplaceCall(size_t jumpSrc, size_t jumpTgt)
{
	SafeWrite32(jumpSrc + 1, jumpTgt - jumpSrc - 1 - 4);
}

void __fastcall ReplaceVirtualFunc(size_t jumpSrc, void* jumpTgt) {
	SafeWrite32(jumpSrc, (size_t)jumpTgt);
}

void __fastcall WriteRelJnz(size_t jumpSrc, size_t jumpTgt)
{
	// jnz rel32
	SafeWrite16(jumpSrc, 0x850F);
	SafeWrite32(jumpSrc + 2, jumpTgt - jumpSrc - 2 - 4);
}

void __fastcall WriteRelJle(size_t jumpSrc, size_t jumpTgt)
{
	// jle rel32
	SafeWrite16(jumpSrc, 0x8E0F);
	SafeWrite32(jumpSrc + 2, jumpTgt - jumpSrc - 2 - 4);
}

void __fastcall PatchMemoryNop(ULONG_PTR Address, size_t Size)
{
	{
		MemoryUnlock unlock(Address, Size);
		for (size_t i = 0; i < Size; i++)
			*reinterpret_cast<volatile BYTE*>(Address + i) = 0x90; //0x90 == opcode for NOP
	}
	FlushInstructionCache(GetCurrentProcess(), (LPVOID)Address, Size);
}

void __fastcall PatchMemoryNopRange(ULONG_PTR StartAddress, ULONG_PTR EndAddress) {
	PatchMemoryNop(StartAddress, EndAddress - StartAddress);
}

#pragma optimize("y", off)