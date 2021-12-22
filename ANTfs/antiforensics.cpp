#include "antiforensics.h"

_Use_decl_annotations_
bool 
antiforensics::WriteToFile(
    std::wstring OutputFile,
    PUINT8 lpBuffer, 
    UINT32 dwNumberOfBytesToWrite
)
{
    std::wstringstream ss;

    bool bStatus = false;

    int duplicate = 1;

    HANDLE hFile = INVALID_HANDLE_VALUE;

    ss << OutputFile.c_str();

    do
    {
        hFile = CreateFile(
            ss.str().c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            CREATE_NEW,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
        if (GetLastError() == ERROR_FILE_EXISTS)
        {
            ss.str(std::wstring());

            ss << OutputFile.c_str();
            ss << L"_";
            ss << duplicate++;
        }
    } while (GetLastError() != ERROR_SUCCESS);

    bStatus = WriteFile(
        hFile,
        lpBuffer,
        dwNumberOfBytesToWrite,
        NULL,
        NULL
    );
    if (!bStatus)
    {
        return bStatus;
    }

    CloseHandle(hFile);
    return bStatus;
}

_Use_decl_annotations_
wchar_t* 
antiforensics::LastError()
{
    wchar_t* error[MAXERRORLENGTH] = { 0 };

    FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&error),
        MAXERRORLENGTH,
        NULL
    );

    return *error;
}

_Use_decl_annotations_
void 
antiforensics::ParseRunList(
    PUINT8& RunListData,
    PUINT32 TotalClusterSize,
    PUINT32 RelativeOffset
)
{
    UINT32 NumberOfClusters = 0;
    UINT32 TempLcn = 0;

    PRUNLIST_HEADER RunListHeader = reinterpret_cast<PRUNLIST_HEADER>(
        RunListData
        );

    ///
    /// Skip over the header. Doesn't need to get parsed.
    /// 
    RunListData++;

    ///
    /// Parse the length of the attribute. 
    /// 
    switch (RunListHeader->ByteLength)
    {
    case 1:
        NumberOfClusters = *reinterpret_cast<PINT8>(RunListData);
        break;
    case 2:
        NumberOfClusters = *reinterpret_cast<PINT16>(RunListData);
        break;
    case 3:
    case 4:
        NumberOfClusters = *reinterpret_cast<PINT32>(RunListData);
        if (RunListHeader->ByteLength == 3)
        {
            NumberOfClusters &= 0xffffff;
        }
        break;
    default:
        ///
        /// We shouldn't be in here but just in case there's anything 
        /// over 4, let's just break to see what value it is and 
        /// implement it. 
        /// 
        std::printf("[!] Got %d and is not implemented. Breaking at %d...\n",
            RunListHeader->ByteLength, __LINE__);
        DebugBreak();
        break;
    }

    *TotalClusterSize = NumberOfClusters;

    ///
    /// Move on to the next entry to parse the number of clusters
    /// this data is on.
    /// 
    RunListData += RunListHeader->ByteLength;

    if (RunListHeader->ByteOffset)
    {
        switch (RunListHeader->ByteOffset)
        {
        case 1:
            TempLcn = *reinterpret_cast<PINT8>(RunListData);
            break;
        case 2:
            TempLcn = *reinterpret_cast<PINT16>(RunListData);
            break;
        case 3:
            TempLcn = *reinterpret_cast<PINT32>(RunListData);
            TempLcn &= 0xffffff;

            ///
            /// Check to see if this value is negative. If it is a negative number,
            /// pad the most significant byte with an 0xff.
            /// 
            if ((TempLcn & 0x800000) >> 17)
            {
                TempLcn |= 0xff000000;
            }
            break;
        case 4:
            TempLcn = *reinterpret_cast<PINT32>(RunListData);
            break;
        default:
            ///
            /// Same as above. We shouldn't be in here but just in case there's
            /// anything over 4, let's just break to see what value it is and then
            /// implement it. 
            ///
            std::printf("[!] Got %d and is not implemented. Breaking at %d...\n",
                RunListHeader->ByteLength, __LINE__);
            DebugBreak();
            break;
        }
    }
    else
    {
        TempLcn = 0;
    }

    *RelativeOffset = TempLcn;
    RunListData += RunListHeader->ByteOffset;
}

_Use_decl_annotations_
PATTRIBUTE 
antiforensics::GetAttributeFor(
    ATTRIBUTE_TYPE AttributeType,
    PFILE_RECORD FileRecord
)
{
    PATTRIBUTE Attribute = nullptr;

    PATTRIBUTE temp = reinterpret_cast<PATTRIBUTE>(
        NextEntry(FileRecord, FileRecord->AttributeOffset)
        );

    if (temp->AttributeType == AttributeEnd)
    {
        return nullptr;
    }

    do
    {
        if (temp->AttributeType == AttributeType)
        {
            switch (temp->AttributeType)
            {
            case AttributeFileName:
                ///
                /// If the attribute's ID is 2, you will get the long filename. If the
                /// attribute's ID is 3, you will get the short filename. I want the 
                /// long filename.
                /// 
                if (temp->AttributeIdentifier == NamespaceDos)
                {
                    Attribute = reinterpret_cast<PATTRIBUTE>(
                        new UINT8[temp->Length]()
                        );
                    RtlCopyMemory(Attribute, temp, temp->Length);
                    return Attribute;
                }
                break;
            case AttributeData:
                /// 
                /// If the attribute is resident and the attribute is not nameless, this means 
                /// that there is an alternative data stream present (at least in my testing).
                /// I am only interested in data, not named attributes.
                /// 
                if (temp->IsNonResident)
                {
                    Attribute = reinterpret_cast<PATTRIBUTE>(
                        new UINT8[temp->Length]()
                        );
                    RtlCopyMemory(Attribute, temp, temp->Length);
                    return Attribute;
                }
                else if (!temp->IsNonResident && temp->NameLength == NULL)
                {
                    Attribute = reinterpret_cast<PATTRIBUTE>(
                        new UINT8[temp->Length]()
                        );
                    RtlCopyMemory(Attribute, temp, temp->Length);

                    return Attribute;
                }
                break;
            default:
                break;
            }
        }

        temp = NextEntry(temp, temp->Length);
    } while (temp->AttributeType != AttributeEnd);

    return Attribute;
}

_Use_decl_annotations_
bool 
antiforensics::VerifySector(
    PFILE_RECORD& FileRecord
)
{
    PUINT16 Usn = reinterpret_cast<PUINT16>(
        NextEntry(FileRecord, BootSector->BytesPerSector - 2)
        );
    if (*Usn != FileRecord->Usn)
    {
        wsprintf(e, L"[%ws::%d] Expected %04x, but got %04x.\n", 
            __FUNCTIONW__, __LINE__, FileRecord->Usn, *Usn);

        return false;
    }

    RtlCopyMemory(
        reinterpret_cast<PUINT8>(FileRecord) + BootSector->BytesPerSector - 2,
        FileRecord->UpdateSequenceArray,
        FileRecord->UsaCount - 1
    );

    return true;
}

_Use_decl_annotations_
bool 
antiforensics::ReadNtfsFrom(
    LONGLONG Offset,
    ULONG Length,
    PVOID Buffer
)
{
    OVERLAPPED ol = { 0 };

    ol.Offset = Offset & 0xffffffff;
    ol.OffsetHigh = Offset >> 32;

    bool bStatus = ReadFile(
        hFileHandle,
        Buffer,
        Length,
        NULL,
        &ol
    );
    if (!bStatus)
    {
        wsprintf(e, L"[%ws::%d] %ws\n", __FUNCTIONW__, __LINE__, LastError());
    }
    return bStatus;
}

antiforensics::antiforensics()
{
}

antiforensics::~antiforensics()
{
    if (!MftRunList.empty())
    {
        for (PRUNLIST temp : MftRunList)
        {
            RtlSecureZeroMemory(temp, sizeof(RUNLIST));
            delete temp; temp = nullptr;
        }
    }

    if (hFileHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFileHandle);
        hFileHandle = NULL;
    }

    if (MftFileRecord != nullptr)
    {
        RtlSecureZeroMemory(MftFileRecord, BytesPerFileRecord);
        delete[] MftFileRecord; MftFileRecord = nullptr;
    }

    if (BootSector != nullptr)
    {
        RtlSecureZeroMemory(BootSector, sizeof(BOOT_SECTOR));
        delete BootSector; BootSector = nullptr;
    }
}

_Use_decl_annotations_
bool 
antiforensics::init(
    const wchar_t* DriveName
)
{
    UINT32 StartingLcn = 0;

    hFileHandle = CreateFile(
        DriveName,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (hFileHandle == INVALID_HANDLE_VALUE)
    {
        wsprintf(e, L"[%ws::%d] %ws", __FUNCTIONW__, __LINE__, LastError());
        return false;
    }

    BootSector = new BOOT_SECTOR();

    bool bStatus = ReadNtfsFrom(NULL, sizeof(BOOT_SECTOR), BootSector);
    if (!bStatus)
    {
        wsprintf(e, L"[%ws::%d] %ws", __FUNCTIONW__, __LINE__, LastError());
        return false;
    }

    if (BootSector->Signature != NTFS_BOOT_SECTOR_SIGNATURE)
    {
        wsprintf(e, L"[%ws::%d] Boot signature mismatch! Expected %04x, but got %04x.\n", 
            __FUNCTIONW__, __LINE__, NTFS_BOOT_SECTOR_SIGNATURE, BootSector->Signature);
        return false;
    }

    ///
    /// The MFT can be calculated by multiplying the starting cluster of the MFT by
    /// BootSector->SectorsPerCluster and then shifting it left by 9. This is done
    /// for the MFT Mirror as well. 
    /// 
    MftOffset = BootSector->StartingClusterOfMft * BootSector->SectorsPerCluster;
    MftOffset *= BootSector->BytesPerSector;

    MftFileRecord = reinterpret_cast<PFILE_RECORD>(
        new UINT8[BytesPerFileRecord]()
        );

    bStatus = ReadNtfsFrom(MftOffset, BytesPerFileRecord, MftFileRecord);
    if (!bStatus)
    {
        wsprintf(e, L"[%ws::%d] %ws", __FUNCTIONW__, __LINE__, LastError());
        return false;
    }

    if (MftFileRecord->Signature != FILE_SIGNATURE)
    {
        wsprintf(e, L"[%ws::%d] $MFT signature mismatch! Expected %04x, but got %04x.\n",
            __FUNCTIONW__, __LINE__, NTFS_BOOT_SECTOR_SIGNATURE, BootSector->Signature);
        return false;
    }

    if (!VerifySector(MftFileRecord))
    {
        return false;
    }

    ///
    /// There can be multiple MFT's in the drive. We need to pull all the 
    /// data run entries to enumerate all of the files on the system.
    /// 
    PATTRIBUTE Attribute = GetAttributeFor(AttributeData, MftFileRecord);
    if (Attribute == NULL)
    {
        wsprintf(e, L"[%ws::%d] Unable to find attribute for %02x.\n", 
            __FUNCTIONW__, __LINE__, AttributeData);
        return false;
    }

    ///
    /// Iterate through the data runs and save them in a vector to be used 
    /// later.
    /// 
    PUINT8 MftRunListData = reinterpret_cast<PUINT8>(
        NextEntry(Attribute, Attribute->NonResident.RunArrayOffset)
        );

    do
    {
        PRUNLIST RunListData = new RUNLIST();

        ParseRunList(MftRunListData, &RunListData->ClusterSize, &RunListData->LcnOffset);

        ///
        /// Calculate the size (in bytes) of the clusters.
        /// 
        RunListData->ClusterSize *= BootSector->SectorsPerCluster;
        RunListData->ClusterSize *= BootSector->BytesPerSector;

        StartingLcn += RunListData->LcnOffset;

        RunListData->AbsoluteOffset = StartingLcn * BootSector->SectorsPerCluster;
        RunListData->AbsoluteOffset *= BootSector->BytesPerSector;

        MftRunList.push_back(RunListData);
    } while (*MftRunListData != NULL);

    if (MftRunList.empty())
    {
        ///
        /// We shouldn't be in here, but just in case.
        /// 
        wsprintf(e, L"[%ws::%d] There were no entries allocated.\n", 
            __FUNCTIONW__, __LINE__);
        return false;
    }
    
    return true;
}

_Use_decl_annotations_
bool 
antiforensics::RecoverFiles(
    std::wstring OutputDirectory,
    PUINT32 NumberOfRecords
)
{
    std::wstringstream ss;

    UINT32 RecordCount = 0;

    bool bStatus = false;

    PATTRIBUTE FilenameAttribHeader = nullptr;
    PATTRIBUTE DataAttributeHeader = nullptr;
    PFILENAME_ATTRIB FileNameAttribute = nullptr;

    PFILE_RECORD FileRecord = reinterpret_cast<PFILE_RECORD>(
        new UINT8[BytesPerFileRecord]()
        );

    wchar_t* FileName = new wchar_t[MAX_PATH * 2]();

    for (PRUNLIST DataRun : MftRunList)
    {
        UINT64 Offset = DataRun->AbsoluteOffset;

        ///
        /// Adjust the size to take into account the file records. We don't want to count down
        /// from 2048 if there's only two file records.
        /// 
        UINT32 NumberOfRecords = DataRun->ClusterSize / BytesPerFileRecord;
        do
        {
            bStatus = ReadNtfsFrom(Offset, BytesPerFileRecord, FileRecord);
            if (bStatus)
            {
                ///
                /// Ran into a weird issue where the logic for signature was picking up junk, 
                /// like FILE_UPLOAD_LIMITE_PREMIUM_TIER_1. The offset to the update sequence isn't
                /// a good check because it could probably change; instead, I AND it to check if it contains
                /// an underscore. It's a little hacky, but it works.
                /// 
                if (FileRecord->Signature == FILE_SIGNATURE && (FileRecord->UsaOffset & 0xff) != '_')
                {
                    ///
                    /// Check to make sure the USN matches the last two bytes of the record. 
                    /// 
                    if (!VerifySector(FileRecord))
                    {
                        std::wcerr << LastError() << std::endl;
                        std::wcerr << "[!] Warning: Usn does not match for record: " << std::hex << Offset << std::endl;
                    }

                    ///
                    /// Check the records allocation status. If it's unallocatd, I want it.
                    /// 
                    if (FileRecord->Flags.value == NULL)
                    {
                        FilenameAttribHeader = GetAttributeFor(AttributeFileName, FileRecord);
                        if (FilenameAttribHeader)
                        {
                            FileNameAttribute = reinterpret_cast<PFILENAME_ATTRIB>(
                                NextEntry(FilenameAttribHeader, FilenameAttribHeader->Resident.ValueOffset)
                                );

                            RtlCopyMemory(FileName, FileNameAttribute->Filename, FileNameAttribute->FilenameLength * 2);

                            ///
                            /// Hit some cases where we would receive multibtye characters and would throw off
                            /// the program and offset the structures. This sanity checks by getting rid of the multibyte
                            /// and replacing it with an underscore.
                            /// 
                            for (int i = 0; i < FileNameAttribute->FilenameLength; i++)
                            {
                                if (FileName[i] & 0xff00)
                                {
                                    FileName[i] = 0x005f;
                                }
                            }

                            ss << OutputDirectory.c_str();
                            ss << FileName;

                            std::wcout << "[+] Writing to " << ss.str().c_str() << "." << std::endl;

                            DataAttributeHeader = GetAttributeFor(AttributeData, FileRecord);
                            if (DataAttributeHeader)
                            {
                                if (!DataAttributeHeader->IsNonResident)
                                {
                                    if (DataAttributeHeader->Resident.ValueLength != NULL)
                                    {
                                        PUINT8 DataLocation = reinterpret_cast<PUINT8>(
                                            NextEntry(DataAttributeHeader, DataAttributeHeader->Resident.ValueOffset)
                                            );

                                        if (!WriteToFile(ss.str(), DataLocation, DataAttributeHeader->Resident.ValueLength))
                                        {
                                            std::wcerr << "Dammit: " << LastError() << std::endl;
                                        }

                                        RecordCount++;
                                    }
                                }
                                else
                                {
                                    if (DataAttributeHeader->NonResident.AllocatedSize != NULL)
                                    {
                                        PUINT8 RunListData = reinterpret_cast<PUINT8>(
                                            NextEntry(DataAttributeHeader, DataAttributeHeader->NonResident.RunArrayOffset)
                                            );

                                        UINT64 StartingLcn = 0;
                                        UINT64 DataOffset = 0;

                                        PUINT8 AllocatedData = new UINT8[DataAttributeHeader->NonResident.AllocatedSize]();

                                        do
                                        {
                                            UINT64 AbsoluteOffset = 0;
                                            UINT32 ClusterSize = 0;
                                            UINT32 TempLcn = 0;

                                            ParseRunList(RunListData, &ClusterSize, &TempLcn);

                                            ///
                                            /// If TempLcn is zero, that means we hit a sparse file. Move on.
                                            /// 
                                            if (TempLcn == NULL)
                                            {
                                                continue;
                                            }

                                            ///
                                            /// Calculate the size (in bytes) of the clusters.
                                            /// 
                                            ClusterSize *= BootSector->SectorsPerCluster;
                                            ClusterSize *= BootSector->BytesPerSector;

                                            StartingLcn += TempLcn;

                                            AbsoluteOffset = StartingLcn * BootSector->SectorsPerCluster;
                                            AbsoluteOffset *= BootSector->BytesPerSector;

                                            ///
                                            /// Throughout my entire testing on tons and tons of files on various 
                                            /// drives, it seems the absolute address goes up to only five bytes
                                            /// so mask off the extra byte since UINT64 is eight.
                                            /// 
                                            AbsoluteOffset &= 0xffffffffff;

                                            if (!ReadNtfsFrom(AbsoluteOffset, ClusterSize, AllocatedData + DataOffset))
                                            {
                                                std::wcerr << "Failed: " << GetLastError() << std::endl;
                                            }

                                            DataOffset += ClusterSize;

                                        } while (*RunListData != NULL);

#ifdef _DEBUG
                                        if (DataAttributeHeader->NonResident.AllocatedSize > 0xffffffff)
                                        {
                                            std::wcerr << "[!] We have a size bigger than a UINT32. File record: ";
                                            std::wcerr << std::hex << Offset << std::endl;
                                        }
#endif // DEBUG

                                        ///
                                        /// Now output the data into a file and then clear up and release
                                        /// the allocated memory.
                                        ///
                                        if (!WriteToFile(ss.str(), AllocatedData, DataAttributeHeader->NonResident.AllocatedSize))
                                        {
                                            std::wcerr << "Dammit: " << LastError() << std::endl;
                                        }

                                        RecordCount++;

                                        RtlSecureZeroMemory(AllocatedData, DataAttributeHeader->NonResident.AllocatedSize);
                                        delete[] AllocatedData;
                                        AllocatedData = nullptr;
                                    }
                                }
                            }

                            ///
                            /// Clear the filename to get reused again.
                            /// 
                            ss.str(std::wstring());

                            if (DataAttributeHeader)
                            {
                                RtlSecureZeroMemory(DataAttributeHeader, DataAttributeHeader->Length);
                                delete[] DataAttributeHeader;
                                DataAttributeHeader = nullptr;
                            }
                        }

                        if (FilenameAttribHeader)
                        {
                            RtlSecureZeroMemory(FilenameAttribHeader, FilenameAttribHeader->Length);
                            delete[] FilenameAttribHeader;
                            FilenameAttribHeader = nullptr;
                        }
                    }
                }
            }

            RtlSecureZeroMemory(FileName, MAX_PATH * 2);

            Offset += BytesPerFileRecord;
        } while (NumberOfRecords--);
    }

    *NumberOfRecords = RecordCount;

    if (FileName != nullptr)
    {
        RtlSecureZeroMemory(FileName, MAX_PATH * 2);
        delete[] FileName;
        FileName = nullptr;
    }

    RtlSecureZeroMemory(FileRecord, BytesPerFileRecord);
    delete[] FileRecord;
    FileRecord = nullptr;

    return bStatus;
}

_Use_decl_annotations_
bool 
antiforensics::WipeRecord(
    const wchar_t* DriveLetter,
    const wchar_t* NameOfFileRecord
)
{
    bool bStatus = false;
    bool bWipedAlready = false;

    PATTRIBUTE FilenameAttribHeader = nullptr;
    PATTRIBUTE DataAttributeHeader = nullptr;
    PFILENAME_ATTRIB FileNameAttribute = nullptr;
    PWIPE_INPUT_BUFFER OverwriteBuffer = nullptr;

    PFILE_RECORD FileRecord = reinterpret_cast<PFILE_RECORD>(
        new UINT8[BytesPerFileRecord]()
        );

    wchar_t* FileName = new wchar_t[MAX_PATH * 2]();

    HANDLE hFile = CreateFile(
        ANTFS_DRIVER_NAME,
        GENERIC_WRITE,
        FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (hFile == INVALID_HANDLE_VALUE)
    {
        wsprintf(e, L"[%ws::%d] %ws\n",
            __FUNCTIONW__, __LINE__, LastError());

        return false;
    }

    for (PRUNLIST DataRun : MftRunList)
    {
        UINT64 Offset = DataRun->AbsoluteOffset;

        ///
        /// Adjust the size to take into account the file records. We don't want to count down
        /// from 2048 if there's only two file records.
        /// 
        UINT32 NumberOfRecords = DataRun->ClusterSize / BytesPerFileRecord;
        do
        {
            bStatus = ReadNtfsFrom(Offset, BytesPerFileRecord, FileRecord);
            if (bStatus)
            {
                ///
                /// Ran into a weird issue where the logic for signature was picking up junk, 
                /// like FILE_UPLOAD_LIMITE_PREMIUM_TIER_1. The offset to the update sequence isn't
                /// a good check because it could probably change; instead, I AND it to check if it contains
                /// an underscore. It's a little hacky, but it works.
                /// 
                if (FileRecord->Signature == FILE_SIGNATURE && (FileRecord->UsaOffset & 0xff) != '_' && FileRecord->Flags.value == NULL)
                {
                    bStatus = false;

                    ///
                    /// Check to make sure the USN matches the last two bytes of the record. Then
                    /// NULL them.
                    /// 
                    if (!VerifySector(FileRecord))
                    {
                        std::wcerr << LastError() << std::endl;
                        std::wcerr << "[!] Warning: Usn does not match for record: " << std::hex << Offset << std::endl;
                    }

                    FilenameAttribHeader = GetAttributeFor(AttributeFileName, FileRecord);
                    if (FilenameAttribHeader)
                    {
                        FileNameAttribute = reinterpret_cast<PFILENAME_ATTRIB>(
                            NextEntry(FilenameAttribHeader, FilenameAttribHeader->Resident.ValueOffset)
                            );

                        ///
                        /// Hit some cases where we would receive multibtye characters and would throw off
                        /// the program and offset the structures. This sanity checks by getting rid of the multibyte
                        /// and replacing it with an underscore.
                        /// 
                        for (int i = 0; i < FileNameAttribute->FilenameLength; i++)
                        {
                            if (FileName[i] & 0xff00)
                            {
                                FileName[i] = 0x005f;
                            }
                        }

                        RtlCopyMemory(FileName, FileNameAttribute->Filename, FileNameAttribute->FilenameLength * 2);

                        if (wcscmp(FileName, NameOfFileRecord) == 0)
                        {
                            OverwriteBuffer = new WIPE_INPUT_BUFFER();

                            OverwriteBuffer->DriveLetter = DriveLetter;
                            OverwriteBuffer->FileName = NameOfFileRecord;
                            OverwriteBuffer->NameLength = wcslen(NameOfFileRecord);

                            ///
                            /// Wipe the record of that data. This section will go through and iterate through 
                            /// each data location.
                            /// 
                            DataAttributeHeader = GetAttributeFor(AttributeData, FileRecord);
                            if (DataAttributeHeader)
                            {
                                SIZE_T InputBufferSize = sizeof(WIPE_INPUT_BUFFER);
                                InputBufferSize += wcslen(OverwriteBuffer->DriveLetter);
                                InputBufferSize += OverwriteBuffer->NameLength;

                                if (!DataAttributeHeader->IsNonResident)
                                {
                                    if (DataAttributeHeader->Resident.ValueLength != NULL)
                                    {
                                        OverwriteBuffer->FileOffset = Offset;
                                        OverwriteBuffer->ClusterSize = BytesPerFileRecord;

                                        DeviceIoControl(
                                            hFile, 
                                            IoctlOverwrite, 
                                            OverwriteBuffer, 
                                            InputBufferSize,
                                            NULL, NULL, NULL, NULL
                                        );
                                    }
                                }
                                else
                                {
                                    if (DataAttributeHeader->NonResident.AllocatedSize != NULL)
                                    {
                                        PUINT8 RunListData = reinterpret_cast<PUINT8>(
                                            NextEntry(DataAttributeHeader, DataAttributeHeader->NonResident.RunArrayOffset)
                                            );

                                        UINT64 StartingLcn = 0;
                                        UINT64 DataOffset = 0;

                                        PUINT8 AllocatedData = new UINT8[DataAttributeHeader->NonResident.AllocatedSize]();

                                        do
                                        {
                                            UINT64 AbsoluteOffset = 0;
                                            UINT32 ClusterSize = 0;
                                            UINT32 TempLcn = 0;

                                            ParseRunList(RunListData, &ClusterSize, &TempLcn);

                                            ///
                                            /// If TempLcn is zero, that means we hit a sparse file. Move on.
                                            /// 
                                            if (TempLcn == NULL)
                                            {
                                                continue;
                                            }

                                            ///
                                            /// Calculate the size (in bytes) of the clusters.
                                            /// 
                                            ClusterSize *= BootSector->SectorsPerCluster;
                                            ClusterSize *= BootSector->BytesPerSector;

                                            StartingLcn += TempLcn;

                                            AbsoluteOffset = StartingLcn * BootSector->SectorsPerCluster;
                                            AbsoluteOffset *= BootSector->BytesPerSector;

                                            ///
                                            /// Throughout my entire testing on tons and tons of files on various 
                                            /// drives, it seems the absolute address goes up to only five bytes
                                            /// so mask off the extra byte since UINT64 is eight.
                                            /// 
                                            AbsoluteOffset &= 0xffffffffff;

                                            OverwriteBuffer->FileOffset = AbsoluteOffset;
                                            OverwriteBuffer->ClusterSize = ClusterSize;

                                            DeviceIoControl(
                                                hFile, 
                                                IoctlOverwrite, 
                                                OverwriteBuffer, 
                                                InputBufferSize,
                                                NULL, NULL, NULL, NULL
                                            );

                                            DataOffset += ClusterSize;

                                        } while (*RunListData != NULL);

                                        ///
                                        /// Now that all the locations have been wiped, wipe the record itself.
                                        /// 
                                        OverwriteBuffer->FileOffset = Offset;
                                        OverwriteBuffer->ClusterSize = BytesPerFileRecord;

                                        DeviceIoControl(
                                            hFile,
                                            IoctlOverwrite,
                                            OverwriteBuffer,
                                            InputBufferSize,
                                            NULL, NULL, NULL, NULL
                                        );

                                        RtlSecureZeroMemory(AllocatedData, DataAttributeHeader->NonResident.AllocatedSize);
                                        delete[] AllocatedData;
                                        AllocatedData = nullptr;
                                    }
                                }
                            }


                            if (DataAttributeHeader)
                            {
                                RtlSecureZeroMemory(DataAttributeHeader, DataAttributeHeader->Length);
                                delete[] DataAttributeHeader;
                                DataAttributeHeader = nullptr;
                            }
                        }
                    }
                }
            }

            Offset += BytesPerFileRecord;
        } while (NumberOfRecords--);
    }

    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
        hFile = NULL;
    }

    if (OverwriteBuffer != nullptr)
    {
        RtlSecureZeroMemory(OverwriteBuffer, sizeof(WIPE_INPUT_BUFFER));
        delete OverwriteBuffer;
        OverwriteBuffer = nullptr;
    }
    else
    {
        bStatus = false;

        wsprintf(e, L"[%ws::%d] Unable to find %ws.\n",
            __FUNCTIONW__, __LINE__, NameOfFileRecord);
    }

    if (FileName != nullptr)
    {
        RtlSecureZeroMemory(FileName, MAX_PATH * 2);
        delete[] FileName;
        FileName = nullptr;
    }

    RtlSecureZeroMemory(FileRecord, BytesPerFileRecord);
    delete[] FileRecord;
    FileRecord = nullptr;

    return bStatus;
}

_Use_decl_annotations_
wchar_t* 
antiforensics::what()
{
    return e;
}


/// EOF