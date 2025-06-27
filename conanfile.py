import re
import os
from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout, CMakeDeps
from conan.tools.files import save, load

class FimdlpConan(ConanFile):
    name = "fimdlp"
    version = "X.X.X"
    license = "MIT"
    author = "Ricardo Montañana <rmontanana@gmail.com>"
    url = "https://github.com/rmontanana/mdlp"
    description = "Discretization algorithm based on the paper by Fayyad & Irani."
    topics = ("discretization", "classification", "machine learning")
    settings = "os", "compiler", "build_type", "arch"
    exports_sources = "src/*", "CMakeLists.txt", "README.md", "config/*", "fimdlpConfig.cmake.in"

    def set_version(self):
        # Read the CMakeLists.txt file to get the version
        try:
            content = load(self, "CMakeLists.txt")
            match = re.search(r"VERSION\s+(\d+\.\d+\.\d+)", content)
            if match:
                self.version = match.group(1)
        except Exception:
            self.version = "2.0.1"  # fallback version

    def requirements(self):
        self.requires("libtorch/2.7.0")
        
    def layout(self):
        cmake_layout(self)
        
    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["fimdlp"]
        self.cpp_info.includedirs = ["include"]
        self.cpp_info.libdirs = ["lib"]
        self.cpp_info.set_property("cmake_find_mode", "both")
        self.cpp_info.set_property("cmake_target_name", "fimdlp::fimdlp")
        self.cpp_info.set_property("cmake_file_name", "fimdlp")