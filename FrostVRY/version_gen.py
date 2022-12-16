# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Generate Version Files for FrostVRY
"""

import argparse
import hashlib
import pathlib
import re

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
			VALUE "Comments", "Thinkbox FrostVRY"
			VALUE "CompanyName", "Thinkbox Software"
			VALUE "FileDescription", "FrostVRY Dynamic Link Library"
			VALUE "FileVersion", "{major}.{minor}.{patch}"
			VALUE "InternalName", "FrostVRY"
			VALUE "LegalCopyright", "Copyright (C) 2022"
			VALUE "OriginalFilename", "FrostVRY.dll"
			VALUE "ProductName", "FrostVRY"
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
#define FRANTIC_DESCRIPTION "Thinkbox Frost for V-Ray"
"""

def write_version_header(version: str, filename: str='FrostVersion.h') -> None:
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
    Write a header file with a hash that should match between FrostMX and FrostMXVRY.
    """

    interface_path = pathlib.Path(source_folder) / '..' / 'include' / 'FrostInterface.hpp'
    if not interface_path.exists():
        interface_path = interface_path = pathlib.Path(source_folder) / 'include' / 'FrostInterface.hpp'

    interface_files = [
        interface_path,
        pathlib.Path(source_folder) / 'include' / 'FrostVRay.hpp'
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


def write_version_resource(version: str, filename: str='FrostVersion.rc') -> None:
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


def read_vray_version(vraybase_ver_path: str) -> str:
    """
    Given the path to the vraybase_ver.h header file,
    extract the V-Ray version.
    """
    definition_regex = re.compile(r'^#define VRAY_DLL_VERSION_[A-Z]*\s*([0-9]*)$')
    result = []
    with open(vraybase_ver_path, 'r', encoding='utf8') as version_header:
        for line in version_header.readlines():
            re_result = definition_regex.match(line)
            if re_result:
                result.append(re_result.group(1))
    return ''.join(result[:-1])


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument(dest='version', required=True, type=str, help='The version number to use.')
    args = parser.parse_args()
    write_version_header(args.version)
    write_version_resource(args.version)
    write_version_hash(args.version, '')
