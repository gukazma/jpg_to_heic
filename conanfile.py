from conans import ConanFile, tools
import platform


class ModuleConan(ConanFile):
    name = "QtConanExample"
    description = "An example for Qt with Conan"
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake", "cmake_find_package_multi", "cmake_paths"

    def configure(self):
        del self.settings.compiler.cppstd

    def requirements(self):
        self.requires("gtest/1.14.0")
        self.requires("libjpeg-turbo/3.0.2")
        self.requires("libsvtav1/1.7.0")
        self.requires("libx265/3.4")
        self.requires("libheif/1.16.2")
        
    def imports(self):
        self.copy("*.dll", "./bin", "bin")
        self.copy("*.so", "./bin", "bin")