#pragma once

#include <Windows.h>
#include <iostream>
#include <sstream>
#include <vector>

#include "typesndefs.h"

class antiforensics
{
private:
	HANDLE hFileHandle = INVALID_HANDLE_VALUE;

	PBOOT_SECTOR BootSector = nullptr;
	PFILE_RECORD MftFileRecord = nullptr;

	UINT32 BytesPerFileRecord = 1024;

	UINT64 MftOffset = 0;

	std::vector<PRUNLIST> MftRunList;

	wchar_t e[MAXERRORLENGTH * sizeof(wchar_t)] = { 0 };

protected:
	/// <summary>
	/// This method is a protected method. NextEntry will calculate 
	/// the offset and return the new address to parse. 
	/// </summary>
	/// <typeparam name="T1"></typeparam>
	/// <typeparam name="T2"></typeparam>
	/// <param name="p"></param>
	/// <param name="Offset"></param>
	/// <returns>Returns the next entry of a datatype.</returns>
	template <typename T1, typename T2>
	T1* 
	NextEntry(
		_In_reads_to_ptr_(Offset) T1* p,
		_In_ T2 Offset
	)
	{
		T1* Entry = reinterpret_cast<T1*>(
			reinterpret_cast<PUINT8>(p) + Offset
			);

		return Entry;
	}

	/// <summary>
	/// This method is a protected method. WriteToFile is a wrapper
	/// to WriteFile. This function will create a file for a deleted 
	/// file's contents to be written to. If there are any duplicates, 
	/// the filename will get renamed with a _2, _3, etc. 
	/// </summary>
	/// <param name="OutputFile"></param>
	/// <param name="lpBuffer"></param>
	/// <param name="dwNumberOfBytesToWrite"></param>
	/// <returns>Returns true if successful.</returns>
	_Success_(return != 0)
	bool 
	WriteToFile(
		_In_ std::wstring OutputFile,
		_In_ PUINT8 lpBuffer,
		_In_ UINT32 dwNumberOfBytesToWrite
	);

	/// <summary>
	/// This method is a protected method. LastError will convert the 
	/// GetLastError into a human-readable format to know exactly
	/// what happened.
	/// </summary>
	/// <returns>Returns a null-terminated error message.</returns>
	_Ret_maybenull_z_
	wchar_t* 
	LastError();

	/// <summary>
	/// This method is a protected method. ParseRunList will process the
	/// data runs of a file record. This will take care of unfragmented, 
	/// fragmented, sparsed, compressed, or any combination.
	/// </summary>
	/// <param name="RunListData"></param>
	/// <param name="TotalClusterSize"></param>
	/// <param name="RelativeOffset"></param>
	void 
	ParseRunList(
		_Outref_result_maybenull_ PUINT8& RunListData,
		_Inout_ PUINT32 TotalClusterSize,
		_Inout_ PUINT32 RelativeOffset
	);

	/// <summary>
	/// This method is a protected method. GetAttributeFor will iterate 
	/// through a given file record for an attribute. If the attribute 
	/// matches, space will be allocated for this attribute and copied. 
	/// This allocation will need to get zeroed and freed after use.
	/// </summary>
	/// <param name="AttributeType"></param>
	/// <param name="FileRecord"></param>
	/// <returns>Returns a PATTRIBUTE structure if successful; otherwise, 
	/// will return nullptr</returns>
	_Ret_maybenull_
	PATTRIBUTE 
	GetAttributeFor(
		_In_ ATTRIBUTE_TYPE AttributeType,
		_In_reads_bytes_(BytesPerFileRecord) PFILE_RECORD FileRecord
	);

	/// <summary>
	/// This method is a protected method. VerifySector will verify the
	/// USN matches at offset 510 and 1,024. If this doesn't match, chances
	/// are the data is corrupted. It will then apply the original bytes
	/// before being overwritten after successful verification.
	/// </summary>
	/// <param name="FileRecord"></param>
	/// <returns>Returns true if successful.</returns>
	_Success_(return != 0)
	bool 
	VerifySector(
		_In_reads_bytes_(BytesPerFileRecord) PFILE_RECORD& FileRecord
	);

	/// <summary>
	/// This method is a protected method. ReadNtfsFrom is a wrapper for
	/// ZwReadFile and is used for reading the MFT.
	/// </summary>
	/// <param name="Offset"></param>
	/// <param name="Length"></param>
	/// <param name="Buffer"></param>
	/// <returns>Returns true if successful.</returns>
	_Success_(return != 0)
	bool 
	ReadNtfsFrom(
		_In_ LONGLONG Offset,
		_In_ ULONG Length,
		_Inout_bytecount_(Length) PVOID Buffer
	);

public:
	antiforensics();
	~antiforensics();

	/// <summary>
	/// This method will intialize all the variables needed to 
	/// parse the MFT. It will calculate the offset to the MFT and 
	/// then use that to locate all of the MFTs, if there are 
	/// multiple. 
	/// </summary>
	/// <param name="DriveName"></param>
	/// <returns>Returns true if successful.</returns>
	_Success_(return != 0)
	bool 
	init(
		_In_ const wchar_t* DriveName
	);

	/// <summary>
	/// This method will iterate through all of the deleted files 
	/// in the MFT. It will locate the contents of the data and then 
	/// output the data into a directory for later analysis.
	/// </summary>
	/// <param name="OutputDirectory"></param>
	/// <param name="NumberOfRecords"></param>
	/// <returns>Returns true if successful.</returns>
	_Success_(return != 0)
	bool 
	RecoverFiles(
		_In_ std::wstring OutputDirectory,
		_Out_ PUINT32 NumberOfRecords
	);

	/// <summary>
	/// This method will iterate through the deleted files in
	/// the MFT looking for a specific file record. If it finds
	/// the file record, it will locate the contents of the data
	/// and sends an IOCTL to the driver to overwrite the record
	/// with zeroes. It will then overwrite the file record, itself.
	/// </summary>
	/// <param name="DriveLetter"></param>
	/// <param name="NameOfFileRecord"></param>
	/// <returns>Returns true if successful.</returns>
	_Success_(return != 0)
	bool 
	WipeRecord(
		_In_ const wchar_t* DriveLetter,
		_In_ const wchar_t* NameOfFileRecord
	);

	/// <summary>
	/// Returns what happened if something goes wrong. 
	/// </summary>
	/// <returns>Returns an error message.</returns>
	_Ret_z_
	wchar_t* 
	what();
};


/// EOF 