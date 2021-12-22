#pragma once
#include <ntddk.h>

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
constexpr auto POOLTAG = 'nRhC';
constexpr auto MAX_LEN = 260 * 2;

///---------------------------------------------------------------
/// Enum types
///---------------------------------------------------------------
typedef enum
{
	IoctlOverwrite = ENCODE_IOCTL(FILE_DEVICE_UNKNOWN, 0x900)
} IOCTLS;

///---------------------------------------------------------------
/// Struct Types
///---------------------------------------------------------------
#include <pshpack1.h>
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