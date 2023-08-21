/** @file
  Defines the protocol interface which can be called before the stack cookie interrupt
  when a stack cookie check fails.

**/

#ifndef STACK_COOKIE_CHECK_FAILURE_HOOK_H__
#define STACK_COOKIE_CHECK_FAILURE_HOOK_H__

#define STACK_COOKIE_CHECK_FAILURE_HOOK_PROTOCOL_GUID \
  { \
    0xC1F15632, 0x4E68, 0x4503, { 0x98, 0x6C, 0x61, 0x89, 0x64, 0xD1, 0x1A, 0xB5 } \
  }

/**
 BEEBE TODO
**/
typedef
VOID
(EFIAPI *STACK_COOKIE_CHECK_FAILURE_HOOK)(
  VOID
  );

typedef struct _STACK_COOKIE_CHECK_FAILURE_HOOK_PROTOCOL {
  STACK_COOKIE_CHECK_FAILURE_HOOK     StackCookieCheckFailureHook;
} STACK_COOKIE_CHECK_FAILURE_HOOK_PROTOCOL;

extern EFI_GUID  gStackCookieCheckFailureHookProtocolGuid;

#endif
