#include <ntifs.h>
#include <ntddk.h>

#include "typesndefs.h"
#include "dispatchroutines.h"

UNICODE_STRING g_usDeviceName = RTL_CONSTANT_STRING(L"\\Device\\ANTfs");
UNICODE_STRING g_usSymbolicName = RTL_CONSTANT_STRING(L"\\??\\ANTfs");

_Function_class_(DRIVER_DISPATCH)
_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS 
DriverDispatchRoutine(
	_In_ PDEVICE_OBJECT,
	_In_ PIRP Irp
)
{
	NTSTATUS Status = STATUS_NOT_IMPLEMENTED;

	PWIPE_INPUT_BUFFER UsermodeBuffer = nullptr;

	UINT32 Information = NULL;
	UINT32 IoControlCode = NULL;

	PIO_STACK_LOCATION IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

	IoControlCode = IoStackLocation->Parameters.DeviceIoControl.IoControlCode;

	switch (IoControlCode)
	{
	case IoctlOverwrite:
		__try
		{
			UsermodeBuffer = static_cast<PWIPE_INPUT_BUFFER>(
				Irp->AssociatedIrp.SystemBuffer
				);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			KdPrint(("[%ws::%d] Access violation occured.\n", 
				__FUNCTIONW__, __LINE__));

			return STATUS_ACCESS_VIOLATION;
		}
		Status = OverwriteFileRecord(UsermodeBuffer);
		break;
	default:
		KdPrint(("[%ws::%d] %04x not supported.\n", 
			__FUNCTIONW__, __LINE__, IoControlCode));
		break;
	}

	Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = Information;

	IofCompleteRequest(Irp, IO_NO_INCREMENT);

	return Status;
}

_Function_class_(DRIVER_DISPATCH)
_Dispatch_type_(IRP_MJ_CREATE)
_Dispatch_type_(IRP_MJ_CLOSE)
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
DriverCreateClose(
	_In_ PDEVICE_OBJECT,
	_In_ PIRP Irp
)
{
	Irp->IoStatus.Information = NULL;
	Irp->IoStatus.Status = STATUS_SUCCESS;

	IofCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

_Function_class_(DRIVER_UNLOAD)
_IRQL_requires_(PASSIVE_LEVEL)
VOID
DriverUnload(
	_In_ PDRIVER_OBJECT DriverObject
)
{
	IoDeleteSymbolicLink(&g_usSymbolicName);
	IoDeleteDevice(DriverObject->DeviceObject);

	KdPrint(("[%ws::%d] Completed successfully.\n", __FUNCTIONW__, __LINE__));
}

EXTERN_C
_Function_class_(DRIVER_INITIALIZE)
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
DriverEntry(
	_In_ PDRIVER_OBJECT DriverObject,
	_In_ PUNICODE_STRING
)
{
	PDEVICE_OBJECT DeviceObject = NULL;

	NTSTATUS Status = IoCreateDevice(
		DriverObject,
		NULL,
		&g_usDeviceName,
		FILE_DEVICE_UNKNOWN,
		FILE_DEVICE_SECURE_OPEN,
		TRUE,
		&DeviceObject
	);
	if (!NT_SUCCESS(Status))
	{
		KdPrint(("[%ws::%d] Failed with status: 0x%08x\n", __FUNCTIONW__, __LINE__, Status));

		if (DeviceObject)
		{
			IoDeleteDevice(DeviceObject);
		}
		return Status;
	}

	Status = IoCreateSymbolicLink(&g_usSymbolicName, &g_usDeviceName);
	if (!NT_SUCCESS(Status))
	{
		KdPrint(("[%ws::%d] Failed with status: 0x%08x\n", __FUNCTIONW__, __LINE__, Status));

		IoDeleteDevice(DeviceObject);
		return Status;
	}

	DeviceObject->Flags |= DO_BUFFERED_IO;

	DriverObject->DriverUnload = DriverUnload;
	DriverObject->MajorFunction[IRP_MJ_CREATE] =
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = DriverCreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DriverDispatchRoutine;

	KdPrint(("[%ws::%d] Completed successfully.\n", __FUNCTIONW__, __LINE__));

	return Status;
}


// EOF