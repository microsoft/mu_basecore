#ifndef __VARIABLE_FILTER_PROTOCOL_H__
#define __VARIABLE_FILTER_PROTOCOL_H__

// MS_CHANGE_? - ???

///
/// Global ID for the Variable Filter Protocol
///
// {C74F6034-DFBA-4F71-9003-A39A4F11E570}
#define EFI_VARIABLE_FILTER_PROTOCOL_GUID \
  { 0xc74f6034, 0xdfba, 0x4f71, {0x90, 0x3, 0xa3, 0x9a, 0x4f, 0x11, 0xe5, 0x70 } }

extern EFI_GUID gEfiVariableFilterProtocolGuid;

typedef struct _EFI_VARIABLE_FILTER_PROTOCOL EFI_VARIABLE_FILTER_PROTOCOL;

typedef struct {
  EFI_GUID FilterGuid;
  EFI_GET_VARIABLE FilterGetVariable;
  EFI_SET_VARIABLE FilterSetVariable;
} EFI_VARIABLE_FILTER_INTERFACE;

typedef
EFI_STATUS
(EFIAPI *EFI_SET_VARIABLE_FILTER)(
  IN EFI_VARIABLE_FILTER_INTERFACE* Filter
  );

#pragma pack(push, 1)
struct _EFI_VARIABLE_FILTER_PROTOCOL
{
  EFI_SET_VARIABLE_FILTER SetVariableFilter;
};
#pragma pack(pop)

#endif // __VARIABLE_FILTER_PROTOCOL_H__
