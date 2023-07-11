#include "dispatchroutines.h"

_Use_decl_annotations_
NTSTATUS 
OverwriteFileRecord(
	PWIPE_INPUT_BUFFER WipeInputBuffer
)
{
	NTSTATUS Status = STATUS_MEMORY_NOT_ALLOCATED;
	KEVENT EventObj;

	UNICODE_STRING usDriveDirectory = RTL_CONSTANT_STRING(L"\\??");
	UNICODE_STRING usDriveLetter = { 0 };
	UNICODE_STRING SymbolicLinkName = { 0 };

	HANDLE hDirectoryHandle = NULL;
	HANDLE hFileHandle = NULL;

	PFILE_OBJECT FileObject = nullptr;
	PDEVICE_OBJECT DiskDeviceObject = nullptr;
	PIRP Irp = nullptr;
	PIO_STACK_LOCATION IoStackLocation = nullptr;

	OBJECT_ATTRIBUTES oa = { 0 };
	IO_STATUS_BLOCK io = { 0 };
	LARGE_INTEGER StartingOffset = { 0 };

	KeInitializeEvent(&EventObj, NotificationEvent, FALSE);

	RtlInitUnicodeString(&usDriveLetter, WipeInputBuffer->DriveLetter);

	PUINT8 ZeroBuffer = static_cast<PUINT8>(
		ExAllocatePoolWithTag(
			NonPagedPoolNx,
			WipeInputBuffer->ClusterSize, 
			POOLTAG
		)
		);

	SymbolicLinkName.Buffer = static_cast<PWCH>(
		ExAllocatePoolWithTag(NonPagedPoolNx, MAX_LEN, POOLTAG)
		);
	if (ZeroBuffer == NULL || SymbolicLinkName.Buffer == NULL)
	{
		KdPrint(("[%ws::%d] Failed with status: %08x.\n", 
			__FUNCTIONW__, __LINE__, Status));
		return Status;
	}
	else
	{
		RtlSecureZeroMemory(ZeroBuffer, WipeInputBuffer->ClusterSize);
		RtlSecureZeroMemory(SymbolicLinkName.Buffer, MAX_LEN);

		SymbolicLinkName.MaximumLength = MAX_LEN;
	}

	InitializeObjectAttributes(
		&oa, 
		&usDriveDirectory, 
		OBJ_CASE_INSENSITIVE, 
		NULL, NULL
	);

	Status = ZwOpenDirectoryObject(&hDirectoryHandle, DIRECTORY_QUERY, &oa);
	if (!NT_SUCCESS(Status))
	{
		KdPrint(("[%ws::%d] Failed with status: %08x.\n", 
			__FUNCTIONW__, __LINE__, Status));
		goto CleanupAndExit;
	}
	else
	{
		KdPrint(("[%ws::%d] Opened a handle to %wZ.\n", 
			__FUNCTIONW__, __LINE__, usDriveDirectory));
	}

	InitializeObjectAttributes(
		&oa, 
		&usDriveLetter, 
		OBJ_CASE_INSENSITIVE, 
		hDirectoryHandle, 
		NULL
	);

	Status = ZwOpenSymbolicLinkObject(&hDirectoryHandle, SYMBOLIC_LINK_QUERY, &oa);
	if (!NT_SUCCESS(Status))
	{
		KdPrint(("[%ws::%d] Failed with status: %08x.\n", 
			__FUNCTIONW__, __LINE__, Status));
		goto CleanupAndExit;
	}

	Status = ZwQuerySymbolicLinkObject(hDirectoryHandle, &SymbolicLinkName, NULL);
	if (!NT_SUCCESS(Status))
	{
		KdPrint(("[%ws::%d] Failed with status: %08x.\n",
			__FUNCTIONW__, __LINE__, Status));
		goto CleanupAndExit;
	}
	else
	{
		KdPrint(("[%ws::%d] Opened symbolic link to %wZ.\n", 
			__FUNCTIONW__, __LINE__, SymbolicLinkName));
	}

	Status = ZwOpenFile(
		&hFileHandle,
		GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
		&oa,
		&io,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		FILE_SYNCHRONOUS_IO_NONALERT
	);
	if (!NT_SUCCESS(Status) || hFileHandle == NULL)
	{
		KdPrint(("[%ws::%d] Failed with status: %08x.\n", 
			__FUNCTIONW__, __LINE__, Status));
		goto CleanupAndExit;
	}

	Status = ObReferenceObjectByHandle(
		hFileHandle, 
		FILE_READ_DATA | FILE_WRITE_DATA, 
		NULL, 
		KernelMode, 
		reinterpret_cast<PVOID*>(&FileObject), 
		NULL
	);
	if (NT_SUCCESS(Status))
	{
		DiskDeviceObject = FileObject->DeviceObject;
		if (DiskDeviceObject->Vpb != NULL)
		{
			if (DiskDeviceObject->Vpb->RealDevice != NULL)
			{
				DiskDeviceObject = DiskDeviceObject->Vpb->RealDevice;
			}
		}
		ObDereferenceObject(FileObject);
	}
	else
	{
		KdPrint(("[%ws::%d] Unable to reference handle.\n", 
			__FUNCTIONW__, __LINE__));
		goto CleanupAndExit;
	}

	StartingOffset.QuadPart = WipeInputBuffer->FileOffset;

	Irp = IoBuildSynchronousFsdRequest(
		IRP_MJ_WRITE,
		DiskDeviceObject,
		ZeroBuffer,
		WipeInputBuffer->ClusterSize,
		&StartingOffset,
		&EventObj,
		&io
	);
	if (Irp == NULL)
	{
		Status = STATUS_INSUFFICIENT_RESOURCES;

		KdPrint(("[%ws::%d] Failed with status: %08x.\n", 
			__FUNCTIONW__, __LINE__, Status));
		goto CleanupAndExit;
	}

	IoStackLocation = IoGetNextIrpStackLocation(Irp);
	IoStackLocation->Flags |= SL_OVERRIDE_VERIFY_VOLUME;
	IoStackLocation->Flags |= SL_FORCE_DIRECT_WRITE;

	Status = IoCallDriver(DiskDeviceObject, Irp);
	if (!NT_SUCCESS(Status))
	{
		KdPrint(("[%ws::%d] Failed with status: %08x.\n", 
			__FUNCTIONW__, __LINE__, Status));
		goto CleanupAndExit;
	}

	if (Status == STATUS_PENDING)
	{
		KeWaitForSingleObject(&EventObj, Executive, KernelMode, FALSE, NULL);
		Status = io.Status;
	}

CleanupAndExit:
	if (ZeroBuffer != NULL)
	{
		RtlSecureZeroMemory(ZeroBuffer, WipeInputBuffer->ClusterSize);
		ExFreePoolWithTag(ZeroBuffer, POOLTAG);
	}

	if (SymbolicLinkName.Buffer != NULL)
	{
		RtlSecureZeroMemory(SymbolicLinkName.Buffer, MAX_LEN);
		ExFreePoolWithTag(SymbolicLinkName.Buffer, POOLTAG);
	}

	if (DiskDeviceObject != NULL)
	{
		ObDereferenceObject(DiskDeviceObject);
	}

	if (hFileHandle != NULL)
	{
		ZwClose(hFileHandle);
		hFileHandle = NULL;
	}

	if (hDirectoryHandle != NULL)
	{
		ZwClose(hDirectoryHandle);
		hDirectoryHandle = NULL;
	}

	return Status;
}


// EOF