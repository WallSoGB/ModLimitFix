constexpr char const* PLUGIN_NAME = "MLF";
constexpr uint32_t PLUGIN_VERSION = 5;

namespace {
	struct NVSEInterface {
		uint32_t	uiNvseVersion;
		uint32_t	uiRuntimeVersion;
		uint32_t	uiEditorVersion;
		BOOL		bIsEditor;
		// .. rest is not needed
	};

	struct PluginInfo {
		enum {
			kInfoVersion = 1
		};

		uint32_t	uiInfoVersion;
		const char* pName;
		uint32_t	uiVersion;
	};
}


namespace Win32IO {

	class NiBinaryStream {
	public:
		NiBinaryStream();
		virtual ~NiBinaryStream();

		uint32_t	m_uiAbsoluteCurrentPos;
		void*		m_pfnRead;
		void*		m_pfnWrite;
	};

	class NiFile : public NiBinaryStream {
	public:
		enum OpenMode {
			READ_ONLY	= 0,
			WRITE_ONLY	= 1,
			APPEND_ONLY = 2,
		};

		uint32_t	m_uiBufferAllocSize;
		uint32_t	m_uiBufferReadSize;
		uint32_t	m_uiPos;
		uint32_t	m_uiCurrentFilePos;
		char*		m_pBuffer;
		void*		m_pFile;
		OpenMode	m_eMode;
		bool		m_bGood;
	};

	class BSFile : public NiFile {
	public:
		bool		bUseAuxBuffer;
		void*		pAuxBuffer;
		int32_t		iAuxTrueFilePos;
		uint32_t	uiAuxBufferMinIndex;
		uint32_t	uiAuxBufferMaxIndex;
		char		cFileName[260];
		int32_t		iResult;
		uint32_t	uiIOSize;
		uint32_t	uiTrueFilePos;
		uint32_t	uiFileSize;
	};

	ASSERT_SIZE(BSFile, 0x158);

	class BSWin32File : public BSFile {
	private:
		SPEC_INLINE HANDLE __fastcall GetFileHandle() {
			return reinterpret_cast<HANDLE>(m_pFile);
		}

		SPEC_INLINE void __fastcall SetFileHandle(HANDLE pSysFile) {
			m_pFile = reinterpret_cast<void*>(pSysFile);
		}

		void __fastcall PickModes(NiFile::OpenMode aeNiOpenMode, uint32_t& arOpenMode, uint32_t& arAccessMode) {
			arOpenMode   = OPEN_EXISTING;
			arAccessMode = GENERIC_READ;
			switch (aeNiOpenMode) {
				case NiFile::READ_ONLY:
					arAccessMode = GENERIC_READ;
					arOpenMode	 = OPEN_EXISTING;
					break;
				case NiFile::WRITE_ONLY:
					arAccessMode = GENERIC_WRITE;
					arOpenMode	 = OPEN_ALWAYS;
					break;
				case NiFile::APPEND_ONLY:
					arAccessMode = GENERIC_WRITE | GENERIC_READ;
					arOpenMode	 = OPEN_EXISTING;
					break;
				default:
					__assume(0);
			}
		}

		bool __fastcall InitFile(const char* apFileName) {
			uint32_t eFlags = FILE_FLAG_SEQUENTIAL_SCAN | FILE_ATTRIBUTE_NORMAL;
			uint32_t eOpenMode;
			uint32_t eAccessMode;
			PickModes(m_eMode, eOpenMode, eAccessMode);

			HANDLE hFile = CreateFileA(apFileName, eAccessMode, FILE_SHARE_READ, nullptr, eOpenMode, eFlags, nullptr);

			if (hFile == INVALID_HANDLE_VALUE) {
				SetFileHandle(nullptr);
				return false;
			}
			else {
				SetFileHandle(hFile);
				if (m_eMode == NiFile::APPEND_ONLY) {
					uint64_t ullPos = 0;
					SetFilePointerEx(GetFileHandle(), *PLARGE_INTEGER(&ullPos), nullptr, FILE_END);
				}
				return true;
			}
		}

	public:
		void __fastcall Hook_BSOpen() {
			InitFile(cFileName);
		}

		bool __fastcall Hook_NiOpen(const char* apFileName) {
			return InitFile(apFileName);
		}

		void Hook_BSGetSize() {
			uint64_t ulliSize = 0;
			GetFileSizeEx(GetFileHandle(), PLARGE_INTEGER(&ulliSize));
			uiFileSize = ulliSize;
		}

		void Hook_SeekAlt(int32_t aiOffset, int32_t aiWhence) {
			uint64_t ullOffset = aiOffset;
			uint64_t ullNewPos = 0;
			m_bGood = SetFilePointerEx(GetFileHandle(), *PLARGE_INTEGER(&ullOffset), PLARGE_INTEGER(&ullNewPos), aiWhence);
			if (m_bGood) {
				m_uiCurrentFilePos = ullNewPos;
				m_uiAbsoluteCurrentPos = m_uiCurrentFilePos;
			}
		}

		uint32_t Hook_GetFileSize() {
			uint64_t ullSize = 0;
			if (GetFileSizeEx(GetFileHandle(), PLARGE_INTEGER(&ullSize)))
				return ullSize;

			return 0;
		}

		static uint32_t Hook_Close(HANDLE apFile) {
			CloseHandle(apFile);
			return 0;
		}

		static uint32_t Hook_DiskRead(void* apBuffer, uint32_t auiElemSize, uint32_t auiBytes, HANDLE apFile) {
			DWORD uiRead = 0;
			if (ReadFile(apFile, apBuffer, auiBytes, &uiRead, nullptr))
				return uiRead;
			return 0;
		}

		static uint32_t Hook_DiskWrite(const void* apBuffer, uint32_t auiElemSize, uint32_t auiBytes, HANDLE apFile) {
			DWORD uiWritten = 0;
			if (WriteFile(apFile, apBuffer, auiBytes, &uiWritten, nullptr))
				return uiWritten;
			return 0;
		}
	};

	namespace HooksAsm {
		namespace BSFile {
			void __declspec(naked) Open() {
				static constexpr uint32_t uiRetAddr = 0xAFF46F;
				__asm {
					mov     ecx, [ebp - 0xC]
					call	Win32IO::BSWin32File::Hook_BSOpen
					jmp 	uiRetAddr
				}
			}

			void __declspec(naked) GetSize() {
				static constexpr uint32_t uiRetAddr = 0xB0057D;
				__asm {
					mov     ecx, [ebp - 0x8]
					call	Win32IO::BSWin32File::Hook_BSGetSize
					jmp 	uiRetAddr
				}
			}
		}

		namespace NiFile {
			void __declspec(naked) Open() {
				static constexpr uint32_t uiRetAddr = 0xAA1531;
				__asm {
					mov		ecx, esi
					mov     edx, [esp + 0x14]
					call	Win32IO::BSWin32File::Hook_NiOpen
					mov     ecx, [esp + 0x18]
					mov		ebx, 0
					jmp 	uiRetAddr
				}
			}

			void __declspec(naked) SeekAlt() {
				static constexpr uint32_t uiRetAddr = 0xAA1743;
				__asm {
					// EBX - aiWhence
					// EDI - aiOffset
					push	ebx
					push	edi
					mov		ecx, esi
					call	Win32IO::BSWin32File::Hook_SeekAlt
					pop		edi
					pop		ebx
					jmp 	uiRetAddr
				}
			}
		}
	}

	void InitHooks() {
		{
			WriteRelJump(0xAA14F3, HooksAsm::NiFile::Open);
			ReplaceCall(0xAA16AE, BSWin32File::Hook_Close);
			ReplaceCall(0xAA18DE, BSWin32File::Hook_Close);
			WriteRelJump(0xAA1715, HooksAsm::NiFile::SeekAlt);
			WriteRelJumpEx(0xAA15F0, &BSWin32File::Hook_GetFileSize);
		}

		{
			WriteRelJump(0xAFF348, HooksAsm::BSFile::Open);
			WriteRelJump(0xB0052B, HooksAsm::BSFile::GetSize);
			ReplaceCall(0xAFFD3C, BSWin32File::Hook_Close);
		}

		{
			constexpr static uint32_t uiReadAddr[]	= { 0xAA1583, 0xAA17AF, 0xAA17CF };
			constexpr static uint32_t uiWriteAddr[] = { 0xAA15CD, 0xAA1881 };
			for (uint32_t uiAddr : uiReadAddr) {
				ReplaceCall(uiAddr, BSWin32File::Hook_DiskRead);
			}

			for (uint32_t uiAddr : uiWriteAddr) {
				ReplaceCall(uiAddr, BSWin32File::Hook_DiskWrite);
			}
		}

		{
			// Disable Obsidian's serialized I/O thread, as it just wastes memory after our patches
			PatchMemoryNop(0xAA306A, 5);
			WriteRelJump(0xAA85C0, 0xECB65A);
			WriteRelJump(0xAA8610, 0xECB3A8);
			WriteRelJump(0xAA8660, 0xECB086);
		}
	}
};

EXTERN_DLL_EXPORT bool NVSEPlugin_Query(const NVSEInterface* apNVSE, PluginInfo* apInfo) {
	apInfo->uiInfoVersion	= PluginInfo::kInfoVersion;
	apInfo->pName			= PLUGIN_NAME;
	apInfo->uiVersion		= PLUGIN_VERSION;

	return !apNVSE->bIsEditor;
}

EXTERN_DLL_EXPORT bool NVSEPlugin_Preload() {
	Win32IO::InitHooks();
	return true;
}

EXTERN_DLL_EXPORT bool NVSEPlugin_Load(const NVSEInterface* apNVSE) {
	// Non-NVSE extenders don't have Preload, and would have to do call InitHooks here instead.
	return true;
}

BOOL WINAPI DllMain(
	HANDLE  hDllHandle,
	DWORD   dwReason,
	LPVOID  lpreserved
)
{
	return TRUE;
}