cmake_minimum_required(VERSION 3.22)

# core/ and ui/ live at the repository root and also build on the host (ADR-0003).
set(EXTRA_COMPONENT_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/../core
    ${CMAKE_CURRENT_LIST_DIR}/../ui
)

# One LVGL configuration shared with the simulator (ADR-0002).
set(SDKCONFIG_DEFAULTS "sdkconfig.defaults;${CMAKE_CURRENT_LIST_DIR}/../config/lvgl.defconfig")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(gogadget)
