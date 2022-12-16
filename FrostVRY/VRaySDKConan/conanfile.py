# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
from typing import Any
from conans import ConanFile

import os
import pathlib
import shutil

VALID_MAX_CONFIGS: dict[tuple[str, str], set[str]] = {
    ('Visual Studio', '16'): { '2022', '2023' },
}

SETTINGS: dict[str, Any] = {
    'os': ['Windows'],
    'compiler': {
        'Visual Studio': {'version': ['16']},
    },
    'build_type': None,
    'arch': 'x86_64'
}

DEFAULT_VRAY_PATH: str = 'C:/Program Files/Chaos Group/V-Ray/3ds Max {}'

class VRayMaxSDKConan(ConanFile):
    name: str = 'vraysdk'
    version: str = '1.0.0'
    description: str = 'A Conan package containing the Chaos Group V-Ray SDK.'
    settings: dict[str, Any] = SETTINGS
    options: dict[str, Any] = {
        'max_version': ['2022', '2023'],
        'vray_path': 'ANY'
    }

    def configure(self) -> None:
        if self.options.max_version == None:
            self.options.max_version = '2023'

        if self.options.vray_path == None:
            self.options.vray_path = DEFAULT_VRAY_PATH.format(self.options.max_version)

    def validate(self) -> None:
        compiler = str(self.settings.compiler)
        compiler_version = str(self.settings.compiler.version)
        compiler_tuple = (compiler, compiler_version)
        max_version = str(self.options.max_version)
        if max_version not in VALID_MAX_CONFIGS[compiler_tuple]:
            raise Exception(f'{str(compiler_tuple)} is not a valid configuration for 3ds Max {max_version}')

    def build(self) -> None:
        # Copy Headers
        build_include_dir = os.path.join(self.build_folder, 'include')
        if os.path.exists(build_include_dir):
            shutil.rmtree(build_include_dir)
        shutil.copytree(
            os.path.join(str(self.options.vray_path), 'include'),
            build_include_dir
        )

        # Copy Libraries
        build_library_dir = os.path.join(
            self.build_folder,
            'lib'
        )
        if os.path.exists(build_library_dir):
            shutil.rmtree(build_library_dir)
        os.makedirs(build_library_dir)

        lib_source_path = pathlib.Path(str(self.options.vray_path)) / 'lib'
        for lib in lib_source_path.glob('*.lib'):
            shutil.copy(
                str(lib),
                build_library_dir
            )

    def package(self) -> None:
        self.copy('*', dst='lib', src='lib')
        self.copy('*', dst='include', src='include')

    def package_info(self) -> None:
        self.cpp_info.libs = self.collect_libs()

    def deploy(self) -> None:
        self.copy('*', dst='lib', src='lib')
        self.copy('*', dst='include', src='include')
