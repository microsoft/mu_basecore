## @file BuildReportQueries.yaml
#
# Plugin to evaluate a platform's build report file and verify common settings
#
#  Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

from edk2toollib.database.edk2_db import Edk2DB
from buildreport_table import BrComponent, BrPcd, BrLibrary
import logging


def CompiledButButNotIncludedInAnyFV(edk2db: Edk2DB, reportlevel: str) -> int:
    """
    Identifies components that are compiled but not included in any Firmware
    Volume (FV). This function queries the database to find components that
    are compiled but not included in any FV, excluding those of type 
    'APPLICATION' and those whose paths end with 'Lib'. It logs a warning for
    each such component found and returns the total count of these components.
    Args:
        edk2db (Edk2DB): An instance of the Edk2DB class to interact with the database.
        reportlevel (str): The level of reporting to be used (not utilized in the current implementation).
    Returns:
        int: The number of components that are compiled but not included in any FV.
    """
    error_count = 0
    with edk2db.session() as session:
        results = (
            session.query(BrComponent)
            .filter(BrComponent.fv_name == None)
            .filter(BrComponent.type != "APPLICATION")
            .filter(BrComponent.path.endswith("Lib") == False)
            .all()
        )

        for result in results:
            logging.warning(f"{result.path} is compiled but not included in any FV")
            error_count += 1
    return error_count

def SetupDxeExistsInFV(edk2db: Edk2DB, reportlevel: str) -> int:
    """
    Checks if 'SetupDxe' exists in the Firmware Volume (FV) and logs a warning
    for each occurrence.
    Args:
        edk2db (Edk2DB): The database object to query.
        reportlevel (str): The level of reporting.
    Returns:
        int: The number of occurrences of 'SetupDxe' in the FV.
    """
    error_count = 0
    with edk2db.session() as session:
        results = (
            session.query(BrComponent)
            .filter(BrComponent.fv_name != None)
            .filter(BrComponent.path.contains("SetupDxe"))
        ).all()

        for result in results:
            logging.warning(f"{result.path} SetupDxe is being included in FV")
            error_count += 1
    return error_count

def PcdConOutUgaSupportIsNotZero(edk2db: Edk2DB, reportlevel: str) -> int:
    """
    Checks if the PcdConOutUgaSupport value is not zero in the database and logs a warning for each occurrence.
    Args:
        edk2db (Edk2DB): The database connection object.
        reportlevel (str): The level of the report.
    Returns:
        int: The count of occurrences where PcdConOutUgaSupport is not zero.
    """
    error_count = 0

    with edk2db.session() as session:
        results = (
            session.query(BrComponent)
            .join(BrPcd)
            .filter(BrPcd.token == "PcdConOutUgaSupport" and BrPcd.value == "1")
        ).all()

        for result in results:
            logging.warning(f"{result.path} PcdConOutUgaSupport is not 0")
            error_count += 1
    return error_count


def PcdTurnOffUsbLegacySupportIsNotOne(edk2db: Edk2DB, reportlevel: str) -> int:
    """
    Checks if the PcdTurnOffUsbLegacySupport PCD is not set to 1 in the database.

    Args:
        edk2db (Edk2DB): The database object to query.
        reportlevel (str): The level of reporting (not used in the function).

    Returns:
        int: The count of occurrences where PcdTurnOffUsbLegacySupport is not set to 1.
    """
    error_count = 0
    with edk2db.session() as session:
        results = (
            session.query(BrComponent)
            .join(BrPcd)
            .filter(BrPcd.token == "PcdTurnOffUsbLegacySupport" and BrPcd.value == "0")
        ).all()
        for result in results:
            logging.warning(f"{result.path} PcdTurnOffUsbLegacySupport is not 1")
            error_count += 1
    return error_count


def Ps2UsedInFv(edk2db: Edk2DB, reportlevel: str) -> int:
    """
    Checks if any components with "Ps2" in their path are included in the firmware volume (FV) and logs a warning for each occurrence.
    Args:
        edk2db (Edk2DB): The database object to query.
        reportlevel (str): The level of reporting to be used (not utilized in the current implementation).
    Returns:
        int: The number of components with "Ps2" in their path that are included in the FV.
    """
    error_count = 0
    with edk2db.session() as session:
        results = (
            session.query(BrComponent)
            .filter(BrComponent.fv_name is not None)
            .filter(BrComponent.path.contains("Ps2"))
        ).all()

        for result in results:
            logging.warning(f"{result.path} Ps2 is being included in FV")
            error_count += 1
    return error_count


def HddPasswordDxeExistsInFV(edk2db: Edk2DB, reportlevel: str) -> int:
    """
    Checks if the HddPasswordDxe component exists in the firmware volume (FV) and logs a warning for each occurrence.
    Args:
        edk2db (Edk2DB): An instance of the Edk2DB class to interact with the database.
        reportlevel (str): The level of reporting to be used (not utilized in the current implementation).
    Returns:
        int: The number of occurrences of HddPasswordDxe in the FV.
    """
    error_count = 0
    with edk2db.session() as session:
        results = (
            session.query(BrComponent)
            .filter(BrComponent.fv_name is not None)
            .filter(BrComponent.path.contains("HddPasswordDxe"))
        ).all()

        for result in results:
            logging.warning(f"{result.path} HddPasswordDxe is being included in FV")
            error_count += 1
    return error_count


def UiAppExistsInFV(edk2db: Edk2DB, reportlevel: str) -> int:
    """
    Checks if the UiApp is included in the Firmware Volume (FV) in the given database.
    Args:
        edk2db (Edk2DB): The database object to query.
        reportlevel (str): The level of reporting (not used in the function).
    Returns:
        int: The count of UiApp inclusions in the FV.
    Logs a warning for each instance of UiApp found in the FV.
    """
    error_count = 0
    with edk2db.session() as session:
        results = (
            session.query(BrComponent)
            .filter(BrComponent.fv_name.isnot(None))
            .filter(BrComponent.path.contains("UiApp"))
            .all()
        )

        for result in results:
            logging.warning(f"{result.path} UiApp is being included in FV")
            error_count += 1
    return error_count


def EbcDxeExistsInFV(edk2db: Edk2DB, reportlevel: str) -> int:
    """
    Checks if any EbcDxe components exist in the firmware volume (FV) and logs a warning for each occurrence.
    Args:
        edk2db (Edk2DB): An instance of the Edk2DB class to interact with the database.
        reportlevel (str): The level of reporting (not used in the current implementation).
    Returns:
        int: The number of EbcDxe components found in the FV.
    """
    error_count = 0
    with edk2db.session() as session:
        results = (
            session.query(BrComponent)
            .filter(BrComponent.fv_name != None)
            .filter(BrComponent.path.contains("/EbcDxe"))
            .all()
        )

        for result in results:
            logging.warning(f"{result.path} EbcDxe exists in FV")
            error_count += 1
    return error_count


def FailedPerformanceLibUsingNonNullLib(edk2db: Edk2DB, reportlevel: str) -> int:
    """
    Checks for instances of PerformanceLib that are not NULL in the database and logs warnings for each instance found.
    Args:
        edk2db (Edk2DB): The database object to query.
        reportlevel (str): The level of the report (not used in the function).
    Returns:
        int: The number of non-NULL PerformanceLib instances found.
    """
    error_count = 0
    with edk2db.session() as session:
        results = (
            session.query(BrComponent)
            .join(BrLibrary)
            .filter(
                (BrLibrary.cls == "PerformanceLib")
                and (~BrLibrary.path.contains("NULL"))
            )
            .all()
        )

        for result in results:
            logging.warning(f"{result.path} PerformanceLib instance is non NULL")
            error_count += 1
    return error_count


def ModuleDoesNotContainStackCheckLib(edk2db: Edk2DB, reportlevel: str) -> int:
    """
    Checks if modules in the database do not contain the StackCheckLib library and
    logs a warning for each module that does not.
    Args:
        edk2db (Edk2DB): The database object to query.
        reportlevel (str): The level of reporting to be used (not utilized in the current implementation).
    Returns:
        int: The number of modules that do not contain the StackCheckLib library.
    """
    error_count = 0
    with edk2db.session() as session:
        results = (
            session.query(BrComponent)
            .filter(BrComponent.fv_name is not None)
            .filter(BrComponent.type != "APPLICATION")
            .filter(BrComponent.type != "USER_DEFINED")
            .all()
        )

        for result in results:
            stack_found = False
            for lib in result.libraries:
                if "StackCheckLib" in lib.path:
                    stack_found = True
                    break

            if not stack_found:
                logging.warning(f"{result.path} does not contain StackCheckLib")
                error_count += 1
    return error_count
