##
# Plugin to evaluate a platform's build report file and verify common settings.
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##


from pathlib import Path
import logging
from typing import List, Optional

from edk2toollib.database.edk2_db import Edk2DB, TableGenerator, Session
from edk2toollib.uefi.edk2.path_utilities import Edk2Path
from edk2toollib.uefi.edk2.parsers.buildreport_parser import BuildReport

from sqlalchemy import ForeignKey
from sqlalchemy.orm import Mapped, mapped_column, relationship


class BrComponent(Edk2DB.Base):
    """A class to represent a component in the database."""

    __tablename__ = "br_component"

    guid: Mapped[str] = mapped_column(primary_key=True)
    name: Mapped[str]
    type: Mapped[Optional[str]]
    path: Mapped[str]
    fv_name: Mapped[Optional[str]]
    depex: Mapped[Optional[str]]
    libraries: Mapped[List["BrLibrary"]] = relationship()
    pcds: Mapped[List["BrPcd"]] = relationship()


class BrLibrary(Edk2DB.Base):
    """A class to represent a library in the database."""

    __tablename__ = "br_library"
    id: Mapped[int] = mapped_column(primary_key=True, autoincrement=True)
    guid: Mapped[str] = mapped_column(ForeignKey("br_component.guid"))
    cls: Mapped[str]
    path: Mapped[str]


class BrPcd(Edk2DB.Base):
    """A class to represent a pcd in the database."""

    __tablename__ = "br_pcd"
    id: Mapped[int] = mapped_column(primary_key=True, autoincrement=True)
    guid: Mapped[str] = mapped_column(ForeignKey("br_component.guid"))
    token_space: Mapped[str]
    token: Mapped[str]
    value: Mapped[str]


class BuildReportTable(TableGenerator):
    def parse(self, session: Session, pathobj: Edk2Path, _id: str, env: dict) -> None:
        build_report = Path(env.get("BUILDREPORT_FILE", ""))
        if not build_report.exists():
            logging.warning(
                "Build report not present, skipping build report table generation."
            )
            return

        report = BuildReport(
            build_report, pathobj.WorkspacePath, ",".join(pathobj.PackagePathList), {}
        )
        report.BasicParse()

        for module in report.Modules.values():
            library_list = []
            pcd_list = []

            for cls, library in module.Libraries.items():
                library_list.append(BrLibrary(guid=module.Guid, cls=cls, path=library))

            for cls, value in module.PCDs.items():
                token_space, token = cls.split(".", 1)
                pcd_list.append(
                    BrPcd(
                        guid=module.Guid,
                        token_space=token_space,
                        token=token,
                        value=value,
                    )
                )
            session.add(
                BrComponent(
                    guid=module.Guid,
                    name=module.Name,
                    type=module.Type or None,
                    path=module.InfPath,
                    fv_name=module.FvName or None,
                    depex=module.Depex or None,
                    libraries=library_list,
                    pcds=pcd_list,
                )
            )

        return
