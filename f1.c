#include <efi.h>

static EFI_GUID dell_input_guid = {
    0x292df98d, 0x748c, 0x419e,
    {0x8b, 0x6e, 0xfe, 0xce, 0xba, 0x3f, 0xcf, 0x63}};
static EFI_GUID dell_error_group = {
    0x1a4d8e9f, 0xbf05, 0x454b,
    {0x8e, 0x01, 0xb4, 0xaf, 0x9b, 0x86, 0xc0, 0x86}};

typedef BOOLEAN(EFIAPI *DELL_QUERY)(UINT32 selector);

typedef struct {
  volatile UINT8 enabled;
  UINT8 reserved[7];
  DELL_QUERY query;
} DELL_INPUT;

static BOOLEAN EFIAPI query(UINT32 selector);

static DELL_INPUT input = {0, {0}, query};
static EFI_BOOT_SERVICES *boot_services;
static EFI_EVENT cleanup_event;
static volatile BOOLEAN return_true;

_Static_assert(sizeof(DELL_INPUT) == 16, "Dell protocol size");
_Static_assert(__builtin_offsetof(DELL_INPUT, query) == 8,
               "Dell query offset");

static BOOLEAN EFIAPI query(UINT32 selector) {
  BOOLEAN result;

  if (selector != 1 || !input.enabled) {
    return FALSE;
  }
  result = return_true;
  return_true = !return_true;
  return result;
}

static VOID EFIAPI cleanup(EFI_EVENT event, VOID *context) {
  (VOID)event;
  (VOID)context;
  input.enabled = FALSE;
}

static VOID EFIAPI on_dell_error(EFI_EVENT event, VOID *context) {
  (VOID)event;
  (VOID)context;
  return_true = TRUE;
  input.enabled = TRUE;
  if (EFI_ERROR(boot_services->SignalEvent(cleanup_event))) {
    input.enabled = FALSE;
  }
}

EFI_STATUS EFIAPI EfiMain(EFI_HANDLE image, EFI_SYSTEM_TABLE *system_table) {
  EFI_EVENT error_event;
  EFI_HANDLE protocol_handle = NULL;
  EFI_STATUS status;

  (VOID)image;
  boot_services = system_table->BootServices;
  status = boot_services->CreateEvent(
      EVT_NOTIFY_SIGNAL, TPL_CALLBACK, cleanup, NULL, &cleanup_event);
  if (EFI_ERROR(status)) {
    return status;
  }
  status = boot_services->CreateEventEx(
      EVT_NOTIFY_SIGNAL, TPL_NOTIFY, on_dell_error, NULL,
      &dell_error_group, &error_event);
  if (EFI_ERROR(status)) {
    boot_services->CloseEvent(cleanup_event);
    return status;
  }
  status = boot_services->InstallProtocolInterface(
      &protocol_handle, &dell_input_guid, EFI_NATIVE_INTERFACE, &input);
  if (EFI_ERROR(status)) {
    boot_services->CloseEvent(error_event);
    boot_services->CloseEvent(cleanup_event);
  }
  return status;
}
