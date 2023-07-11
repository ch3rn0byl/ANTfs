#include "antiforensics.h"

AntiForensics::AntiForensics(LPCWSTR lpFileName) :
	m_ErrorHandler(nullptr),
	m_Privileges(std::make_unique<Privileges>()),
	m_hFileHandle(INVALID_HANDLE_VALUE),
	m_BootSector(std::make_unique<BOOT_SECTOR>()),
	m_MftOffset(0),
	m_BytesPerFileRecord(1024),
	m_MftFileRecord(std::make_unique<FILE_RECORD>())
{
	if (!m_Privileges->IsRunningAsAdmin())
	{
		throw std::runtime_error("This program must be run with Administrator privileges.");
	}

	m_hFileHandle = CreateFile(
		lpFileName,
		GENERIC_READ | GENERIC_WRITE, 
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
	if (m_hFileHandle == INVALID_HANDLE_VALUE)
	{
		m_ErrorHandler = std::make_unique<ErrorHandler>(GetLastError());
		throw std::runtime_error(m_ErrorHandler->GetLastErrorAsStringA().c_str());
	}

	BOOL bStatus = ReadNtfsFrom(NULL, sizeof(BOOT_SECTOR), m_BootSector.get());
	if (!bStatus)
	{
		m_ErrorHandler = std::make_unique<ErrorHandler>(GetLastError());
		throw std::runtime_error(m_ErrorHandler->GetLastErrorAsStringA().c_str());
	}

	//
	// Validate the signature to ensure we are working with the NTFS filesystem.
	//
	if (m_BootSector->Signature != NTFS_BOOT_SECTOR_SIGNATURE)
	{
#ifdef _DEBUG
		std::printf("[DEBUG] Expected %04x, but got %04x.\n", NTFS_BOOT_SECTOR_SIGNATURE, m_BootSector->Signature);
#endif // _DEBUG
		throw std::runtime_error("Invalid NTFS signature.");
	}

	//
	// The MFT can be calculated by multiplying the starting cluster of the MFT by
	// BootSector.SectorsPerCluster and then shifting it left by 9. This is done for
	// the MFT Mirror as well.
	//
	m_MftOffset = m_BootSector->StartingClusterOfMft * m_BootSector->SectorsPerCluster;
	m_MftOffset *= m_BootSector->BytesPerSector;

	//
	// Read in the file record of the MFT. This will contain all the information needed
	// for parsing.
	//
	//bStatus = ReadNtfsFrom(m_MftOffset, m_BytesPerFileRecord, m_MftFileRecord.get());
	bStatus = ReadNtfsFrom(m_MftOffset, m_BytesPerFileRecord, m_MftFileRecord.get());
	if (!bStatus)
	{
		m_ErrorHandler = std::make_unique<ErrorHandler>(GetLastError());
		throw std::runtime_error(m_ErrorHandler->GetLastErrorAsStringA().c_str());
	}

	//
	// Validate the file record signature to ensure we have a file record.
	//
	if (m_MftFileRecord->Signature != FILE_SIGNATURE)
	{ 
#ifdef _DEBUG
		std::printf("[DEBUG] Expected %04x, but got %04x.\n", FILE_SIGNATURE, m_MftFileRecord->Signature);
#endif // _DEBUG
		throw std::runtime_error("Invalid file record signature.");
	}

	if (!VerifySector((_FILE_RECORD*)m_MftFileRecord.get()))
	{
		throw std::runtime_error("Invalid file record sector.");
	}
}

AntiForensics::AntiForensics() :
	m_ErrorHandler(nullptr),
	m_Privileges(nullptr),
	m_hFileHandle(INVALID_HANDLE_VALUE),
	m_BootSector(nullptr),
	m_MftOffset(0),
	m_BytesPerFileRecord(1024),
	m_MftFileRecord(nullptr)
{
	throw std::runtime_error("A drive letter must be specified.");
}

AntiForensics::~AntiForensics()
{
	if (m_hFileHandle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hFileHandle);
		m_hFileHandle = INVALID_HANDLE_VALUE;
	}
}

std::wstring AntiForensics::what()
{
	return m_ErrorHandler->GetLastErrorAsStringW();
}

_Use_decl_annotations_
BOOL 
AntiForensics::ReadNtfsFrom(LONGLONG Offset, ULONG Length, PVOID Buffer)
{
	OVERLAPPED Overlapped = { 0 };

	Overlapped.Offset = static_cast<DWORD>(Offset) & 0xffffffff;
	Overlapped.OffsetHigh = static_cast<DWORD>(Offset >> 32);

	BOOL bStatus = ReadFile(
		m_hFileHandle,
		Buffer,
		Length,
		NULL,
		&Overlapped
	);
	if (!bStatus)
	{
		m_ErrorHandler = std::make_unique<ErrorHandler>(GetLastError());
	}

	return bStatus;
}

BOOL AntiForensics::VerifySector(PFILE_RECORD& pFileRecord)
{
	// come back to this to comment it so i remember what it is lol
	PUINT16 pUsn = reinterpret_cast<PUINT16>(
		NextEntry(pFileRecord, m_BootSector->BytesPerSector - 2)
		);
	if (*pUsn != pFileRecord->Usn)
	{
#ifdef _DEBUG
		std::printf("[DEBUG] Expected %04x, but got %04x.\n", pFileRecord->Usn, *pUsn);
#endif // _DEBUG

		return FALSE;
	}

	RtlCopyMemory(
		reinterpret_cast<PUINT8>(pFileRecord) + m_BootSector->BytesPerSector - 2,
		pFileRecord->UpdateSequenceArray,
		pFileRecord->UsaCount - 1
	);

	return TRUE;
}


// EOF