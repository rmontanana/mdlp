import os
import re
from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.files import load, copy


class FimdlpConan(ConanFile):
    name = "fimdlp"
    version = "X.X.X"
    license = "MIT"
    author = "Ricardo Montañana <rmontanana@gmail.com>"
    url = "https://github.com/rmontanana/mdlp"
    description = "Discretization algorithm based on the paper by Fayyad & Irani Multi-Interval Discretization of Continuous-Valued Attributes for Classification Learning."
    topics = ("machine-learning", "discretization", "mdlp", "classification")
    
    # Package configuration
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "enable_testing": [True, False],
        "enable_sample": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "enable_testing": False,
        "enable_sample": False,
    }
    
    # Sources are located in the same place as this recipe, copy them to the recipe
    exports_sources = "CMakeLists.txt", "src/*", "sample/*", "tests/*", "config/*", "fimdlpConfig.cmake.in"

    def set_version(self):
        content = load(self, "CMakeLists.txt")
        version_pattern = re.compile(r'project\s*\([^\)]*VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)', re.IGNORECASE | re.DOTALL)
        match = version_pattern.search(content)
        if match:
            self.version = match.group(1)
        else:
            raise Exception("Version not found in CMakeLists.txt")
    
    def config_options(self):
        if self.settings.os == "Windows":
            self.options.rm_safe("fPIC")
    
    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")
    
    def requirements(self):
        # PyTorch dependency for tensor operations
        self.requires("libtorch/2.7.1")
        
    def build_requirements(self):
        self.requires("arff-files/1.2.0") # for tests and sample
        if self.options.enable_testing: 
            self.test_requires("gtest/1.16.0")
    
    def layout(self):
        cmake_layout(self)
    
    def generate(self):
        # Generate CMake configuration files
        deps = CMakeDeps(self)
        deps.generate()
        
        tc = CMakeToolchain(self)
        # Set CMake variables based on options
        tc.variables["ENABLE_TESTING"] = self.options.enable_testing
        tc.variables["ENABLE_SAMPLE"] = self.options.enable_sample
        tc.variables["BUILD_SHARED_LIBS"] = self.options.shared
        tc.generate()
    
    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        
        # Run tests if enabled
        if self.options.enable_testing:
            cmake.test()
    
    def package(self):
        # Install using CMake
        cmake = CMake(self)
        cmake.install()
        
        # Copy license file
        copy(self, "LICENSE", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))
    
    def package_info(self):
        # Library configuration
        self.cpp_info.libs = ["fimdlp"]
        self.cpp_info.includedirs = ["include"]
        
        # CMake package configuration
        self.cpp_info.set_property("cmake_file_name", "fimdlp")
        self.cpp_info.set_property("cmake_target_name", "fimdlp::fimdlp")
        
        # Compiler features
        self.cpp_info.cppstd = "17"
        
        # System libraries (if needed)
        if self.settings.os in ["Linux", "FreeBSD"]:
            self.cpp_info.system_libs.append("m")  # Math library
            self.cpp_info.system_libs.append("pthread")  # Threading
        
        # Build information for consumers
        self.cpp_info.builddirs = ["lib/cmake/fimdlp"]
