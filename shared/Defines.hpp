#pragma once

#pragma warning(disable: 4100 4201 4244 4324 4389 5054)

#include <Windows.h>
#include <stdint.h>
#include <stdio.h>
#include <cmath>

#include "SafeWrite/SafeWrite.hpp"

template <typename T_Ret = uint32_t, typename ...Args>
__forceinline T_Ret ThisCall(uint32_t _addr, const void* _this, Args ...args) {
	return ((T_Ret(__thiscall*)(const void*, Args...))_addr)(_this, std::forward<Args>(args)...);
}

template <typename T_Ret = void, typename ...Args>
__forceinline T_Ret StdCall(uint32_t _addr, Args ...args) {
	return ((T_Ret(__stdcall*)(Args...))_addr)(std::forward<Args>(args)...);
}

template <typename T_Ret = void, typename ...Args>
__forceinline T_Ret CdeclCall(uint32_t _addr, Args ...args) {
	return ((T_Ret(__cdecl*)(Args...))_addr)(std::forward<Args>(args)...);
}

template <typename T_Ret = void, typename ...Args>
__forceinline T_Ret FastCall(uint32_t _addr, Args ...args) {
	return ((T_Ret(__fastcall*)(Args...))_addr)(std::forward<Args>(args)...);
}

#pragma region Macros
#define EXTERN_DLL_EXPORT extern "C" __declspec(dllexport)

#ifdef NDEBUG
#define ASSUME_ASSERT(x) __assume(x)
#else
#define ASSUME_ASSERT(x) assert(x)
#endif

#define ASSERT_SIZE(a, b) static_assert(sizeof(a) == b, "Wrong structure size!");
#define ASSERT_OFFSET(a, b, c) static_assert(offsetof(a, b) == c, "Wrong member offset!");

#define SPEC_RESTRICT	__declspec(restrict)
#define SPEC_NOINLINE	__declspec(noinline)
#define SPEC_INLINE		__forceinline
#define SPEC_NORETURN	__declspec(noreturn)
#define SPEC_NOALIAS	__declspec(noalias)
#pragma endregion