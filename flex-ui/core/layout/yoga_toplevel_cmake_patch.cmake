# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.
#
# FlexUI patch: removed add_subdirectory(tests) and add_subdirectory(fuzz)
# to avoid duplicate GTest/GoogleBenchmark conflicts when vendored into FlexUI.

cmake_minimum_required(VERSION 3.13...3.26)
project(yoga-all)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/project-defaults.cmake)

add_subdirectory(yoga)

# cmake install config
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS yogacore EXPORT yoga-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY
    "${CMAKE_CURRENT_LIST_DIR}/yoga"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

install(EXPORT yoga-targets
    FILE yoga-targets.cmake
    NAMESPACE yoga::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/yoga
)
