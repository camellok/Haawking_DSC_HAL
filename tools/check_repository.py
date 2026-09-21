#!/usr/bin/env python3
"""Validate repository boundaries that do not require the target toolchain."""

import re
import sys
from pathlib import Path
from typing import List


ROOT = Path(__file__).resolve().parents[1]
SOURCE_SUFFIXES = {".c", ".cpp", ".h"}
FORBIDDEN_SUFFIXES = {".a", ".elf", ".hex", ".lib", ".o", ".obj", ".out"}
SPDX_LINE = "SPDX-License-Identifier: Apache-2.0"
COPYRIGHT_LINE = "Copyright (c) 2019-2026, Beijing Haawking Technology Co., Ltd"
AUTHOR_LINE = "Author: Silin Luo"
EMAIL_LINE = "Email : silin.luo@mail.haawking.com"
MARKDOWN_LINK = re.compile(r"!?\[[^\]]*\]\(([^)]+)\)")


def iter_repository_files() -> List[Path]:
    return [
        path
        for path in ROOT.rglob("*")
        if path.is_file() and ".git" not in path.parts
    ]


def check_source_files(files: List[Path]) -> List[str]:
    errors: List[str] = []

    for path in files:
        if path.suffix.lower() not in SOURCE_SUFFIXES:
            continue

        text = path.read_text(encoding="utf-8")
        relative_path = path.relative_to(ROOT)
        header = "\n".join(text.splitlines()[:16])
        if SPDX_LINE not in "\n".join(text.splitlines()[:5]):
            errors.append(f"{relative_path}: missing Apache-2.0 SPDX header")

        if COPYRIGHT_LINE not in header:
            errors.append(f"{relative_path}: missing standard copyright line")

        if AUTHOR_LINE not in header or EMAIL_LINE not in header:
            errors.append(f"{relative_path}: missing standard author information")

        if f"File  : {path.name}" not in header:
            errors.append(f"{relative_path}: File header does not match filename")

        if re.search(r"^ \* Description: \S.*$", header, re.MULTILINE) is None:
            errors.append(f"{relative_path}: missing file description")

        if relative_path.parts[:2] == ("include", "hal"):
            if "driverlib" in text.lower():
                errors.append(f"{relative_path}: public header references DriverLib")

        if relative_path.parts[:2] == ("source", "common"):
            if "driverlib" in text.lower():
                errors.append(f"{relative_path}: common source references DriverLib")

    return errors


def check_forbidden_files(files: List[Path]) -> List[str]:
    errors: List[str] = []

    for path in files:
        relative_path = path.relative_to(ROOT)
        if path.suffix.lower() in FORBIDDEN_SUFFIXES:
            errors.append(f"{relative_path}: generated or binary artifact is forbidden")

    return errors


def check_markdown_links(files: List[Path]) -> List[str]:
    errors: List[str] = []

    for path in files:
        if path.suffix.lower() != ".md":
            continue

        text = path.read_text(encoding="utf-8")
        for match in MARKDOWN_LINK.finditer(text):
            target = match.group(1).strip().strip("<>")
            if not target or target.startswith(("#", "http://", "https://", "mailto:")):
                continue

            target_path = target.split("#", maxsplit=1)[0]
            if not target_path:
                continue

            resolved = (path.parent / target_path).resolve()
            if not resolved.exists():
                relative_path = path.relative_to(ROOT)
                errors.append(f"{relative_path}: broken local link {target}")

    return errors


def main() -> int:
    files = iter_repository_files()
    errors = []
    errors.extend(check_source_files(files))
    errors.extend(check_forbidden_files(files))
    errors.extend(check_markdown_links(files))

    if errors:
        print("Repository checks failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print(f"Repository checks passed ({len(files)} files inspected).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
