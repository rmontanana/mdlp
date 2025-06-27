import re
from conan import ConanFile, CMake

class FimdlpConan(ConanFile):
    name = "fimdlp"
    version = "X.X.X"
    license = "MIT"
    author = "Ricardo Montañana <rmontanana@gmail.com>"
    url = "https://github.com/rmontanana/mdlp"
    description = "Discretization algorithm based on the paper by Fayyad & Irani."
    topics = ("discretization", "classification", "machine learning")
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake"
    exports_sources = "src/*", "CMakeLists.txt", "README.md"

    def init(self):
        # Read the CMakeLists.txt file to get the version
        # This is a simple example; you might want to use a more robust method
        # to parse the CMakeLists.txt file.
        # For example, you could use a regex to extract the version number.
        with open("CMakeLists.txt", "r") as f:
            lines = f.readlines()
        for line in lines:
            if "VERSION" in line:
                # Extract the version number using regex
                match = re.search(r"VERSION\s+(\d+\.\d+\.\d+)", line)
                if match:
                    self.version = match.group(1)

    def requirements(self):
        self.requires("libtorch/2.7.0")  # Adjust version as necessary

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        # self.copy("*.h", dst="include", src="src/include")
        # self.copy("*fimdlp*", dst="lib", keep_path=False)
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["fimdlp"]