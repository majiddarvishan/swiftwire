cmake_minimum_required(VERSION 3.16)
project(swiftwire LANGUAGES CXX VERSION 0.1.0)

option(SWIFTWIRE_BUILD_EXAMPLES "Build SwiftWire examples" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Threads REQUIRED)
find_package(Boost 1.74 REQUIRED COMPONENTS system)

add_subdirectory(src)

if(SWIFTWIRE_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

# Optional install
include(GNUInstallDirs)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS swiftwire EXPORT swiftwireTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(EXPORT swiftwireTargets
        NAMESPACE swiftwire::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/swiftwire)
