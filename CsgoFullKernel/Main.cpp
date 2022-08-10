#include "Csgo.h"

void DoBeepCpp()
{
	NTSTATUS status = STATUS_SUCCESS;
	IO_STATUS_BLOCK IoStatus;

	/*
	PDEVICE_OBJECT device = NULL;
	PFILE_OBJECT file = NULL;
	KEVENT event;
	KeInitializeEvent(&event, NotificationEvent, FALSE);

	UNICODE_STRING u;
	RtlInitUnicodeString(&u, L"\\Device\\Beep");
	status = IoGetDeviceObjectPointer(&u, SYNCHRONIZE, &file, &device);

	Print("status is %X", status);

	BEEP_SET_PARAMETERS s;
	s.Frequency = 500;
	s.Duration = 1000;
	PIRP pIrp = IoBuildDeviceIoControlRequest(IOCTL_BEEP_SET, device, &s, sizeof(s), NULL, NULL, FALSE, &event, &IoStatus);
	status = IoCallDriver(device, pIrp);
	Print("status is %X", status);
	Print("IoStatus is %X", IoStatus.Status);
	
	KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

	Print("IoStatus is %X", IoStatus.Status);
	return;
	*/

	UNICODE_STRING deviceName = RTL_CONSTANT_STRING(DD_BEEP_DEVICE_NAME_U);

	OBJECT_ATTRIBUTES objectAttributes;
	InitializeObjectAttributes(&objectAttributes, &deviceName, OBJ_KERNEL_HANDLE | OBJ_OPENIF | OBJ_CASE_INSENSITIVE, NULL, NULL);

	HANDLE beepDriver;
	status = ZwOpenFile(&beepDriver, FILE_READ_ACCESS, &objectAttributes, &IoStatus, NULL, NULL);

	Print("beepDriver handle %p", beepDriver);

	if (!beepDriver)
	{
		Print("Initialization failed !");

		return;
	}

	if (status == STATUS_SUCCESS)
	{
		ULONG returned = NULL;

		BEEP_SET_PARAMETERS BeepSettings;
		BeepSettings.Duration = 1000;
		BeepSettings.Frequency = 500;

		status = ZwDeviceIoControlFile(beepDriver, NULL, NULL, NULL, &IoStatus, IOCTL_BEEP_SET, &BeepSettings, sizeof(BeepSettings), &returned, NULL);
		Print("ZwDeviceIoControlFile status is %X", status);
		Print("IoStatus status is %X", status);

		if (status == STATUS_PENDING)
		{
			status = ZwWaitForSingleObject(beepDriver, FALSE, NULL);
			Print("ZwWaitForSingleObject status is %X", status);
			status = IoStatus.Status;
			Print("IoStatus status is %X", status);
		}
	}
	else
	{
		Print("status is not success %X", status);
	}
}

void MainThread()
{
	Print("doin the thing");
	currentProcess = IoGetCurrentProcess();
	currentThread = KeGetCurrentThread();
	memcpy(&currentCid, (PVOID)((char*)currentThread + cidOffset), sizeof(CLIENT_ID));

	NTSTATUS status = STATUS_SUCCESS;

	while (SaveWhileLoop())
	{
		Print("waiting for csgo.exe...");

		status = GetProcByName("csgo.exe", &targetApplication, 0);
		if (NT_SUCCESS(status))
			break;

		Sleep(1000);
	}

	Print("found csgo");

	Print("getting pid...");
	HANDLE procId = PsGetProcessId(targetApplication);

	if (!procId)
	{
		Print("failed to find proc id");
		ExitThread();
	}
	pid = (ULONG)procId;
	Print("got pid %i", pid);

	int count = 0;
	while (!GetModuleBasex86(targetApplication, L"serverbrowser.dll"))
	{
		SaveWhileLoop();
		if (count >= 30) //wait 30 sec then abort
		{
			Print("failed to get serverbrowser dll\n");
			ExitThread();
		}
		count++;
		Sleep(1000);
	}

	clientBase = GetModuleBasex86(targetApplication, L"client.dll");
	if (!clientBase)
	{
		Print("failed to get clientBase");
		ExitThread();
	}

	engineBase = GetModuleBasex86(targetApplication, L"engine.dll");
	if (!engineBase)
	{
		Print("failed to get engineBase");
		ExitThread();
	}

	CsgoMain();
	ExitThread();
}

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);

	Print("welcome");
	NTSTATUS status = STATUS_SUCCESS;

	status = InitWindowUtils();
	if (!NT_SUCCESS(status))
	{
		Print("failed to init window utils");
		return STATUS_UNSUCCESSFUL;
	}

	status = InitKeyMap();
	if (!NT_SUCCESS(status))
	{
		Print("failed to init keymap");
		return STATUS_UNSUCCESSFUL;
	}

	status = InitMouse(&mouseObject);
	if (!NT_SUCCESS(status))
	{
		Print("failed to init mouse");
		return STATUS_UNSUCCESSFUL;
	}

	status = InitDrawing();
	if (!NT_SUCCESS(status))
	{
		Print("failed to init drawing");
		return STATUS_UNSUCCESSFUL;
	}

	status = StartThread(MainThread);
	if (!NT_SUCCESS(status))
	{
		Print("failed to start thread");
		return STATUS_UNSUCCESSFUL;
	}

	return STATUS_SUCCESS;
}
