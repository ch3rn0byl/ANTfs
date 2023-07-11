#pragma once
#include <Windows.h>
#include <stdexcept>

#include "ErrorHandler.h"
#include "privileges.h"
#include "antidefs.h"

class AntiForensics
{
public:
	AntiForensics(LPCWSTR lpFileName);
	AntiForensics();
	~AntiForensics();

	std::wstring what();

protected:
	template <typename T1, typename T2>
	T1* NextEntry(
		_In_reads_to_ptr_(Offset) T1* p,
		_In_ T2 Offset
	)
	{
		T1* Entry = reinterpret_cast<T1*>(
			reinterpret_cast<PUINT8>(p) + Offset
			);

		return Entry;
	}

	BOOL ReadNtfsFrom(
		_In_ LONGLONG Offset,
		_In_ ULONG Length, 
		_Inout_bytecount_(Length) PVOID Buffer
	);

	BOOL VerifySector(
		_In_reads_bytes_(m_BytesPerFileRecord) PFILE_RECORD& pFileRecord
	);

private:
	std::unique_ptr<ErrorHandler> m_ErrorHandler;
	std::unique_ptr<Privileges> m_Privileges;

	HANDLE m_hFileHandle;

	std::unique_ptr<BOOT_SECTOR> m_BootSector;

	UINT64 m_MftOffset;
	UINT32 m_BytesPerFileRecord;

	std::unique_ptr<FILE_RECORD> m_MftFileRecord;
};


// EOF