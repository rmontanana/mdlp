import os
from conan import ConanFile
from conan.tools.cmake import CMake, cmake_layout
from conan.tools.build import can_run


class FimdlpTestConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    # VirtualBuildEnv and VirtualRunEnv can be avoided if "tools.env:CONAN_RUN_TESTS" is false
    generators = "CMakeDeps", "CMakeToolchain", "VirtualRunEnv"
    apply_env = False  # avoid the default VirtualBuildEnv from the base class
    test_type = "explicit"

    def requirements(self):
        self.requires(self.tested_reference_str)

    def layout(self):
        cmake_layout(self)

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def test(self):
        if can_run(self):
            cmd = os.path.join(self.cpp.build.bindir, "test_fimdlp")
            self.run(cmd, env="conanrun")