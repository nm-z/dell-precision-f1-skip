#include <efi.h>

/* Observer only: records that the firmware loaded this driver and that the
 * Dell A34 error event fired. Installs no protocol, so it cannot change
 * whether the real F1 skip works. */

static EFI_GUID probe_guid = {
    0x6b4d1c2e, 0x1f0a, 0x4d3b,
    {0x9a, 0x2c, 0x5e, 0x7f, 0x1d, 0x2a, 0x3b, 0x4c}};
static EFI_GUID dell_error_group = {
    0x1a4d8e9f, 0xbf05, 0x454b,
    {0x8e, 0x01, 0xb4, 0xaf, 0x9b, 0x86, 0xc0, 0x86}};

static EFI_BOOT_SERVICES *boot_services;
static EFI_RUNTIME_SERVICES *runtime_services;
static EFI_EVENT record_event;

static VOID record(CHAR16 *name) {
  UINT8 one = 1;
  runtime_services->SetVariable(
      name, &probe_guid,
      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS |
          EFI_VARIABLE_RUNTIME_ACCESS,
      sizeof(one), &one);
}

static VOID EFIAPI on_record(EFI_EVENT event, VOID *context) {
  (VOID)event;
  (VOID)context;
  record(L"F1SkipEvent");
}

static VOID EFIAPI on_dell_error(EFI_EVENT event, VOID *context) {
  (VOID)event;
  (VOID)context;
  boot_services->SignalEvent(record_event);
}

EFI_STATUS EFIAPI EfiMain(EFI_HANDLE image, EFI_SYSTEM_TABLE *system_table) {
  EFI_EVENT error_event;
  EFI_STATUS status;

  (VOID)image;
  boot_services = system_table->BootServices;
  runtime_services = system_table->RuntimeServices;
  record(L"F1SkipLoad");
  status = boot_services->CreateEvent(
      EVT_NOTIFY_SIGNAL, TPL_CALLBACK, on_record, NULL, &record_event);
  if (EFI_ERROR(status)) {
    return status;
  }
  status = boot_services->CreateEventEx(
      EVT_NOTIFY_SIGNAL, TPL_NOTIFY, on_dell_error, NULL,
      &dell_error_group, &error_event);
  if (EFI_ERROR(status)) {
    boot_services->CloseEvent(record_event);
  }
  return status;
}
