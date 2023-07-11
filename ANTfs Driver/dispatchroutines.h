#pragma once
#include <ntifs.h>
#include "typesndefs.h"

// <summary>
// This locates the lower device of NTFS and directly
// overwrites the contents of a file with zeroes, as 
// well as the file record of that file. This is done
// live, on disk.
// </summary>
// <param name="WipeInputBuffer"></param>
// <returns>Returns STATUS_SUCCESS or the 
// appropriate status code.</returns>
_Success_(return >= 0)
NTSTATUS 
OverwriteFileRecord(
	_In_ PWIPE_INPUT_BUFFER WipeInputBuffer
);


// EOF