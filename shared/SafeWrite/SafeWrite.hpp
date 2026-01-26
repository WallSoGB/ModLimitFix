#pragma once

__declspec(noinline) void __fastcall SafeWrite8(size_t addr, size_t data);
__declspec(noinline) void __fastcall SafeWrite16(size_t addr, size_t data);
__declspec(noinline) void __fastcall SafeWrite32(size_t addr, size_t data);
__declspec(noinline) void __fastcall SafeWriteBuf(size_t addr, const void* data, size_t len);

// 5 bytes
__declspec(noinline) void __fastcall WriteRelJump(size_t jumpSrc, size_t jumpTgt);
__declspec(noinline) void __fastcall WriteRelCall(size_t jumpSrc, size_t jumpTgt);


// 6 bytes
__declspec(noinline) void __fastcall WriteRelJnz(size_t jumpSrc, size_t jumpTgt);
__declspec(noinline) void __fastcall WriteRelJle(size_t jumpSrc, size_t jumpTgt);

__declspec(noinline) void __fastcall PatchMemoryNop(ULONG_PTR Address, size_t Size);
void __fastcall PatchMemoryNopRange(ULONG_PTR StartAddress, ULONG_PTR EndAddress);

template <typename T>
void __fastcall WriteRelCall(size_t jumpSrc, T jumpTgt) {
	WriteRelCall(jumpSrc, (size_t)jumpTgt);
}

template <typename T>
void __fastcall WriteRelJump(size_t jumpSrc, T jumpTgt) {
	WriteRelJump(jumpSrc, (size_t)jumpTgt);
}

__declspec(noinline) void __fastcall ReplaceCall(size_t jumpSrc, size_t jumpTgt);

template <typename T>
void __fastcall ReplaceCall(size_t jumpSrc, T jumpTgt) {
	ReplaceCall(jumpSrc, (size_t)jumpTgt);
}

void __fastcall ReplaceVirtualFunc(size_t jumpSrc, void* jumpTgt);

// Stores the function-to-call before overwriting it, to allow calling the overwritten function after our hook is over.
// Thanks Demorome and lStewieAl

// Taken from lStewieAl.
// Returns the address of the jump/called function, assuming there is one.
static inline size_t GetRelJumpAddr(size_t jumpSrc) {
	return *(size_t*)(jumpSrc + 1) + jumpSrc + 5;
}

static inline size_t GetWriteAddr(size_t writeAddr) {
	return *(size_t*)(writeAddr);
}

// Specialization for member function pointers
template <typename C, typename Ret, typename... Args>
void __fastcall WriteRelJumpEx(size_t source, Ret(C::* const target)(Args...) const) {
    union
    {
        Ret(C::* tgt)(Args...) const;
        size_t funcPtr;
    } conversion;
    conversion.tgt = target;

    WriteRelJump(source, conversion.funcPtr);
}

template <typename C, typename Ret, typename... Args>
void __fastcall WriteRelJumpEx(size_t source, Ret(C::* const target)(Args...)) {
    union
    {
        Ret(C::* tgt)(Args...);
        size_t funcPtr;
    } conversion;
    conversion.tgt = target;

    WriteRelJump(source, conversion.funcPtr);
}

template <typename C, typename Ret, typename... Args>
void __fastcall WriteRelCallEx(size_t source, Ret(C::* const target)(Args...) const) {
	union
	{
		Ret(C::* tgt)(Args...) const;
		size_t funcPtr;
	} conversion;
	conversion.tgt = target;

	WriteRelCall(source, conversion.funcPtr);
}

template <typename C, typename Ret, typename... Args>
void __fastcall WriteRelCallEx(size_t source, Ret(C::* const target)(Args...)) {
	union
	{
		Ret(C::* tgt)(Args...);
		size_t funcPtr;
	} conversion;
	conversion.tgt = target;

	WriteRelCall(source, conversion.funcPtr);
}

template <typename C, typename Ret, typename... Args>
void __fastcall ReplaceCallEx(size_t source, Ret(C::* const target)(Args...) const) {
	union
	{
		Ret(C::* tgt)(Args...) const;
		size_t funcPtr;
	} conversion;
	conversion.tgt = target;

	ReplaceCall(source, conversion.funcPtr);
}

template <typename C, typename Ret, typename... Args>
void __fastcall ReplaceCallEx(size_t source, Ret(C::* const target)(Args...)) {
	union
	{
		Ret(C::* tgt)(Args...);
		size_t funcPtr;
	} conversion;
	conversion.tgt = target;

	ReplaceCall(source, conversion.funcPtr);
}

template <typename C, typename Ret, typename... Args>
void __fastcall ReplaceVirtualFuncEx(size_t source, Ret(C::* const target)(Args...) const) {
	union
	{
		Ret(C::* tgt)(Args...) const;
		size_t funcPtr;
	} conversion;
	conversion.tgt = target;

	SafeWrite32(source, conversion.funcPtr);
}

template <typename C, typename Ret, typename... Args>
void __fastcall ReplaceVirtualFuncEx(size_t source, Ret(C::* const target)(Args...)) {
	union
	{
		Ret(C::* tgt)(Args...);
		size_t funcPtr;
	} conversion;
	conversion.tgt = target;

	SafeWrite32(source, conversion.funcPtr);
}

template <typename C, typename Ret, typename... Args>
void __fastcall ReplaceVTableEntry(void** apVTable, uint32_t auiPosition, Ret(C::* const target)(Args...) const) {
	union {
		Ret(C::* tgt)(Args...) const;
		size_t funcPtr;
	} conversion;
	conversion.tgt = target;

	apVTable[auiPosition] = (void*)conversion.funcPtr;
}

template <typename C, typename Ret, typename... Args>
void __fastcall ReplaceVTableEntry(void** apVTable, uint32_t auiPosition, Ret(C::* const target)(Args...)) {
	union {
		Ret(C::* tgt)(Args...);
		size_t funcPtr;
	} conversion;
	conversion.tgt = target;

	apVTable[auiPosition] = (void*)conversion.funcPtr;
}

class CallDetour {
	size_t overwritten_addr = 0;
public:
	__declspec(noinline) void __fastcall WriteRelCall(size_t jumpSrc, void* jumpTgt)
	{
		__assume(jumpSrc != 0);
		__assume(jumpTgt != nullptr);
		if (*reinterpret_cast<uint8_t*>(jumpSrc) != 0xE8) {
			char cTextBuffer[72];
			sprintf_s(cTextBuffer, "Cannot write detour; jumpSrc is not a function call. (0x%08X)", jumpSrc);
			MessageBoxA(nullptr, cTextBuffer, "WriteRelCall", MB_OK | MB_ICONERROR);
			return;
		}
		overwritten_addr = GetRelJumpAddr(jumpSrc);
		::WriteRelCall(jumpSrc, jumpTgt);
	}

	template <typename T>
	__declspec(noinline) void __fastcall ReplaceCall(size_t jumpSrc, T jumpTgt) {
		__assume(jumpSrc != 0);
		if (*reinterpret_cast<uint8_t*>(jumpSrc) != 0xE8) {
			char cTextBuffer[72];
			sprintf_s(cTextBuffer, "Cannot write detour; jumpSrc is not a function call. (0x%08X)", jumpSrc);
			MessageBoxA(nullptr, cTextBuffer, "WriteRelCall", MB_OK | MB_ICONERROR);
			return;
		}
		overwritten_addr = GetRelJumpAddr(jumpSrc);
		::ReplaceCall(jumpSrc, (size_t)jumpTgt);
	}

	template <typename C, typename Ret, typename... Args>
	void __fastcall ReplaceCallEx(size_t source, Ret(C::* const target)(Args...) const) {
		union
		{
			Ret(C::* tgt)(Args...) const;
			size_t funcPtr;
		} conversion;
		conversion.tgt = target;

		ReplaceCall(source, conversion.funcPtr);
	}

	template <typename C, typename Ret, typename... Args>
	void __fastcall ReplaceCallEx(size_t source, Ret(C::* const target)(Args...)) {
		union
		{
			Ret(C::* tgt)(Args...);
			size_t funcPtr;
		} conversion;
		conversion.tgt = target;

		ReplaceCall(source, conversion.funcPtr);
	}

	template <typename T>
	void SafeWrite32(size_t jumpSrc, T jumpTgt) {
		__assume(jumpSrc != 0);
		overwritten_addr = GetWriteAddr(jumpSrc);
		::SafeWrite32(jumpSrc, (size_t)jumpTgt);
	}

	[[nodiscard]] size_t GetOverwrittenAddr() const { return overwritten_addr; }
};

class VirtFuncDetour {
protected:
	size_t overwritten_addr = 0;

public:
	template <typename C, typename Ret, typename... Args>
	void __fastcall ReplaceVirtualFuncEx(size_t source, Ret(C::* const target)(Args...)) {
		union
		{
			Ret(C::* tgt)(Args...);
			size_t funcPtr;
		} conversion;
		conversion.tgt = target;

		overwritten_addr = *(uint32_t*)source;

		SafeWrite32(source, conversion.funcPtr);
	}

	[[nodiscard]] size_t GetOverwrittenAddr() const { return overwritten_addr; }
};