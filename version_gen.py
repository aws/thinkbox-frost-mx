# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Generate Version Files for FrostMX
"""

import argparse
import hashlib
import pathlib

RC_FILE_TEMPLATE: str = """
VS_VERSION_INFO VERSIONINFO
 FILEVERSION {major},{minor},{patch}
 PRODUCTVERSION {major},{minor},{patch}
 FILEFLAGSMASK 0x17L
#ifdef _DEBUG
 FILEFLAGS 0x1L
#else
 FILEFLAGS 0x0L
#endif
 FILEOS 0x4L
 FILETYPE 0x2L
 FILESUBTYPE 0x0L
BEGIN
	BLOCK "StringFileInfo"
	BEGIN
		BLOCK "040904b0"
		BEGIN
			VALUE "Comments", "Thinkbox FrostMX"
			VALUE "CompanyName", "Thinkbox Software"
			VALUE "FileDescription", "FrostMX Dynamic Link Library"
			VALUE "FileVersion", "{major}.{minor}.{patch}"
			VALUE "InternalName", "FrostMX"
			VALUE "LegalCopyright", "Copyright (C) 2022"
			VALUE "OriginalFilename", "Frost.dlo"
			VALUE "ProductName", "FrostMX"
			VALUE "ProductVersion", "{major}.{minor}.{patch}"
		END
	END
	BLOCK "VarFileInfo"
	BEGIN
		VALUE "Translation", 0x409, 1200
	END
END
"""

HEADER_FILE_TEMPLATE: str = """
#pragma once
/////////////////////////////////////////////////////
// AWS Thinkbox auto generated version include file.
/////////////////////////////////////////////////////

#define FRANTIC_VERSION "{version}"
#define FRANTIC_MAJOR_VERSION {major}
#define FRANTIC_MINOR_VERSION {minor}
#define FRANTIC_PATCH_NUMBER {patch}
#define FRANTIC_DESCRIPTION "Thinkbox Frost for 3ds Max"
"""

def write_version_header(version: str, filename: str='FrostMXVersion.h') -> None:
    """
    Write a header file with the version data.
    """
    major, minor, patch = version.split('.')
    with open(filename, 'w', encoding='utf8') as version_header:
        version_header.write(HEADER_FILE_TEMPLATE.format(
            version=version,
            major=major,
            minor=minor,
            patch=patch
        ))


def write_version_hash(version: str, source_folder: str, filename: str='FrostVersionHash.hpp'):
    """
    Write a header file with a hash that should match between FrostMX and FrostMXVRay.
    """
    interface_files = [
        pathlib.Path(source_folder) / 'include' / 'FrostInterface.hpp',
        pathlib.Path(source_folder) / 'FrostVRY' / 'include' / 'FrostVRay.hpp'
    ]

    hash_collector = hashlib.md5()
    hash_collector.update(version.encode('utf8'))
    for interface_file_name in interface_files:
        with open(interface_file_name, 'rb') as interface_file:
            hash_collector.update(interface_file.read())

    digest = hash_collector.hexdigest()
    frost_version_hash_content = f'#pragma once\n#define FROST_VERSION_HASH 0x{digest[0:16]}\n'
    with open(filename, 'w', encoding='utf8') as version_hash_header:
        version_hash_header.write(frost_version_hash_content)


def write_version_resource(version: str, filename: str='FrostMXVersion.rc') -> None:
    """
    Write an rc file with the version data.
    """
    major, minor, patch = version.split('.')
    with open(filename, 'w', encoding='utf8') as version_resource:
        version_resource.write(RC_FILE_TEMPLATE.format(
            major=major,
            minor=minor,
            patch=patch
        ))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument(dest='version', required=True, type=str, help='The version number to use.')
    args = parser.parse_args()
    write_version_header(args.version)
    write_version_resource(args.version)
    write_version_hash(args.version, '')
