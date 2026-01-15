"""
Copyright (c) 2026 Microsoft Corporation. All rights reserved.

StatusCodeprocessor.py

A tool for parsing and decoding UEFI/EDK2 status codes from debug logs.
Supports both standard PI specification codes and platform-specific custom codes,
with automatic header discovery, macro resolution, and GUID-to-module name lookup.

"""

import argparse
import re
import os
import logging

# Configure logging
logger = logging.getLogger(__name__)

# Status Code Type Definitions
EFI_STATUS_CODE_TYPE_MASK = 0x000000FF
EFI_STATUS_CODE_SEVERITY_MASK = 0xFF000000
EFI_STATUS_CODE_RESERVED_MASK = 0x00FFFF00

# Code Types
EFI_PROGRESS_CODE = 0x00000001
EFI_ERROR_CODE = 0x00000002
EFI_DEBUG_CODE = 0x00000003

# Severities
EFI_ERROR_MINOR = 0x40000000
EFI_ERROR_MAJOR = 0x80000000
EFI_ERROR_UNRECOVERED = 0x90000000
EFI_ERROR_UNCONTAINED = 0xA0000000

# Status Code Value Masks
EFI_STATUS_CODE_CLASS_MASK = 0xFF000000
EFI_STATUS_CODE_SUBCLASS_MASK = 0x00FF0000
EFI_STATUS_CODE_OPERATION_MASK = 0x0000FFFF

# Subclass Specific Mask
EFI_SUBCLASS_SPECIFIC = 0x1000
EFI_OEM_SPECIFIC = 0x8000

# Classes
CLASSES = {
    0x00000000: "COMPUTING_UNIT",
    0x01000000: "PERIPHERAL",
    0x02000000: "IO_BUS",
    0x03000000: "SOFTWARE",
}

# Computing Unit Subclasses
COMPUTING_UNIT_SUBCLASSES = {
    0x00000000: "UNSPECIFIED",
    0x00010000: "HOST_PROCESSOR",
    0x00020000: "FIRMWARE_PROCESSOR",
    0x00030000: "IO_PROCESSOR",
    0x00040000: "CACHE",
    0x00050000: "MEMORY",
    0x00060000: "CHIPSET",
    0x00070000: "MANAGEABILITY",
}

# Peripheral Subclasses
PERIPHERAL_SUBCLASSES = {
    0x00000000: "UNSPECIFIED",
    0x00010000: "KEYBOARD",
    0x00020000: "MOUSE",
    0x00030000: "LOCAL_CONSOLE",
    0x00040000: "REMOTE_CONSOLE",
    0x00050000: "SERIAL_PORT",
    0x00060000: "PARALLEL_PORT",
    0x00070000: "FIXED_MEDIA",
    0x00080000: "REMOVABLE_MEDIA",
    0x00090000: "AUDIO_INPUT",
    0x000A0000: "AUDIO_OUTPUT",
    0x000B0000: "LCD_DEVICE",
    0x000C0000: "NETWORK",
    0x000D0000: "DOCKING",
    0x000E0000: "TPM",
}

# IO Bus Subclasses
IO_BUS_SUBCLASSES = {
    0x00000000: "UNSPECIFIED",
    0x00010000: "PCI",
    0x00020000: "USB",
    0x00030000: "IBA",
    0x00040000: "AGP",
    0x00050000: "PC_CARD",
    0x00060000: "LPC",
    0x00070000: "SCSI",
    0x00080000: "ATA_ATAPI",
    0x00090000: "FC",
    0x000A0000: "IP_NETWORK",
    0x000B0000: "SMBUS",
    0x000C0000: "I2C",
}

# Software Subclasses
SOFTWARE_SUBCLASSES = {
    0x00000000: "UNSPECIFIED",
    0x00010000: "SEC",
    0x00020000: "PEI_CORE",
    0x00030000: "PEI_MODULE",
    0x00040000: "DXE_CORE",
    0x00050000: "DXE_BS_DRIVER",
    0x00060000: "DXE_RT_DRIVER",
    0x00070000: "SMM_DRIVER",
    0x00080000: "EFI_APPLICATION",
    0x00090000: "EFI_OS_LOADER",
    0x000A0000: "RT",
    0x000B0000: "AL",
    0x000C0000: "EBC_EXCEPTION",
    0x000D0000: "IA32_EXCEPTION",
    0x000E0000: "IPF_EXCEPTION",
    0x000F0000: "PEI_SERVICE",
    0x00100000: "EFI_BOOT_SERVICE",
    0x00110000: "EFI_RUNTIME_SERVICE",
    0x00120000: "EFI_DXE_SERVICE",
    0x00130000: "X64_EXCEPTION",
    0x00140000: "ARM_EXCEPTION",
}

# Progress Code Operations - Common
COMMON_PROGRESS_OPERATIONS = {
    0x00000000: "INIT_BEGIN",
    0x00000001: "INIT_END",
}

# Software Progress Code Operations
SOFTWARE_COMMON_OPERATIONS = {
    0x00000000: "PC_INIT",
    0x00000001: "PC_LOAD",
    0x00000002: "PC_INIT_BEGIN",
    0x00000003: "PC_INIT_END",
    0x00000004: "PC_AUTHENTICATE_BEGIN",
    0x00000005: "PC_AUTHENTICATE_END",
    0x00000006: "PC_INPUT_WAIT",
    0x00000007: "PC_USER_SETUP",
}

# DXE Core Specific Operations
DXE_CORE_OPERATIONS = {
    0x1000: "PC_ENTRY_POINT",
    0x1001: "PC_HANDOFF_TO_NEXT",
    0x1002: "PC_RETURN_TO_LAST",
    0x1003: "PC_START_DRIVER",
    0x1004: "PC_ARCH_READY",
}

# Runtime Service Operations
RUNTIME_SERVICE_OPERATIONS = {
    0x1000: "PC_GET_TIME",
    0x1001: "PC_SET_TIME",
    0x1002: "PC_GET_WAKEUP_TIME",
    0x1003: "PC_SET_WAKEUP_TIME",
    0x1004: "PC_SET_VIRTUAL_ADDRESS_MAP",
    0x1005: "PC_CONVERT_POINTER",
    0x1006: "PC_GET_VARIABLE",
    0x1007: "PC_GET_NEXT_VARIABLE_NAME",
    0x1008: "PC_SET_VARIABLE",
    0x1009: "PC_GET_NEXT_HIGH_MONOTONIC_COUNT",
    0x100A: "PC_RESET_SYSTEM",
    0x100B: "PC_UPDATE_CAPSULE",
    0x100C: "PC_QUERY_CAPSULE_CAPABILITIES",
    0x100D: "PC_QUERY_VARIABLE_INFO",
}

# Error Code Operations - Common
COMMON_ERROR_OPERATIONS = {
    0x00000000: "EC_NON_SPECIFIC",
    0x00000001: "EC_DISABLED",
    0x00000002: "EC_NOT_SUPPORTED",
    0x00000003: "EC_NOT_DETECTED",
    0x00000004: "EC_NOT_CONFIGURED",
}

# Software Error Operations
SOFTWARE_ERROR_OPERATIONS = {
    0x00000000: "EC_NON_SPECIFIC",
    0x00000001: "EC_LOAD_ERROR",
    0x00000002: "EC_INVALID_PARAMETER",
    0x00000003: "EC_UNSUPPORTED",
    0x00000004: "EC_INVALID_BUFFER",
    0x00000005: "EC_OUT_OF_RESOURCES",
    0x00000006: "EC_ABORTED",
    0x00000007: "EC_ILLEGAL_SOFTWARE_STATE",
    0x00000008: "EC_ILLEGAL_HARDWARE_STATE",
    0x00000009: "EC_START_ERROR",
    0x0000000A: "EC_BAD_DATE_TIME",
    0x0000000B: "EC_CFG_INVALID",
    0x0000000C: "EC_CFG_CLR_REQUEST",
    0x0000000D: "EC_CFG_DEFAULT",
    0x0000000E: "EC_PWD_INVALID",
    0x0000000F: "EC_PWD_CLR_REQUEST",
    0x00000010: "EC_PWD_CLEARED",
    0x00000011: "EC_EVENT_LOG_FULL",
    0x00000012: "EC_WRITE_PROTECTED",
    0x00000013: "EC_FV_CORRUPTED",
    0x00000014: "EC_INCONSISTENT_MEMORY_MAP",
}


def discover_status_code_headers(search_path):
    """Automatically discover platform-specific status code header files."""
    if not search_path or not os.path.exists(search_path):
        return []
    
    discovered_headers = []
    logger.info("\n=== Discovering Platform Status Code Headers ===")
    logger.info(f"Searching in: {search_path}")
    
    # Patterns to match status code headers
    patterns = [
        r'.*StatusCode.*\.h$',
        r'.*StatusCodes.*\.h$',
    ]
    
    for root, _, files in os.walk(search_path):
        for file_name in files:
            for pattern in patterns:
                if re.match(pattern, file_name, re.IGNORECASE):
                    full_path = os.path.join(root, file_name)
                    if 'Pi/PiStatusCode.h' not in full_path:
                        discovered_headers.append(full_path)
                        relative_path = os.path.relpath(full_path, search_path)
                        logger.debug(f"  Found: {relative_path}")
                    break
    
    logger.info(f"Total discovered: {len(discovered_headers)} header file(s)\n")
    return discovered_headers


def parse_single_header_file(header_file_path, existing_definitions=None):
    """Parse a single status code header file."""
    status_codes = {}
    definitions = existing_definitions.copy() if existing_definitions else {}
    file_name = os.path.basename(header_file_path)
    
    logger.debug(f"\n  [DEBUG] Starting parse of {file_name}")
    logger.debug(f"  [DEBUG] Initial definitions count: {len(definitions)}")
    if definitions:
        logger.debug(f"  [DEBUG] Some initial definitions: {list(definitions.keys())[:5]}")
    
    if not os.path.exists(header_file_path):
        return status_codes, definitions, file_name
    
    try:
        with open(header_file_path, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
            
            complex_pattern = r'#define\s+([A-Z_][A-Z0-9_]*)\s+\(([^)]+)\)'
            simple_pattern = r'#define\s+([A-Z_][A-Z0-9_]*)\s+(0x[0-9A-Fa-f]+)'
            
            # First pass: collect all simple definitions
            simple_count = 0
            for match in re.finditer(simple_pattern, content):
                name = match.group(1)
                value = int(match.group(2), 16)
                definitions[name] = value
                simple_count += 1
            
            logger.debug(f"  [DEBUG] Found {simple_count} simple definitions")
            
            # Second pass: parse complex expressions
            complex_count = 0
            failed_count = 0
            for match in re.finditer(complex_pattern, content):
                name = match.group(1)
                expression = match.group(2)
                
                logger.debug(f"  [DEBUG] Attempting to evaluate: {name} = ({expression})")
                
                try:
                    eval_expr = expression
                    
                    for def_name, def_value in definitions.items():
                        if def_name in eval_expr:
                            eval_expr = eval_expr.replace(def_name, str(def_value))
                    
                    remaining_macros = re.findall(r'[A-Z_][A-Z0-9_]+', eval_expr)
                    if remaining_macros:
                        logger.debug(f"  [DEBUG] WARNING: Unresolved macros: {remaining_macros}")
                    
                    eval_expr = eval_expr.replace(' ', '')
                    value = eval(eval_expr)
                    
                    definitions[name] = value
                    status_codes[value] = (name, file_name)
                    complex_count += 1
                    
                    logger.debug(f"  [DEBUG] SUCCESS: {name} = 0x{value:08X}")
                    
                except Exception as e:
                    failed_count += 1
                    logger.debug(f"  [DEBUG] FAILED to evaluate: {name} = ({expression})")
                    logger.debug(f"  [DEBUG] Error: {e}")
                    continue
            
            logger.debug(f"  [DEBUG] Complex expressions: {complex_count} succeeded, {failed_count} failed")
            logger.debug(f"  [DEBUG] Final definitions count: {len(definitions)}")
            logger.debug(f"  [DEBUG] Status codes found: {len(status_codes)}")
                    
    except Exception as e:
        logger.warning(f"  Warning: Error parsing {header_file_path}: {e}")
    
    return status_codes, definitions, file_name


def parse_platform_status_codes(header_paths):
    """Parse platform-specific status code definitions from header files.
    
    Returns a tuple of (status_codes_dict, definitions_dict):
    - status_codes_dict: Maps complete status code values to (name, source_file)
    - definitions_dict: Maps all parsed macro names to their values (for component lookup)
    """
    all_status_codes = {}
    global_definitions = {}
    
    if not header_paths:
        return all_status_codes, {}
    
    logger.info("=== Parsing Platform Status Codes ===")
    
    for header_path in header_paths:
        file_name = os.path.basename(header_path)
        logger.info(f"\nProcessing: {file_name}")
        
        status_codes, updated_definitions, _ = parse_single_header_file(header_path, global_definitions)
        global_definitions.update(updated_definitions)
        
        logger.debug(f"  [DEBUG] Global definitions now has {len(global_definitions)} entries")
        
        if status_codes:
            logger.debug(f"  Parsed {len(status_codes)} status code(s):")
            for value, (name, source) in sorted(status_codes.items()):
                logger.debug(f"    {name} = 0x{value:08X}")
            all_status_codes.update(status_codes)
        else:
            logger.debug("  No status codes found")
    
    logger.info("\n=== Summary ===")
    logger.info(f"Total platform status codes loaded: {len(all_status_codes)}\n")
    
    return all_status_codes, global_definitions


def parse_status_code_type(code_type):
    """Parse the status code type field (CodeType in error messages)"""
    result = {}
    
    severity = code_type & EFI_STATUS_CODE_SEVERITY_MASK
    severity_name = "NONE"
    if severity == EFI_ERROR_MINOR:
        severity_name = "EFI_ERROR_MINOR"
    elif severity == EFI_ERROR_MAJOR:
        severity_name = "EFI_ERROR_MAJOR"
    elif severity == EFI_ERROR_UNRECOVERED:
        severity_name = "EFI_ERROR_UNRECOVERED"
    elif severity == EFI_ERROR_UNCONTAINED:
        severity_name = "EFI_ERROR_UNCONTAINED"
    
    result['severity'] = (severity, severity_name)
    result['reserved'] = code_type & EFI_STATUS_CODE_RESERVED_MASK
    
    code_type_val = code_type & EFI_STATUS_CODE_TYPE_MASK
    type_name = "UNKNOWN"
    if code_type_val == EFI_PROGRESS_CODE:
        type_name = "EFI_PROGRESS_CODE"
    elif code_type_val == EFI_ERROR_CODE:
        type_name = "EFI_ERROR_CODE"
    elif code_type_val == EFI_DEBUG_CODE:
        type_name = "EFI_DEBUG_CODE"
    
    result['type'] = (code_type_val, type_name)
    
    return result


def parse_status_code_value(value, platform_codes=None, platform_definitions=None):
    """Parse the status code value field
    
    Args:
        value: The status code value to parse
        platform_codes: Dict mapping complete status codes to (name, source_file)
        platform_definitions: Dict mapping all macro names to their values (for component lookup)
    """
    result = {}
    
    if platform_codes and value in platform_codes:
        macro_name, source_file = platform_codes[value]
        result['platform_code'] = macro_name
        result['platform_source'] = source_file
        result['note'] = f"Platform-specific status code: {macro_name} (from {source_file})"
    
    # Create reverse lookup for platform definitions (value -> name)
    # Filter to appropriate ranges for cleaner lookups
    reverse_definitions = {}
    if platform_definitions:
        for name, val in platform_definitions.items():
            if val not in reverse_definitions:
                reverse_definitions[val] = []
            reverse_definitions[val].append(name)
        
    class_val = value & EFI_STATUS_CODE_CLASS_MASK
    class_name = CLASSES.get(class_val, "UNKNOWN_CLASS")
    
    # For custom/OEM classes, try to find the name from platform definitions
    if class_name == "UNKNOWN_CLASS" and reverse_definitions:
        if class_val in reverse_definitions:
            # Prefer names with "CLASS" or "PARENT" in them for clarity
            candidates = reverse_definitions[class_val]
            platform_class_name = next((n for n in candidates if 'CLASS' in n.upper() or 'PARENT' in n.upper()), candidates[0])
            class_name = f"{platform_class_name} (0x{class_val:08X})"
        else:
            class_name = f"UNKNOWN_CLASS (0x{class_val:08X})"
    
    result['class'] = (class_val, class_name)
    
    subclass_val = value & EFI_STATUS_CODE_SUBCLASS_MASK
    is_standard_class = class_val in CLASSES
    
    if not is_standard_class:
        # For custom/OEM classes, try to find more specific names from platform codes
        subclass_name = f"OEM/CUSTOM (0x{subclass_val:08X})"
        operation_val = value & EFI_STATUS_CODE_OPERATION_MASK
        operation_name = f"OEM/CUSTOM (0x{operation_val:04X})"
        
        # Check if we have platform-specific definitions for subclass or operation
        if platform_codes or reverse_definitions:
            # Try to find a definition for class + subclass combination (highest priority)
            # But only look in reverse_definitions for component definitions, excluding full status codes
            class_subclass_val = class_val | subclass_val
            if class_subclass_val in reverse_definitions and class_subclass_val not in platform_codes:
                # Use reverse lookup for component definitions (not full status codes)
                # Prefer names that contain "SUBCLASS" for clarity
                candidates = reverse_definitions[class_subclass_val]
                macro_name = next((n for n in candidates if 'SUBCLASS' in n.upper()), candidates[0])
                subclass_name = f"{macro_name} (0x{subclass_val:08X})"
            # Only check standalone subclass value if class+subclass not found
            # and subclass is non-zero and in valid range (0x00XX0000)
            elif subclass_val != 0 and (subclass_val & 0xFF00FFFF) == 0 and subclass_val in reverse_definitions:
                # Filter to prefer SUBCLASS-related names
                candidates = reverse_definitions[subclass_val]
                macro_name = next((n for n in candidates if 'SUBCLASS' in n.upper()), candidates[0])
                subclass_name = f"{macro_name} (0x{subclass_val:08X})"
            
            # Try to find a definition for the operation value
            # Priority: class+operation > standalone operation
            # Only look in reverse_definitions for component values, excluding full status codes
            class_operation_val = class_val | operation_val
            if class_operation_val in reverse_definitions and class_operation_val not in platform_codes:
                op_macro_name = reverse_definitions[class_operation_val][0]
                operation_name = f"{op_macro_name} (0x{operation_val:04X})"
            elif operation_val != 0 and operation_val < 0x10000 and operation_val in reverse_definitions:
                # Use reverse lookup for component definitions (operations are 16-bit)
                op_macro_name = reverse_definitions[operation_val][0]
                operation_name = f"{op_macro_name} (0x{operation_val:04X})"
        
        result['subclass'] = (subclass_val, subclass_name)
        result['operation'] = (operation_val, operation_name)
        
        if 'note' not in result:
            result['note'] = "This appears to be a vendor-specific or OEM-defined status code that does not follow standard UEFI PI specification format"
        
        result['raw_interpretation'] = {
            'byte3': (value >> 24) & 0xFF,
            'byte2': (value >> 16) & 0xFF,
            'byte1': (value >> 8) & 0xFF,
            'byte0': value & 0xFF,
        }
        return result
    
    subclass_name = "UNKNOWN_SUBCLASS"
    if class_val == 0x00000000:
        subclass_name = COMPUTING_UNIT_SUBCLASSES.get(subclass_val, f"UNKNOWN (0x{subclass_val:08X})")
    elif class_val == 0x01000000:
        subclass_name = PERIPHERAL_SUBCLASSES.get(subclass_val, f"UNKNOWN (0x{subclass_val:08X})")
    elif class_val == 0x02000000:
        subclass_name = IO_BUS_SUBCLASSES.get(subclass_val, f"UNKNOWN (0x{subclass_val:08X})")
    elif class_val == 0x03000000:
        subclass_name = SOFTWARE_SUBCLASSES.get(subclass_val, f"UNKNOWN (0x{subclass_val:08X})")
        
    result['subclass'] = (subclass_val, subclass_name)
    
    operation_val = value & EFI_STATUS_CODE_OPERATION_MASK
    operation_name = "UNKNOWN_OPERATION"
    
    if operation_val >= EFI_SUBCLASS_SPECIFIC:
        if class_val == 0x03000000:
            if subclass_val == 0x00040000:
                operation_name = DXE_CORE_OPERATIONS.get(operation_val, f"SUBCLASS_SPECIFIC (0x{operation_val:04X})")
            elif subclass_val == 0x00110000:
                operation_name = RUNTIME_SERVICE_OPERATIONS.get(operation_val, f"SUBCLASS_SPECIFIC (0x{operation_val:04X})")
            else:
                operation_name = f"SUBCLASS_SPECIFIC (0x{operation_val:04X})"
        else:
            operation_name = f"SUBCLASS_SPECIFIC (0x{operation_val:04X})"
    elif operation_val >= EFI_OEM_SPECIFIC:
        operation_name = f"OEM_SPECIFIC (0x{operation_val:04X})"
    else:
        if class_val == 0x03000000:
            operation_name = SOFTWARE_COMMON_OPERATIONS.get(operation_val,
                            SOFTWARE_ERROR_OPERATIONS.get(operation_val,
                            f"COMMON (0x{operation_val:04X})"))
        else:
            operation_name = COMMON_PROGRESS_OPERATIONS.get(operation_val,
                            COMMON_ERROR_OPERATIONS.get(operation_val,
                            f"COMMON (0x{operation_val:04X})"))
    
    result['operation'] = (operation_val, operation_name)
    
    return result


def find_guid_in_files(guid, search_path):
    """Search for GUID in .inf, .dec, and .fdf files to find module name"""
    if not search_path or not os.path.exists(search_path):
        return None
    
    guid_upper = guid.upper()
    file_count = 0
    
    INF_FILE_REGEX = r"\s*BASE_NAME\s*=\s*([a-zA-Z]\w*)\s*"
    INF_GUID_REGEX = (
        r"\s*FILE_GUID\s*=\s*([0-9a-fA-F]{8,8}-[0-9a-fA-F]{4,4}-[0-9a-fA-F]{4,4}-"
        r"[0-9a-fA-F]{4,4}-[0-9a-fA-F]{12,12})\s*"
    )
    DEC_REGEX = (
        r"\s*([a-zA-Z]\w*)\s*=\s*"
        + r"\{\s*0x([0-9a-fA-F]{1,8})\s*,\s*"
        + r"0x([0-9a-fA-F]{1,4})\s*,\s*"
        + r"0x([0-9a-fA-F]{1,4})\s*,\s*"
        + r"\s*\{\s*0x([0-9a-fA-F]{1,2})\s*,\s*"
        + r"0x([0-9a-fA-F]{1,2})\s*,\s*"
        + r"0x([0-9a-fA-F]{1,2})\s*,\s*"
        + r"0x([0-9a-fA-F]{1,2})\s*,\s*"
        + r"0x([0-9a-fA-F]{1,2})\s*,\s*"
        + r"0x([0-9a-fA-F]{1,2})\s*,\s*"
        + r"0x([0-9a-fA-F]{1,2})\s*,\s*"
        + r"0x([0-9a-fA-F]{1,2})\s*\}\s*\}"
    )
    FDF_GUID_REGEX = (
        r"\s*FILE\s*DRIVER\s*=\s*([0-9a-fA-F]{8,8}-[0-9a-fA-F]{4,4}-[0-9a-fA-F]{4,4}-"
        r"[0-9a-fA-F]{4,4}-[0-9a-fA-F]{12,12})\s*\{+\s*"
    )
    FDF_FILE_REGEX = r"\s*SECTION\s*UI\s*=\s*\"([\w\s]+)\"\s*"
    
    logger.debug(f"\nSearching for GUID in: {search_path}")
    
    for root, _, files in os.walk(search_path):
        for name in files:
            file_path = os.path.join(root, name)
            file_count += 1
            
            try:
                if name.lower().endswith(".inf"):
                    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                        base_name = None
                        found_guid = None
                        
                        file_pattern = re.compile(INF_FILE_REGEX)
                        guid_pattern = re.compile(INF_GUID_REGEX)
                        
                        for line in f:
                            if file_match := file_pattern.match(line):
                                base_name = file_match.group(1)
                            elif guid_match := guid_pattern.match(line):
                                found_guid = guid_match.group(1).upper()
                            
                            if base_name and found_guid:
                                if found_guid == guid_upper:
                                    logger.debug(f"  Found in: {os.path.relpath(file_path, search_path)}")
                                    return base_name
                                base_name, found_guid = None, None
                
                elif name.lower().endswith(".dec"):
                    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                        pattern = re.compile(DEC_REGEX)
                        for line in f:
                            match = pattern.match(line)
                            if match:
                                guid_key = (
                                    f"{match.group(2).upper().zfill(8)}-"
                                    f"{match.group(3).upper().zfill(4)}-"
                                    f"{match.group(4).upper().zfill(4)}-"
                                    f"{match.group(5).upper().zfill(2)}{match.group(6).upper().zfill(2)}-"
                                    f"{''.join(match.groups()[6:]).upper().zfill(12)}"
                                )
                                if guid_key == guid_upper:
                                    logger.debug(f"  Found in: {os.path.relpath(file_path, search_path)}")
                                    return match.group(1)
                
                elif name.lower().endswith(".fdf"):
                    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                        file_pattern = re.compile(FDF_FILE_REGEX)
                        guid_pattern = re.compile(FDF_GUID_REGEX)
                        
                        ui_name = None
                        found_guid = None
                        
                        for line in f:
                            if file_match := file_pattern.match(line):
                                ui_name = file_match.group(1)
                            elif guid_match := guid_pattern.match(line):
                                found_guid = guid_match.group(1).upper()
                            
                            if ui_name and found_guid:
                                if found_guid == guid_upper:
                                    logger.debug(f"  Found in: {os.path.relpath(file_path, search_path)}")
                                    return ui_name
                                ui_name, found_guid = None, None
                            
            except Exception:
                continue
    
    logger.debug(f"  Searched {file_count} files - GUID not found")
    return None


def process_progress_code(code_str, search_path=None, explicit_headers=None, auto_discover=False):
    """Process a progress code string"""
    platform_codes = {}
    platform_definitions = {}
    
    if auto_discover and search_path:
        discovered_headers = discover_status_code_headers(search_path)
        if discovered_headers:
            platform_codes, platform_definitions = parse_platform_status_codes(discovered_headers)
    
    if explicit_headers:
        if not auto_discover:
            logger.info("\n=== Parsing Explicit Platform Headers ===")
        explicit_codes, explicit_definitions = parse_platform_status_codes(explicit_headers)
        platform_codes.update(explicit_codes)
        platform_definitions.update(explicit_definitions)
    
    match = re.search(r'V([0-9A-Fa-f]+)', code_str)
    if not match:
        print("Error: Could not parse progress code value")
        return
    
    value_str = match.group(1)
    try:
        value = int(value_str, 16)
    except ValueError:
        print(f"Error: Invalid hex value '{value_str}'")
        return
    
    parsed = parse_status_code_value(value, platform_codes, platform_definitions)
    
    print("\n=== Progress Code Analysis ===\n")
    print(f"Status Code Value: 0x{value:08X}")
    
    if 'platform_code' in parsed:
        print(f"Platform Code: {parsed['platform_code']}")
        print(f"Source File:   {parsed['platform_source']}")
    
    print(f"Class:     {parsed['class'][1]} (0x{parsed['class'][0]:08X})")
    print(f"Subclass:  {parsed['subclass'][1]} (0x{parsed['subclass'][0]:08X})")
    print(f"Operation: {parsed['operation'][1]} (0x{parsed['operation'][0]:04X})")
    
    if 'note' in parsed:
        print(f"\nNOTE: {parsed['note']}")
    
    if 'raw_interpretation' in parsed:
        print("Raw byte breakdown:")
        print(f"  Byte 3 (MSB): 0x{parsed['raw_interpretation']['byte3']:02X}")
        print(f"  Byte 2:       0x{parsed['raw_interpretation']['byte2']:02X}")
        print(f"  Byte 1:       0x{parsed['raw_interpretation']['byte1']:02X}")
        print(f"  Byte 0 (LSB): 0x{parsed['raw_interpretation']['byte0']:02X}")


def process_error_code(error_str, search_path=None, explicit_headers=None, auto_discover=False):
    """Process an error code string"""
    platform_codes = {}
    platform_definitions = {}
    
    if auto_discover and search_path:
        discovered_headers = discover_status_code_headers(search_path)
        if discovered_headers:
            platform_codes, platform_definitions = parse_platform_status_codes(discovered_headers)
    
    if explicit_headers:
        if not auto_discover:
            logger.info("\n=== Parsing Explicit Platform Headers ===")
        explicit_codes, explicit_definitions = parse_platform_status_codes(explicit_headers)
        platform_codes.update(explicit_codes)
        platform_definitions.update(explicit_definitions)
    
    code_type_match = re.search(r'C([0-9A-Fa-f]+):', error_str)
    if not code_type_match:
        print("Error: Could not parse CodeType")
        return
    
    code_type_str = code_type_match.group(1)
    try:
        code_type = int(code_type_str, 16)
    except ValueError:
        print(f"Error: Invalid hex value for CodeType '{code_type_str}'")
        return
    
    value_match = re.search(r'V([0-9A-Fa-f]+)', error_str)
    if not value_match:
        print("Error: Could not parse Value")
        return
    
    value_str = value_match.group(1)
    try:
        value = int(value_str, 16)
    except ValueError:
        print(f"Error: Invalid hex value for Value '{value_str}'")
        return
    
    instance_match = re.search(r'I([0-9A-Fa-f]+)', error_str)
    instance = int(instance_match.group(1), 16) if instance_match else 0
    
    guid_match = re.search(r'([0-9A-Fa-f]{8}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{12})', error_str)
    guid = guid_match.group(1) if guid_match else None
    
    if guid:
        ext_data_match = re.search(guid + r'\s+([0-9A-Fa-f]+)', error_str)
        ext_data = int(ext_data_match.group(1), 16) if ext_data_match else None
    else:
        ext_data = None
    
    type_parsed = parse_status_code_type(code_type)
    value_parsed = parse_status_code_value(value, platform_codes, platform_definitions)
    
    # === DEFAULT OUTPUT - ALWAYS SHOWN ===
    print("\n=== Error Code Analysis ===")
    print(f"\nCodeType: 0x{code_type:08X}")
    print(f"  Severity:  0x{type_parsed['severity'][0]:08X} ({type_parsed['severity'][1]})")
    print(f"  Reserved:  0x{type_parsed['reserved']:08X} {'(none)' if type_parsed['reserved'] == 0 else ''}")
    print(f"  Type:      0x{type_parsed['type'][0]:08X} ({type_parsed['type'][1]})")
    
    print(f"\nStatus Code Value: 0x{value:08X}")
    
    if 'platform_code' in value_parsed:
        print(f"  Platform Code: {value_parsed['platform_code']}")
        print(f"  Source File:   {value_parsed['platform_source']}")
    
    print(f"  Class:     {value_parsed['class'][1]} (0x{value_parsed['class'][0]:08X})")
    print(f"  Subclass:  {value_parsed['subclass'][1]} (0x{value_parsed['subclass'][0]:08X})")
    print(f"  Operation: {value_parsed['operation'][1]} (0x{value_parsed['operation'][0]:04X})")
    
    if 'note' in value_parsed:
        print(f"\n  NOTE: {value_parsed['note']}")
    
    if 'raw_interpretation' in value_parsed:
        print("  Raw byte breakdown:")
        print(f"    Byte 3 (MSB): 0x{value_parsed['raw_interpretation']['byte3']:02X}")
        print(f"    Byte 2:       0x{value_parsed['raw_interpretation']['byte2']:02X}")
        print(f"    Byte 1:       0x{value_parsed['raw_interpretation']['byte1']:02X}")
        print(f"    Byte 0 (LSB): 0x{value_parsed['raw_interpretation']['byte0']:02X}")
    
    print(f"\nInstance: {instance}")
    
    if guid:
        module_name = None
        if search_path:
            module_name = find_guid_in_files(guid, search_path)
        
        if module_name:
            print(f"Module:   {module_name} ({guid})")
        else:
            print(f"GUID:     {guid}")
            if search_path:
                logger.debug("  (Module name not found in search path)")
    
    if ext_data is not None:
        print(f"Extended Data: 0x{ext_data:08X}")


def main():
    parser = argparse.ArgumentParser(
        description='UEFI Status Code Processor - Parse progress codes and error codes',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Basic usage
  %(prog)s -p "PROGRESS CODE: V03041001 I0"
  
  # With GUID search (auto-discovery is enabled by default)
  %(prog)s -e "ERROR: C40000002:VA40A0003 I0 C4F5CB5C-C210-4C70-A682-7EBCC803CC06 6F5DFC10" -s /path/to/workspace
  
  # Add additional platform-specific header files (combines with auto-discovery)
  %(prog)s -e "ERROR: C40000002:VA40A0003 I0 C4F5CB5C-C210-4C70-A682-7EBCC803CC06 6F5DFC10" -s /path/to/workspace -c /path/to/CustomStatusCodes.h
  
  # Disable auto-discovery if needed
  %(prog)s -e "ERROR: C40000002:VA40A0003 I0 C4F5CB5C-C210-4C70-A682-7EBCC803CC06 6F5DFC10" -s /path/to/workspace --no-auto-discover
  
  # Enable debug mode to see detailed parsing information
  %(prog)s -e "ERROR: C40000002:VA40A0003 I0 C4F5CB5C-C210-4C70-A682-7EBCC803CC06 6F5DFC10" -s /path/to/workspace --debug
        """
    )
    
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('-p', '--progress', metavar='CODE',
                      help='Process a progress code string')
    group.add_argument('-e', '--error', metavar='CODE',
                      help='Process an error code string')
    
    parser.add_argument('-s', '--search', metavar='PATH',
                      help='Path to search for module GUID definitions and auto-discover status code headers')
    
    parser.add_argument('-c', '--platform-codes', metavar='HEADER', nargs='*',
                      help='Path(s) to additional platform-specific status code header file(s) (works together with auto-discovery)')
    
    parser.add_argument('--no-auto-discover', action='store_true',
                      help='Disable automatic discovery of *StatusCode*.h headers (auto-discovery is enabled by default)')
    
    parser.add_argument('--debug', action='store_true',
                      help='Enable debug output to see detailed parsing information')
    
    args = parser.parse_args()
    
    # Configure logging based on debug flag
    if args.debug:
        logging.basicConfig(
            level=logging.DEBUG,
            format='%(message)s'
        )
    else:
        logging.basicConfig(
            level=logging.CRITICAL,
            format='%(message)s'
        )
    
    # Auto-discover is enabled by default unless --no-auto-discover is specified
    auto_discover = not args.no_auto_discover
    
    if args.progress:
        process_progress_code(args.progress, args.search, args.platform_codes, auto_discover)
    elif args.error:
        process_error_code(args.error, args.search, args.platform_codes, auto_discover)


if __name__ == '__main__':
    main()
