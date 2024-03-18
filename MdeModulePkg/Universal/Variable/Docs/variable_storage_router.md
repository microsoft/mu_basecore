# UEFI Variable Storage Router

- [UEFI Variable Storage Router](#uefi-variable-storage-router)
  - [Introduction](#introduction)
  - [Design](#design)
  - [Theory of Operation](#theory-of-operation)
    - [High-Level Flow](#high-level-flow)
    - [FVB Example Flow](#fvb-example-flow)
  - [DXE \& SMM Interfaces](#dxe--smm-interfaces)
    - [Variable Storage Protocol](#variable-storage-protocol)
      - [`EDKII_VARIABLE_STORAGE_GET_ID`: `GetId()`](#edkii_variable_storage_get_id-getid)
        - [`EDKII_VARIABLE_STORAGE_GET_ID`: Parameters](#edkii_variable_storage_get_id-parameters)
        - [`EDKII_VARIABLE_STORAGE_GET_ID`: Return Value](#edkii_variable_storage_get_id-return-value)
      - [`EDKII_VARIABLE_STORAGE_GET_VARIABLE`: `GetVariable()`](#edkii_variable_storage_get_variable-getvariable)
        - [`EDKII_VARIABLE_STORAGE_GET_VARIABLE`: Parameters](#edkii_variable_storage_get_variable-parameters)
        - [`EDKII_VARIABLE_STORAGE_GET_VARIABLE`: Return Value](#edkii_variable_storage_get_variable-return-value)
      - [`EDKII_VARIABLE_STORAGE_GET_AUTHENTICATED_VARIABLE`: `GetAuthenticatedVariable()`](#edkii_variable_storage_get_authenticated_variable-getauthenticatedvariable)
        - [`EDKII_VARIABLE_STORAGE_GET_AUTHENTICATED_VARIABLE`: Parameters](#edkii_variable_storage_get_authenticated_variable-parameters)
        - [`EDKII_VARIABLE_STORAGE_GET_AUTHENTICATED_VARIABLE`: Return Value](#edkii_variable_storage_get_authenticated_variable-return-value)
      - [`EDKII_VARIABLE_STORAGE_GET_NEXT_VARIABLE_NAME`: `GetNextVariableName()`](#edkii_variable_storage_get_next_variable_name-getnextvariablename)
        - [`EDKII_VARIABLE_STORAGE_GET_NEXT_VARIABLE_NAME`: Parameters](#edkii_variable_storage_get_next_variable_name-parameters)
        - [`EDKII_VARIABLE_STORAGE_GET_NEXT_VARIABLE_NAME`: Return Value](#edkii_variable_storage_get_next_variable_name-return-value)
      - [`EDKII_VARIABLE_STORAGE_GET_STORAGE_USAGE`: `GetStorageUsage()`](#edkii_variable_storage_get_storage_usage-getstorageusage)
        - [`EDKII_VARIABLE_STORAGE_GET_STORAGE_USAGE`: Parameters](#edkii_variable_storage_get_storage_usage-parameters)
        - [`EDKII_VARIABLE_STORAGE_GET_STORAGE_USAGE`: Return Value](#edkii_variable_storage_get_storage_usage-return-value)
      - [`EDKII_VARIABLE_STORAGE_GET_AUTHENTICATED_SUPPORT`:`GetAuthenticatedSupport()`](#edkii_variable_storage_get_authenticated_supportgetauthenticatedsupport)
        - [`EDKII_VARIABLE_STORAGE_GET_AUTHENTICATED_SUPPORT`: Parameters](#edkii_variable_storage_get_authenticated_support-parameters)
        - [`EDKII_VARIABLE_STORAGE_GET_AUTHENTICATED_SUPPORT`: Return Value](#edkii_variable_storage_get_authenticated_support-return-value)
      - [`EDKII_VARIABLE_STORAGE_WRITE_SERVICE_IS_READY`: `WriteServiceIsReady()`](#edkii_variable_storage_write_service_is_ready-writeserviceisready)
        - [`EDKII_VARIABLE_STORAGE_WRITE_SERVICE_IS_READY`: Parameters](#edkii_variable_storage_write_service_is_ready-parameters)
        - [`EDKII_VARIABLE_STORAGE_WRITE_SERVICE_IS_READY`: Return Value](#edkii_variable_storage_write_service_is_ready-return-value)
      - [`EDKII_VARIABLE_STORAGE_SET_VARIABLE`: `SetVariable()`](#edkii_variable_storage_set_variable-setvariable)
        - [`EDKII_VARIABLE_STORAGE_SET_VARIABLE`: Parameters](#edkii_variable_storage_set_variable-parameters)
        - [`EDKII_VARIABLE_STORAGE_SET_VARIABLE`: Return Value](#edkii_variable_storage_set_variable-return-value)
      - [`EDKII_VARIABLE_STORAGE_GARBAGE_COLLECT`: `GarbageCollect()`](#edkii_variable_storage_garbage_collect-garbagecollect)
        - [`EDKII_VARIABLE_STORAGE_GARBAGE_COLLECT`: Parameters](#edkii_variable_storage_garbage_collect-parameters)
        - [`EDKII_VARIABLE_STORAGE_GARBAGE_COLLECT`: Return Value](#edkii_variable_storage_garbage_collect-return-value)
      - [`EDKII_VARIABLE_STORAGE_PROTOCOL`](#edkii_variable_storage_protocol)
        - [`EDKII_VARIABLE_STORAGE_PROTOCOL`: Members](#edkii_variable_storage_protocol-members)
        - [`EDKII_VARIABLE_STORAGE_PROTOCOL`: Code Example](#edkii_variable_storage_protocol-code-example)
    - [Variable Storage Selector Protocol](#variable-storage-selector-protocol)
      - [`EDKII_VARIABLE_STORAGE_SELECTOR_GET_ID`: `GetId()`](#edkii_variable_storage_selector_get_id-getid)
        - [`EDKII_VARIABLE_STORAGE_SELECTOR_GET_ID`: Parameters](#edkii_variable_storage_selector_get_id-parameters)
        - [`EDKII_VARIABLE_STORAGE_SELECTOR_GET_ID`: Return Value](#edkii_variable_storage_selector_get_id-return-value)
      - [`EDKII_VARIABLE_STORAGE_SELECTOR_PROTOCOL`](#edkii_variable_storage_selector_protocol)
        - [`EDKII_VARIABLE_STORAGE_SELECTOR_PROTOCOL`: Members](#edkii_variable_storage_selector_protocol-members)
        - [`EDKII_VARIABLE_STORAGE_SELECTOR_PROTOCOL`: Code Example](#edkii_variable_storage_selector_protocol-code-example)
    - [Variable Storage Support Protocol](#variable-storage-support-protocol)
      - [`EDKII_VARIABLE_STORAGE_SUPPORT_NOTIFY_WRITE_SERVICE_READY`: `NotifyWriteServiceReady()`](#edkii_variable_storage_support_notify_write_service_ready-notifywriteserviceready)
        - [`EDKII_VARIABLE_STORAGE_SUPPORT_NOTIFY_WRITE_SERVICE_READY`: Parameters](#edkii_variable_storage_support_notify_write_service_ready-parameters)
        - [`EDKII_VARIABLE_STORAGE_SUPPORT_NOTIFY_WRITE_SERVICE_READY`: Return Value](#edkii_variable_storage_support_notify_write_service_ready-return-value)
      - [`EDKII_VARIABLE_STORAGE_SUPPORT_PROTOCOL`](#edkii_variable_storage_support_protocol)
        - [`EDKII_VARIABLE_STORAGE_SUPPORT_PROTOCOL`: Members](#edkii_variable_storage_support_protocol-members)
        - [`EDKII_VARIABLE_STORAGE_SUPPORT_PROTOCOL`: Code Example](#edkii_variable_storage_support_protocol-code-example)
  - [PEI Interfaces](#pei-interfaces)
    - [Variable Storage PPI](#variable-storage-ppi)
      - [`PEI_EDKII_VARIABLE_STORAGE_GET_ID`: `GetId()`](#pei_edkii_variable_storage_get_id-getid)
        - [`PEI_EDKII_VARIABLE_STORAGE_GET_ID`: Parameters](#pei_edkii_variable_storage_get_id-parameters)
        - [`PEI_EDKII_VARIABLE_STORAGE_GET_ID`: Return Value](#pei_edkii_variable_storage_get_id-return-value)
      - [`PEI_EDKII_VARIABLE_STORAGE_GET_VARIABLE`: `GetVariable()`](#pei_edkii_variable_storage_get_variable-getvariable)
        - [`PEI_EDKII_VARIABLE_STORAGE_GET_VARIABLE`: Parameters](#pei_edkii_variable_storage_get_variable-parameters)
        - [`PEI_EDKII_VARIABLE_STORAGE_GET_VARIABLE`: Return Value](#pei_edkii_variable_storage_get_variable-return-value)
      - [`PEI_EDKII_VARIABLE_STORAGE_GET_NEXT_VARIABLE_NAME`: `GetNextVariableName()`](#pei_edkii_variable_storage_get_next_variable_name-getnextvariablename)
        - [`PEI_EDKII_VARIABLE_STORAGE_GET_NEXT_VARIABLE_NAME`: Parameters](#pei_edkii_variable_storage_get_next_variable_name-parameters)
        - [`PEI_EDKII_VARIABLE_STORAGE_GET_NEXT_VARIABLE_NAME`: Return Value](#pei_edkii_variable_storage_get_next_variable_name-return-value)
      - [`EDKII_VARIABLE_STORAGE_PPI`](#edkii_variable_storage_ppi)
        - [Members](#members)
        - [Code Example](#code-example)
    - [Variable Storage Selector PPI](#variable-storage-selector-ppi)
      - [`PEI_EDKII_VARIABLE_STORAGE_SELECTOR_GET_ID`: `GetId()`](#pei_edkii_variable_storage_selector_get_id-getid)
        - [`PEI_EDKII_VARIABLE_STORAGE_SELECTOR_GET_ID`: Parameters](#pei_edkii_variable_storage_selector_get_id-parameters)
        - [`PEI_EDKII_VARIABLE_STORAGE_SELECTOR_GET_ID`: Return Value](#pei_edkii_variable_storage_selector_get_id-return-value)
      - [`EDKII_VARIABLE_STORAGE_SELECTOR_PPI`](#edkii_variable_storage_selector_ppi)
        - [`EDKII_VARIABLE_STORAGE_SELECTOR_PPI`: Members](#edkii_variable_storage_selector_ppi-members)
        - [`EDKII_VARIABLE_STORAGE_SELECTOR_PPI`: Code Example](#edkii_variable_storage_selector_ppi-code-example)

## Introduction

The UEFI Specification describes an interface between the operating system (OS) and platform firmware. A UEFI
Specification compliant system must implement two high-level sets of services - Boot Services which consist of
functions available prior to a successful call to ``EFI_BOOT_SERVICES.ExitBootServices()`` and Runtime Services which
consist of functions that are available before and after any call to ``EFI_BOOT_SERVICES.ExitBootServices()``.

A fundamental Runtime Service is called the UEFI variable services. These services are comprised of an API that the
platform firmware must implement to satisfy the relevant API requirements defined in the UEFI Specification. While the
underlying implementation is platform-specific, the callers will include both the operating system and firmware
components.

While the UEFI Specification only prescribes requirements on the interfaces defined at the OS/Firmware layer, certain
rigidity has manifested within the industry standard implementation of UEFI varible services in the TianoCore EDK II
project. In particular, non-volatile variable storage has been implemented to tightly couple Firmware Volume Block
(FVB) dependencies directly in the core EDK II UEFI variable drivers. During the 15 year lifetime of these drivers,
new storage technologies have emerged that are not compatible with the FVB model.

1. New storage technologies have come to market.
2. Device trends have shifted to low-power ultra-mobile devices and cloud server systems.
3. New offload engines like BMC and specialized security processors are capable of managing non-volatile storage.
4. Platform designs have evolved to include multiple non-volatile storage areas.

To accommodate these changes, a design called the "UEFI variable storage router" is used to decouple the underlying
storage technology from common UEFI variable business logic.

At present, this design seeks full compliance with the UEFI Specification, as written. Therefore, it neglects some more
advanced features that would broaden storage hardware compatibility at the expense of UEFI Specification compliance.
The advantage of this approach is that it manages complexity - increasing flexibility with sufficient guard rails in
place to prevent platform usage from becoming to unwieldy.

For example, platforms are known today to not honor the UEFI Specification's requirement that non-volatile variables
be committed to storage before returning success from a `SetVariable()` call. While this design does not prevent this
behavior, it does not introduce new capabilities that explicitly enable it.

## Design

## Theory of Operation

TODO: Will put more diagrams and a more detailed explanation here later.

### High-Level Flow

![High-Level Variable Router Flow](var_router_high_level.png)

### FVB Example Flow

![FVB Example Variable Router Flow](var_router_fvb_example.png)

## DXE & SMM Interfaces

### Variable Storage Protocol

Abstracts non-volatile media storage access details from the EDK II UEFI variable driver. A platform may produce an
arbitrary number of variable storage protocols that are used by the variable driver for variable management.

After all variable storage protocols have been installed, the platform must install a single variable storage selector
protocol to signal that the platform is ready to initialize UEFI variable services.

---

- Produced by: A variable storage driver
- Consumed by: The EDK II UEFI variable driver

---

#### `EDKII_VARIABLE_STORAGE_GET_ID`: `GetId()`

Retrieves a protocol instance-specific GUID.

Returns a unique GUID per VARIABLE_STORAGE_PROTOCOL instance.

##### `EDKII_VARIABLE_STORAGE_GET_ID`: Parameters

- `This`: A pointer to this instance of the `EDKII_VARIABLE_STORAGE_PROTOCOL`.
- `InstanceGuid`: A pointer to an `EFI_GUID` that is this protocol instance's GUID.

##### `EDKII_VARIABLE_STORAGE_GET_ID`: Return Value

- `EFI_SUCCESS`: The data was returned successfully.
- `EFI_INVALID_PARAMETER`: A required parameter is NULL.

#### `EDKII_VARIABLE_STORAGE_GET_VARIABLE`: `GetVariable()`

Retrieves a variable's value using its name and GUID.

Reads the specified variable from the UEFI variable store. If the Data buffer is too small to hold the contents of the
variable, the error `EFI_BUFFER_TOO_SMALL` is returned and `DataSize` is set to the required buffer size to obtain the
data.

##### `EDKII_VARIABLE_STORAGE_GET_VARIABLE`: Parameters

- `This`: A pointer to this instance of the `EDKII_VARIABLE_STORAGE_PROTOCOL`.
- `AtRuntime`: TRUE if the platform is in OS Runtime, FALSE if still in Pre-OS stage.
- `FromSmm`: TRUE if `GetVariable()` is being called by SMM code, FALSE if called by DXE code.
- `VariableName`: A pointer to a null-terminated string that is the variable's name.
- `VariableGuid`: A pointer to an `EFI_GUID` that is the variable's vendor GUID. The combination of `VariableGuid` and
  `VariableName` must be unique.
- `Attributes` (optional): If non-NULL, on return, points to the variable's attributes.
- `DataSize`: On entry, points to the size in bytes of the `Data` buffer. On return, points to the size of the data
  returned in `Data`.
- `Data`: Points to the buffer which will hold the returned variable value.

##### `EDKII_VARIABLE_STORAGE_GET_VARIABLE`: Return Value

- `EFI_SUCCESS`: The variable was read successfully.
- `EFI_NOT_FOUND`: The variable could not be found.
- `EFI_BUFFER_TOO_SMALL`: The `DataSize` is too small for the resulting data. `DataSize` is updated with the size
  required for the specified variable.
- `EFI_INVALID_PARAMETER`: `VariableName`, `VariableGuid`, `DataSize`, or `Data` is NULL.
- `EFI_DEVICE_ERROR`: The variable could not be retrieved because of a device error.

#### `EDKII_VARIABLE_STORAGE_GET_AUTHENTICATED_VARIABLE`: `GetAuthenticatedVariable()`

Retrieves an authenticated variable's value using its name and GUID.

Reads the specified authenticated variable from the UEFI variable store. If the Data buffer is too small to hold the
contents of the variable, the error `EFI_BUFFER_TOO_SMALL` is returned and `DataSize` is set to the required buffer
size to obtain the data.

##### `EDKII_VARIABLE_STORAGE_GET_AUTHENTICATED_VARIABLE`: Parameters

- `This`: A pointer to this instance of the `EDKII_VARIABLE_STORAGE_PROTOCOL`.
- `AtRuntime`: TRUE if the platform is in OS Runtime, FALSE if still in Pre-OS stage.
- `FromSmm`: TRUE if `GetVariable()` is being called by SMM code, FALSE if called by DXE code.
- `VariableName`: A pointer to a null-terminated string that is the variable's name.
- `VariableGuid`: A pointer to an `EFI_GUID` that is the variable's GUID. The combination of `VariableGuid` and
  `VariableName` must be unique.
- `Attributes` (optional): If non-NULL, on return, points to the variable's attributes.
- `DataSize`: On entry, points to the size in bytes of the `Data` buffer. On return, points to the size of the data
  returned in `Data`.
- `Data`: Points to the buffer which will hold the returned variable value.
- `KeyIndex`: Index of associated public key in database.
- `MonotonicCount`: Associated monotonic count value to protect against replay attack.
- `TimeStamp`: Associated TimeStamp value to protect against replay attack.

##### `EDKII_VARIABLE_STORAGE_GET_AUTHENTICATED_VARIABLE`: Return Value

- `EFI_SUCCESS`: The variable was read successfully.
- `EFI_NOT_FOUND`: The variable could not be found.
- `EFI_BUFFER_TOO_SMALL`: The `DataSize` is too small for the resulting data. `DataSize` is updated with the size
  required for the specified variable.
- `EFI_INVALID_PARAMETER`: `VariableName`, `VariableGuid`, `DataSize`, or `Data` is NULL.
- `EFI_DEVICE_ERROR`: The variable could not be retrieved because of a device error.

#### `EDKII_VARIABLE_STORAGE_GET_NEXT_VARIABLE_NAME`: `GetNextVariableName()`

Return the next variable name and GUID.

This function is called multiple times to retrieve the `VariableName` and `VariableGuid` of all variables currently
available in the system. On each call, the previous results are passed into the interface, and, on return, the
interface returns the data for the next interface. When the entire variable list has been returned, `EFI_NOT_FOUND` is
returned.

##### `EDKII_VARIABLE_STORAGE_GET_NEXT_VARIABLE_NAME`: Parameters

- `This`: A pointer to this instance of the `EDKII_VARIABLE_STORAGE_PROTOCOL`.
- `VariableNameSize`: On entry, points to the size of the buffer pointed to by `VariableName`. On return, the size of
  the variable name buffer.
- `VariableName`: On entry, a pointer to a null-terminated string that is the variable's name. On return, points to the
  next variable's null-terminated name string.
- `VariableGuid`: On entry, a pointer to an `EFI_GUID` that is the variable's GUID. On return, a pointer to the next
  variable's GUID.
- `VariableAttributes`: A pointer to the variable attributes.

##### `EDKII_VARIABLE_STORAGE_GET_NEXT_VARIABLE_NAME`: Return Value

- `EFI_SUCCESS`: The variable was read successfully.
- `EFI_NOT_FOUND`: The variable could not be found.
- `EFI_BUFFER_TOO_SMALL`: The `VariableNameSize` is too small for the resulting data. `VariableNameSize` is updated
  with the size required for the specified variable.
- `EFI_INVALID_PARAMETER`: `VariableName`, `VariableGuid`, or `VariableNameSize` is NULL.
- `EFI_DEVICE_ERROR`: The variable could not be retrieved because of a device error.

#### `EDKII_VARIABLE_STORAGE_GET_STORAGE_USAGE`: `GetStorageUsage()`

Returns information on the amount of space available in the variable store. If the amount of data that can be written
depends on if the platform is in Pre-OS stage or OS stage, the AtRuntime parameter should be used to compute usage.

##### `EDKII_VARIABLE_STORAGE_GET_STORAGE_USAGE`: Parameters

- `This`: A pointer to this instance of the `EDKII_VARIABLE_STORAGE_PROTOCOL`.
- `AtRuntime`: TRUE if the platform is in OS Runtime, FALSE if still in Pre-OS stage.
- `VariableStoreSize`: On return, points to the total size of the NV storage. Indicates the maximum amount of data that
  can be stored in this NV storage area.
- `CommonVariablesTotalSize`: On return, points to the total combined size of all the common UEFI variables that are
  stored in this NV storage area. Excludes variables with the EFI_VARIABLE_HARDWARE_ERROR_RECORD attribute set.
- `HwErrVariablesTotalSize`: On return, points to the total combined size of all the UEFI variables that have the
  EFI_VARIABLE_HARDWARE_ERROR_RECORD attribute set and which are stored in this NV storage area. Excludes all other
  variables.

##### `EDKII_VARIABLE_STORAGE_GET_STORAGE_USAGE`: Return Value

- `EFI_SUCCESS`: Space information was returned successfully.
- `EFI_INVALID_PARAMETER`: Any of the given parameters are NULL.

#### `EDKII_VARIABLE_STORAGE_GET_AUTHENTICATED_SUPPORT`:`GetAuthenticatedSupport()`

Returns whether this NV storage area supports storing authenticated variables.

##### `EDKII_VARIABLE_STORAGE_GET_AUTHENTICATED_SUPPORT`: Parameters

- `This`: A pointer to this instance of the `EDKII_VARIABLE_STORAGE_PROTOCOL`.
- `AuthSupported`: On return, points to a boolean value indicating whether this NV storage area can store authenticated
  variables.

##### `EDKII_VARIABLE_STORAGE_GET_AUTHENTICATED_SUPPORT`: Return Value

- `EFI_SUCCESS`: `AuthSupported` was returned successfully.
- `EFI_INVALID_PARAMETER`: A pointer parameter is NULL.

#### `EDKII_VARIABLE_STORAGE_WRITE_SERVICE_IS_READY`: `WriteServiceIsReady()`

Returns whether this NV storage area is ready to accept calls to SetVariable().

##### `EDKII_VARIABLE_STORAGE_WRITE_SERVICE_IS_READY`: Parameters

- `This`: A pointer to this instance of the `EDKII_VARIABLE_STORAGE_PROTOCOL`.

##### `EDKII_VARIABLE_STORAGE_WRITE_SERVICE_IS_READY`: Return Value

- `TRUE`: The NV storage area is ready to accept calls to SetVariable().
- `FALSE`: The NV storage area is not ready to accept calls to SetVariable().

#### `EDKII_VARIABLE_STORAGE_SET_VARIABLE`: `SetVariable()`

This code sets a variable's value using its name and GUID.

Caution: This function may receive untrusted input.

- This function may be invoked in SMM mode and will be given untrusted input data.
- This function must perform basic validation before parsing the data.
- This function must parse the authentication carefully to prevent security issues, like buffer overflow and integer
  overflow.
- This function must check variable attributes carefully to prevent authentication bypass.

##### `EDKII_VARIABLE_STORAGE_SET_VARIABLE`: Parameters

- `This`: A pointer to this instance of the `EDKII_VARIABLE_STORAGE_PROTOCOL`.
- `AtRuntime`: TRUE if the platform is in OS Runtime, FALSE if still in Pre-OS stage.
- `FromSmm`: TRUE if `SetVariable()` is being called by SMM code, FALSE if called by DXE code.
- `VariableName`: Name of Variable to be found.
- `VendorGuid`: Variable vendor GUID.
- `Attributes`: Attribute value of the variable found.
- `DataSize`: Size of Data found. If size is less than the data, this value contains the required size.
- `Data`: Data pointer.
- `KeyIndex`: If writing an authenticated variable, the public key index.
- `MonotonicCount`: If writing a monotonic counter authenticated variable, the counter value.
- `TimeStamp`: If writing a timestamp authenticated variable, the timestamp value.

##### `EDKII_VARIABLE_STORAGE_SET_VARIABLE`: Return Value

- `EFI_SUCCESS`: The variable was set successfully.
- `EFI_INVALID_PARAMETER`: An invalid parameter was given.
- `EFI_OUT_OF_RESOURCES`: Insufficient resources to set the variable.
- `EFI_WRITE_PROTECTED`: The variable is read-only.

#### `EDKII_VARIABLE_STORAGE_GARBAGE_COLLECT`: `GarbageCollect()`

Performs variable store garbage collection/reclaim operation.

##### `EDKII_VARIABLE_STORAGE_GARBAGE_COLLECT`: Parameters

- `This`: A pointer to this instance of the `EDKII_VARIABLE_STORAGE_PROTOCOL`.

##### `EDKII_VARIABLE_STORAGE_GARBAGE_COLLECT`: Return Value

- `EFI_SUCCESS`: The garbage collection operation was successful.
- `EFI_INVALID_PARAMETER`: An invalid parameter was given.
- `EFI_OUT_OF_RESOURCES`: Insufficient resources to complete garbage collection.
- `EFI_NOT_READY`: Write services or garbage collection, in general, are not ready.
- `EFI_WRITE_PROTECTED`: Write services are not available at this time.

#### `EDKII_VARIABLE_STORAGE_PROTOCOL`

##### `EDKII_VARIABLE_STORAGE_PROTOCOL`: Members

- `GetId()`: Retrieves a protocol instance-specific GUID.
- `GetVariable()`: Retrieves a variable's value given its name and GUID.
- `GetAuthenticatedVariable()`: Retrieves an authenticated variable's value given its name and GUID.
- `GetNextVariableName()`: Return the next variable name and GUID.
- `GetStorageUsage()`: Returns information on storage usage in the variable store.
- `GetAuthenticatedSupport()`: Returns whether this non-volatile storage area supports authenticated variables.
- `SetVariable()`: Sets a variable's value using its name and GUID.
- `WriteServiceIsReady()`: Indicates if `SetVariable()` is ready or not.
- `GarbageCollect()`: Performs variable store garbage collection/reclaim operation.

##### `EDKII_VARIABLE_STORAGE_PROTOCOL`: Code Example

```c
///
/// Variable Storage Protocol
///
struct _EDKII_VARIABLE_STORAGE_PROTOCOL {
  EDKII_VARIABLE_STORAGE_GET_ID                        GetId;                             ///< Retrieves a protocol instance-specific GUID
  EDKII_VARIABLE_STORAGE_GET_VARIABLE                  GetVariable;                       ///< Retrieves a variable's value given its name and GUID
  EDKII_VARIABLE_STORAGE_GET_AUTHENTICATED_VARIABLE    GetAuthenticatedVariable;          ///< Retrieves an authenticated variable's value given its name and GUID
  EDKII_VARIABLE_STORAGE_GET_NEXT_VARIABLE_NAME        GetNextVariableName;               ///< Return the next variable name and GUID
  EDKII_VARIABLE_STORAGE_GET_STORAGE_USAGE             GetStorageUsage;                   ///< Returns information on storage usage in the variable store
  EDKII_VARIABLE_STORAGE_GET_AUTHENTICATED_SUPPORT     GetAuthenticatedSupport;           ///< Returns whether this non-volatile storage area supports authenticated variables
  EDKII_VARIABLE_STORAGE_SET_VARIABLE                  SetVariable;                       ///< Sets a variable's value using its name and GUID.
  EDKII_VARIABLE_STORAGE_WRITE_SERVICE_IS_READY        WriteServiceIsReady;               ///< Indicates if SetVariable() is ready or not
  EDKII_VARIABLE_STORAGE_GARBAGE_COLLECT               GarbageCollect;                    ///< Performs variable store garbage collection/reclaim operation.
};
```

### Variable Storage Selector Protocol

This protocol is used by the EDK II UEFI variable driver to acquire the variable storage instance ID (GUID) for a
particular variable name and vendor GUID. This ID is used to locate the appropriate variable storage protocol.

A platform must only install a single Variable Storage Selector protocol. The protocol should only be installed after
all Variable Storage protocols that will used on the platform have been installed. The installation of the Variable
Storage Selector protocol is a signal that the platform has accounted for all of its variable storage methods and is
ready to initialize UEFI variable services which will utilize those methods.

---

- Produced by: Platform code
- Consumed by: The EDK II UEFI variable driver

---

#### `EDKII_VARIABLE_STORAGE_SELECTOR_GET_ID`: `GetId()`

Gets the ID (GUID) for the variable storage instance that is used to store a given variable.

This function will be invoked any time the UEFI variable driver needs to perform a variable operation
(e.g. GetVariable(), GetNextVariableName(), SetVariable(), etc.) and must determine what Variable Storage Protocol
should be used for the transaction.

This function may be implemented with arbitrary logic to select a given Variable Storage Protocol. For example, if
only firmware volume block based (FVB) storage is used, the platform should install a single Variable Storage Protocol
and simply always return that protocol instance GUID unconditionally in this function implementation.

As another example, if a platform wishes to define two non-volatile regions of SPI flash to store variables, it could
produce two FVB-backed Variable Storage protocol instances. This function implementation could be implemented to store
a targeted set of variables in one store and all other variables in the other store. It could do this by defining the
targeted subset via a known list of variable name and variable GUID or simply give all variables for a secondary store
a special GUID and conditionalize on that.

In any case, the ID returned must be consistent for a given set of inputs (i.e. variable name and variable GUID). This
ensures get and set operations throughout boot and the platform's lifetime refer to the same store of data.

##### `EDKII_VARIABLE_STORAGE_SELECTOR_GET_ID`: Parameters

- `VariableName`: A pointer to a null-terminated string that is the variable's name.
- `VariableGuid`: A pointer to an EFI_GUID that is the variable's vendor GUID.
- `VariableStorageId`: The ID for the variable storage instance that stores a given variable.

##### `EDKII_VARIABLE_STORAGE_SELECTOR_GET_ID`: Return Value

- `EFI_SUCCESS`: Variable storage instance ID that was retrieved.
- `EFI_NOT_FOUND`: A variable storage instance is not available to store this variable.

#### `EDKII_VARIABLE_STORAGE_SELECTOR_PROTOCOL`

##### `EDKII_VARIABLE_STORAGE_SELECTOR_PROTOCOL`: Members

- `GetId()`: A function pointer to retrieve the ID of the selected variable storage.

##### `EDKII_VARIABLE_STORAGE_SELECTOR_PROTOCOL`: Code Example

```c
///
/// Variable Storage Selector Protocol
///
struct _EDKII_VARIABLE_STORAGE_SELECTOR_PROTOCOL {
  EDKII_VARIABLE_STORAGE_SELECTOR_GET_ID    GetId;
};
```

### Variable Storage Support Protocol

#### `EDKII_VARIABLE_STORAGE_SUPPORT_NOTIFY_WRITE_SERVICE_READY`: `NotifyWriteServiceReady()`

Notifies the UEFI variable driver that the variable storage drivers `WriteServiceIsReady()` function is now returning
`TRUE` instead of `FALSE`.

Variable storage drivers should call this function as soon as possible.

The UEFI variable driver will delay producing the Variable Write Architectural Protocol
(`gEfiVariableWriteArchProtocolGuid`) until all present variable storage protocols return `TRUE` from
`EDKII_VARIABLE_STORAGE_PROTOCOL.WriteServiceIsReady()`. The UEFI variable driver will query the
`WriteServiceIsReady()` status from each present variable storage protocol instance on each invocation of this
function.

---

- Produced by: The EDK II UEFI variable driver
- Consumed by: Variable storage drivers

---

##### `EDKII_VARIABLE_STORAGE_SUPPORT_NOTIFY_WRITE_SERVICE_READY`: Parameters

- None

##### `EDKII_VARIABLE_STORAGE_SUPPORT_NOTIFY_WRITE_SERVICE_READY`: Return Value

- None

#### `EDKII_VARIABLE_STORAGE_SUPPORT_PROTOCOL`

##### `EDKII_VARIABLE_STORAGE_SUPPORT_PROTOCOL`: Members

- `NotifyWriteServiceReady()`: Notify variable writes are available in a variable driver.

##### `EDKII_VARIABLE_STORAGE_SUPPORT_PROTOCOL`: Code Example

```c
///
/// Variable Storage Support Protocol
/// Interface functions for variable storage driver to access core variable driver functions in DXE/SMM phase.
///
struct _EDKII_VARIABLE_STORAGE_SUPPORT_PROTOCOL {
  EDKII_VARIABLE_STORAGE_SUPPORT_NOTIFY_WRITE_SERVICE_READY    NotifyWriteServiceReady;   ///< Notify variable writes are available in a variable driver
};
```

## PEI Interfaces

### Variable Storage PPI

Abstracts non-volatile media storage access details from the EDK II PEI UEFI variable module. A platform may produce
an arbitrary number of variable storage PPTs that are used by the PEI variable module for variable management during
the PEI boot phase.

---

- Produced by: Variable storage PEIMs
- Consumed by: The EDK II PEI UEFI variable module

---

#### `PEI_EDKII_VARIABLE_STORAGE_GET_ID`: `GetId()`

Retrieves a PPI instance-specific GUID.

Returns a unique GUID per VARIABLE_STORAGE_PPI instance.

##### `PEI_EDKII_VARIABLE_STORAGE_GET_ID`: Parameters

- `This`: A pointer to this instance of the `EDKII_VARIABLE_STORAGE_PPI`.
- `InstanceGuid`: A pointer to an `EFI_GUID` that is this PPI instance's GUID.

##### `PEI_EDKII_VARIABLE_STORAGE_GET_ID`: Return Value

- `EFI_SUCCESS`: The data was returned successfully.
- `EFI_INVALID_PARAMETER`: A required parameter is NULL.

#### `PEI_EDKII_VARIABLE_STORAGE_GET_VARIABLE`: `GetVariable()`

Retrieves a variable's value using its name and GUID.

Reads the specified variable from the UEFI variable store. If the Data buffer is too small to hold the contents of the
variable, the error `EFI_BUFFER_TOO_SMALL` is returned and `DataSize` is set to the required buffer size to obtain the
data.

##### `PEI_EDKII_VARIABLE_STORAGE_GET_VARIABLE`: Parameters

- `This`: A pointer to this instance of the `EDKII_VARIABLE_STORAGE_PPI`.
- `VariableName`: A pointer to a null-terminated string that is the variable's name.
- `VariableGuid`: A pointer to an `EFI_GUID` that is the variable's GUID. The combination of `VariableGuid` and
  `VariableName` must be unique.
- `Attributes` (optional): If non-NULL, on return, points to the variable's attributes.
- `DataSize`: On entry, points to the size in bytes of the `Data` buffer. On return, points to the size of the data
  returned in `Data`.
- `Data`: Points to the buffer which will hold the returned variable value.

##### `PEI_EDKII_VARIABLE_STORAGE_GET_VARIABLE`: Return Value

- `EFI_SUCCESS`: The variable was read successfully.
- `EFI_NOT_FOUND`: The variable could not be found.
- `EFI_BUFFER_TOO_SMALL`: The `DataSize` is too small for the resulting data. `DataSize` is updated with the size
  required for the specified variable.
- `EFI_INVALID_PARAMETER`: `VariableName`, `VariableGuid`, `DataSize`, or `Data` is NULL.
- `EFI_DEVICE_ERROR`: The variable could not be retrieved because of a device error.

#### `PEI_EDKII_VARIABLE_STORAGE_GET_NEXT_VARIABLE_NAME`: `GetNextVariableName()`

Return the next variable name and GUID.

This function is called multiple times to retrieve the `VariableName` and `VariableGuid` of all variables currently
available in the system. On each call, the previous results are passed into the interface, and, on return, the
interface returns the data for the next interface. When the entire variable list has been returned, `EFI_NOT_FOUND`
is returned.

##### `PEI_EDKII_VARIABLE_STORAGE_GET_NEXT_VARIABLE_NAME`: Parameters

- `This`: A pointer to this instance of the `EDKII_VARIABLE_STORAGE_PPI`.
- `VariableNameSize`: On entry, points to the size of the buffer pointed to by `VariableName`. On return, the size of
  the variable name buffer.
- `VariableName`: On entry, a pointer to a null-terminated string that is the variable's name. On return, points to
  the next variable's null-terminated name string.
- `VariableGuid`: On entry, a pointer to an `EFI_GUID` that is the variable's GUID. On return, a pointer to the next
  variable's GUID.
- `VariableAttributes`: A pointer to the variable attributes.

##### `PEI_EDKII_VARIABLE_STORAGE_GET_NEXT_VARIABLE_NAME`: Return Value

- `EFI_SUCCESS`: The variable was read successfully.
- `EFI_NOT_FOUND`: The variable could not be found.
- `EFI_BUFFER_TOO_SMALL`: The `VariableNameSize` is too small for the resulting data. `VariableNameSize` is updated
  with the size required for the specified variable.
- `EFI_INVALID_PARAMETER`: `VariableName`, `VariableGuid`, or `VariableNameSize` is NULL.
- `EFI_DEVICE_ERROR`: The variable could not be retrieved because of a device error.

#### `EDKII_VARIABLE_STORAGE_PPI`

##### Members

- `GetId()`: Retrieves a PPI instance-specific GUID.
- `GetVariable()`: Retrieves a variable's value given its name and GUID.
- `GetNextVariableName()`: Return the next variable name and GUID.

##### Code Example

```c
///
/// Variable Storage PPI
/// Interface functions for variable non-volatile storage access in the PEI boot phase.
///
struct _EDKII_VARIABLE_STORAGE_PPI {
  PEI_EDKII_VARIABLE_STORAGE_GET_ID                    GetId;                ///< Retrieves a PPI instance-specific GUID
  PEI_EDKII_VARIABLE_STORAGE_GET_VARIABLE              GetVariable;          ///< Retrieves a variable's value given its name and GUID
  PEI_EDKII_VARIABLE_STORAGE_GET_NEXT_VARIABLE_NAME    GetNextVariableName;  ///< Return the next variable name and GUID
};
```

### Variable Storage Selector PPI

This PPI is used by the EDK II PEI UEFI variable module to acquire the variable storage instance ID (GUID) for a
particular variable name and vendor GUID. This ID is used to locate the appropriate variable storage PPI.

A platform must only install a single Variable Storage Selector PPI. The PPI should only be installed after all
Variable Storage PPIs that will used on the platform have been installed. The installation of the Variable Storage
Selector PPI is a signal that the platform has accounted for all of its variable storage methods and is ready to
initialize PEI variable services which will utilize those methods.

---

- Produced by: Platform code
- Consumed by: The EDK II PEI UEFI variable module

---

#### `PEI_EDKII_VARIABLE_STORAGE_SELECTOR_GET_ID`: `GetId()`

Gets the ID (GUID) for the variable storage instance that is used to store a given variable.

This function will be invoked any time the UEFI variable module needs to perform a variable operation
(e.g. GetVariable(), GetNextVariableName(), SetVariable(), etc.) and must determine what Variable Storage PPI should be
used for the transaction.

This function may be implemented with arbitrary logic to select a given Variable Storage PPI. For example, if only
firmware volume block-based (FVB) storage is used, the platform should install a single Variable Storage PPI and simply
always return that PPI instance GUID unconditionally in this function implementation.

As another example, if a platform wishes to define two non-volatile regions of SPI flash to store variables, it could
produce two FVB-backed Variable Storage PPI instances. This function implementation could be implemented to store a
targeted set of variables in one store and all other variables in the other store. It could do this by defining the
targeted subset via a known list of variable name and variable GUID or simply give all variables for a secondary store
a special GUID and conditionally on that.

In any case, the ID returned must be consistent for a given set of inputs (i.e. variable name and variable GUID). This
get and set operations throughout boot and the platform's lifetime refer to the same store of data.

##### `PEI_EDKII_VARIABLE_STORAGE_SELECTOR_GET_ID`: Parameters

- `VariableName`: A pointer to a null-terminated string that is the variable's name.
- `VariableGuid`: A pointer to an `EFI_GUID` that is the variable's GUID.
- `VariableStorageId`: The ID for the variable storage instance that stores a given variable.

##### `PEI_EDKII_VARIABLE_STORAGE_SELECTOR_GET_ID`: Return Value

- `EFI_SUCCESS`: Variable storage instance ID that was retrieved.
- `EFI_NOT_FOUND`: A variable storage instance is not available to store this variable.

#### `EDKII_VARIABLE_STORAGE_SELECTOR_PPI`

##### `EDKII_VARIABLE_STORAGE_SELECTOR_PPI`: Members

- `GetId()`: Retrieves an instance-specific variable storage ID.

##### `EDKII_VARIABLE_STORAGE_SELECTOR_PPI`: Code Example

```c
///
/// Variable Storage PPI
///
struct _EDKII_VARIABLE_STORAGE_SELECTOR_PPI {
  PEI_EDKII_VARIABLE_STORAGE_SELECTOR_GET_ID    GetId;      ///< Retrieves an instance-specific variable storage ID
};
```
