#pragma once
#include <winnt.h>

///---------------------------------------------------------------
/// Macro(s)
///---------------------------------------------------------------
#define ENCODE_IOCTL(DeviceType, Function)( \
	((DeviceType) << 16 |					\
	((FILE_WRITE_DATA) << 14) |				\
	((Function) << 2) |						\
	(METHOD_BUFFERED))						\
)

///---------------------------------------------------------------
/// Definitions
///---------------------------------------------------------------
constexpr auto ANTFS_DRIVER_NAME = L"\\\\.\\ANTfs";
constexpr auto FILE_SIGNATURE = 'ELIF';
constexpr auto NTFS_BOOT_SECTOR_SIGNATURE = 0xaa55;

///---------------------------------------------------------------
/// Enum types
///---------------------------------------------------------------
typedef enum
{
	IoctlOverwrite = ENCODE_IOCTL(FILE_DEVICE_UNKNOWN, 0x900)
} IOCTLS; 

typedef enum
{
	NamespacePosix,
	NamespaceWin32,
	NamespaceDos,
	NamespaceWin32andDos
} FILENAME_NAMESPACE;

typedef enum
{
	AttributeStandardInformation = 0x10,
	AttributeAttributeList = 0x20,
	AttributeFileName = 0x30,
	AttributeObjectId = 0x40,
	AttributeSecurityDescriptor = 0x50,
	AttributeVolumeName = 0x60,
	AttributeVolumeInformation = 0x70,
	AttributeData = 0x80,
	AttributeIndexRoot = 0x90,
	AttributeIndexAllocation = 0xa0,
	AttributeBitmap = 0xb0,
	AttributeReparsePoint = 0xc0,
	AttributeEAInformation = 0xd0,
	AttributeEA = 0xe0,
	AttributePropertySet = 0xf0,
	AttributeLoggedUtilityStream = 0x100,
	AttributeEnd = 0xffffffff
} ATTRIBUTE_TYPE, * PATTRIBUTE_TYPE;

typedef enum
{
	$Mft,
	$MftMirror,
	$LogFile,
	$Volume,
	$AttrDef,
	$RootIndex,
	$Bitmap,
	$Boot,
	$BadClus,
	$Secure,
	$UpCase,
	$Extend
} SPECIAL_FILE_TYPE, * PSPECIAL_FILE_TYPE;

///---------------------------------------------------------------
/// Struct Types
///---------------------------------------------------------------
#include <pshpack1.h>
typedef struct _BOOT_SECTOR
{
	UINT8 JmpToBootCode[3];
	char OemName[8];
	UINT16 BytesPerSector;
	UINT8 SectorsPerCluster;
	UINT16 ReservedSectors;
	UINT8 Unused0[5];
	UINT8 MediaDescriptor;
	UINT16 Unused1;
	UINT16 SectorsPerTrack;
	UINT16 NumberOfHeads;
	UINT32 HiddenSectors;
	UINT32 Unused2;
	UINT32 Unused3;
	UINT64 TotalSectorsInSystem;
	UINT64 StartingClusterOfMft;
	UINT64 StartingClusterOfMftMirror;
	UINT32 ClustersPerFileRecord;
	UINT32 ClustersPerIndexBlock;
	UINT64 VolumeSerialNumber;
	UINT32 Checksum;
	UINT8 BootstrapCode[426];
	UINT16 Signature;
} BOOT_SECTOR, * PBOOT_SECTOR;

///
/// Using nameless structs so disabling the warnings that prevent the driver from 
/// being compiled. 
/// 
#pragma warning(disable : 4201)
typedef struct _FILE_RECORD
{
	UINT32 Signature;						/// The type of NTFS record (FILE, INDX, BAAD, etc.)
	UINT16 UsaOffset;						/// Offset in bytes to the Update Sequence Array
	UINT16 UsaCount;						/// The number of values in the Update Sequence Array
	UINT64 Lsn;								/// $Logfile Sequence Number 

	UINT16 SequenceNumber;					/// Number of times that the MFT entry has been used
	UINT16 LinkCount;						/// The number of hard links to the MFT entry
	UINT16 AttributeOffset;					/// Offset to the first attribute

	union
	{
		struct
		{
			UINT16 NotInUseNotDirectory : 1;/// bit 0
			UINT16 InUseNotDirectory : 1;	/// bit 1
			UINT16 NotInUseIsDirectory : 1; /// bit 2
			UINT16 InUseIsDirectory : 1;	/// bit 3
		};
		UINT16 value;
	} Flags;

	UINT32 BytesInUse;						/// Number of bytes used by the FILE record
	UINT32 BytesAllocated;					/// Number of bytes allocated for the FILE record
	UINT64 BaseFileRecord;					/// Reference to the base FILE record
	UINT32 NextAttributeNumber;				/// Next attribute id
	UINT32 IdOfThisRecord;					/// Id of this record
	UINT16 Usn;								/// Update Sequence Number
	UINT8 UpdateSequenceArray[3];			/// Update Sequence Array is 4 bytes with 2 bytes padding
} FILE_RECORD, * PFILE_RECORD;

typedef struct _ATTRIBUTE
{
	INT32 AttributeType;
	UINT32 Length;					/// Length of the attribute. If this is the last, it will be 0xffffffff
	UINT8 IsNonResident;			/// If true, this is a nonresident entry (deleted)
	UINT8 NameLength;
	UINT16 NameOffset;

	union
	{
		struct
		{
			UINT16 Compressed : 8;	/// bits 0 - 7
			UINT16 Placeholder : 6; /// bits 8 - 13
			UINT16 Encrypted : 1;	/// bit 14
			UINT16 Sparse : 1;		/// bit 15
		};
		UINT16 value;
	} Flags;

	UINT16 AttributeIdentifier;		/// The number that is unique to this MFT entry

	union
	{
		struct
		{
			UINT32 ValueLength;		/// The size, in bytes, of the attribute value
			UINT16 ValueOffset;		/// The offset, in bytes, to the attribute value
			UINT8 IndexedFlag;		/// The attribute properties. 1 if indexed. 
			UINT8 Padding;
		} Resident;

		struct
		{
			UINT64 LowVcn;			/// The lowest valid Virtual Cluster Number
			UINT64 HighVcn;			/// The highest valid Virtual Cluster Number
			UINT16 RunArrayOffset;	/// The offset, in bytes, to the run array that contains mappings between VCNs and LCNs
			UINT16 CompressionUnit; /// The compression unit for this attribute. If this is 0, it is not compressed
			UINT32 Unused;
			UINT64 AllocatedSize;	/// The size, in bytes, of disk space allocated to hold the attribute value
			UINT64 DataSize;		/// The size, in bytes, of the attribute value. This value may be larger than the AllocatedSize if the attribute value is compressed or sparse
			UINT64 InitializedSize; /// The size, in bytes, of the initialized portion of the attribute value
			UINT64 CompressedSize;	/// This will exist if it's compressed
		} NonResident;
	};
} ATTRIBUTE, * PATTRIBUTE;

typedef struct _FILENAME_ATTRIB
{
	union
	{
		struct
		{
			UINT64 FileRecordNumber : 48;
			UINT64 SequenceNumber : 16;
		};
	} ParentDirectory;

	UINT64 FileCreationTime;
	UINT64 FileModifiedTime;
	UINT64 RecordChanged;
	UINT64 LastAccessedTime;
	UINT64 AllocatedSizeOfData;
	UINT64 RealSizeOfData;
	UINT32 FileAttributes;
	UINT32 Reserved;
	UINT8 FilenameLength;
	UINT8 FilenameNamespace;
	WCHAR Filename[ANYSIZE_ARRAY];
} FILENAME_ATTRIB, * PFILENAME_ATTRIB;

typedef struct _STANDARD_INFORMATION_ATTRIB
{
	UINT64 CreationTime;
	UINT64 ChangeTime;
	UINT64 LastWriteTime;
	UINT64 LastAccessTime;

	union
	{
		struct
		{
			UINT32 ReadOnly : 1;		/// bit 1
			UINT32 Hidden : 1;			/// bit 2
			UINT32 System : 1;			/// bit 3
			UINT32 Padding : 2;			/// bits 4 - 5
			UINT32 Archive : 1;			/// bit 6
			UINT32 Device : 1;			/// bit 7
			UINT32 Normal : 1;			/// bit 8
			UINT32 Temporary : 1;		/// bit 9
			UINT32 SparseFile : 1;		/// bit 10
			UINT32 ReparsePoint : 1;	/// bit 11
			UINT32 Compressed : 1;		/// bit 12
			UINT32 Offline : 1;			/// bit 13
			UINT32 IsContentIndexed : 1;/// bit 14
			UINT32 Encrypted : 1;		/// bit 15
		};
		UINT32 value;
	} FileAttributes;

	UINT32 MaxNumOfVersions;
	UINT32 VersionNumber;
	UINT32 ClassId;
	UINT32 OwnerId;
	UINT32 SecurityId;
	UINT32 QuotaCharged;
	UINT64 UpdateSequenceNumber;
} STANDARD_INFORMATION_ATTRIB, * PSTANDARD_INFORMATION_ATTRIB;

typedef union _RUNLIST_HEADER
{
	struct
	{
		UINT8 ByteLength : 4;
		UINT8 ByteOffset : 4;
	};
	UINT8 value;
} RUNLIST_HEADER, * PRUNLIST_HEADER;

typedef struct _RUNLIST
{
	UINT32 ClusterSize;
	UINT32 LcnOffset;
	UINT64 AbsoluteOffset;
} RUNLIST, * PRUNLIST;

typedef struct _WIPE_INPUT_BUFFER
{
	const wchar_t* DriveLetter;
	SIZE_T NameLength;
	const wchar_t* FileName;
	UINT64 FileOffset;
	UINT32 ClusterSize;
} WIPE_INPUT_BUFFER, * PWIPE_INPUT_BUFFER;
#include <poppack.h>


/// EOF