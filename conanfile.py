from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps, cmake_layout


class FocusForgeConan(ConanFile):
    name = "focusforge"
    version = "1.0.0"
    license = "MIT"
    description = "Telegram Task & Focus Manager — C++ / userver"
    settings = "os", "compiler", "build_type", "arch"

    # userver подключается отдельно через системный пакет или cmake find_package.
    # Здесь указываем вспомогательные зависимости.
    requires = [
        "gtest/1.14.0",
        "fmt/10.2.1",
        "nlohmann_json/3.11.3",
        "date/3.0.1",         # Howard Hinnant date library (UTC helpers)
        "openssl/3.2.0",
    ]

    options = {
        "with_postgres": [True, False],
        "with_redis":    [True, False],
        "with_mongo":    [True, False],
        "build_tests":   [True, False],
    }
    default_options = {
        "with_postgres": True,
        "with_redis":    True,
        "with_mongo":    True,
        "build_tests":   True,
        "gtest/*:build_gmock": True,
    }

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["BUILD_TESTS"] = self.options.build_tests
        tc.generate()
        deps = CMakeDeps(self)
        deps.generate()
