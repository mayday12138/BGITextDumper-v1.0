#include "WinFile.h"
#include <string>
#include "ScriptHeader.h"
#include "Instruction.h"

using std::string;
using std::wstring;

#define POST_NAME L"_BinOrder.txt"

BOOL GBKMode = TRUE;
HRESULT DisasmProc(wstring& FileName, PBYTE Buffer, ULONG Size);

int wmain(int argc, WCHAR* argv[])
{
	if (argc != 2 && argc != 3)
	{
		printf("exe file [mode]\n");
		getchar();
		return 0;
	}

	if (argc == 3)
	{
		GBKMode = FALSE;
	}

	if (wcsstr(argv[1], L".txt") || wcsstr(argv[1], L".exe") || wcsstr(argv[1], L".bat"))
	{
		//��ֹ������233
		return 0;
	}

	WinFile File;
	if (FAILED(File.Open(argv[1], WinFile::FileRead)))
	{
		return 0;
	}

	PBYTE Buffer = (PBYTE)HeapAlloc(GetProcessHeap(), 0, File.GetSize32());
	File.Read(Buffer, File.GetSize32());

	HRESULT Result = DisasmProc((wstring(argv[1]) + POST_NAME), Buffer, File.GetSize32());

	HeapFree(GetProcessHeap(), 0, Buffer);
	File.Release();
	return 0;
}

wstring FixStringW(wstring& Line)
{
	wstring Name;
	for (auto it : Line)
	{
		if (it == 0x000A)
		{
			Name += L"\\n";
		}
		else
		{
			Name += it;
		}
	}
	return Name;
}

HRESULT DisasmProc(wstring& FileName, PBYTE Buffer, ULONG Size)
{
	WinFile File;
	if (File.Open(FileName.c_str(), WinFile::FileWrite) != S_OK)
	{
		printf("Failed to open\n");
		getchar();
		return E_FAIL;
	}

	ULONG iPos = 0;
	ULONG StreamSize = Size;

	ULONG Finder = iPos;
	ULONG LastOffset = Size;
	while (Finder < Size)
	{
		if (*(PULONG)(Buffer + Finder) == 0x0000001B)
		{
			LastOffset = Finder + 4;
		}
		Finder += 4;
	}

	ULONG FinderAll = iPos;
	ULONG MinAddr = 0x7FFFFFFFUL;
	string OpString;
	while (iPos < Size)
	{
		ULONG Ins = *(PULONG)(Buffer + iPos);
		if (Ins & 0xFFF00000)
		{
			break;
		}
		GetInstructionInfo(Buffer, iPos, OpString, MinAddr);
	}


	iPos = 0;
	//StreamSize = max(LastOffset, MinAddr);
	StreamSize = MinAddr;
	iPos += StreamSize;

	PBYTE BlockPtr = (Buffer + iPos);
	while (iPos < Size)
	{
		string JPString;
		CHAR   UTF8String[2000] = {0};
		WCHAR  WideString[2000] = { 0 };
		CHAR PrefixLineNo[100] = { 0 };
		wsprintfA(PrefixLineNo, "[0x%08x]", iPos);

		JPString = (PCHAR)(Buffer + iPos);

		MultiByteToWideChar(GBKMode ? 936 : 932, 0, JPString.c_str(), JPString.length(), WideString, 1500);

		wstring WideFixedStr = FixStringW(wstring(WideString));
		WideCharToMultiByte(CP_UTF8, 0, WideFixedStr.c_str(), WideFixedStr.length(), UTF8String, 2000, nullptr, nullptr);

		File.Write((PBYTE)PrefixLineNo, lstrlenA(PrefixLineNo));
		File.Write((PBYTE)UTF8String, lstrlenA(UTF8String));
		File.Write((PBYTE)"\r\n", 2);
		File.Write((PBYTE)";", 1);
		File.Write((PBYTE)PrefixLineNo, lstrlenA(PrefixLineNo));

		File.Write((PBYTE)UTF8String, lstrlenA(UTF8String));

		File.Write((PBYTE)"\r\n\r\n", 4);

		ULONG Len = lstrlenA((PCHAR)BlockPtr) + 1;
		BlockPtr += Len;
		iPos += Len;
	}
	File.Release();
	return S_OK;
}
