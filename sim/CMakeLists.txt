# Host build (ADR-0003, ADR-0005): core unit tests, golden screenshots of ui/,
# and an optional SDL viewer. No ESP-IDF headers exist here, so an esp_* include
# in core/ or ui/ fails to compile.
cmake_minimum_required(VERSION 3.20)
project(gogadget_host LANGUAGES C CXX)

set(GG_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
option(GG_GOLDEN_UPDATE "Create missing golden screenshots instead of failing (make golden)" OFF)
option(GG_SIM_VIEWER "Build the SDL viewer when SDL2 is available" ON)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# Plain char is unsigned on Xtensa; match it (ADR-0014).
add_compile_options(-funsigned-char)

add_library(gg_warnings INTERFACE)
target_compile_options(gg_warnings INTERFACE -Wall -Wextra -Werror)

include(FetchContent)

# LVGL: the exact registry archive the firmware's component manager downloads
# (ui/idf_component.yml pins lvgl/lvgl ==9.6.0~1). CI checks the two agree.
set(GG_LVGL_VERSION "9.6.0~1")
FetchContent_Declare(lvgl
    URL "https://components-file.espressif.com/components/lvgl/lvgl/9.6.0~1/lvgl__lvgl-v9.6.0_1.zip"
    URL_HASH SHA256=ad10122975bf5c3cdab55fbd2157fcaf1f67afe59792ee7f462d1ebdc18697b7
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_Declare(unity
    URL "https://github.com/ThrowTheSwitch/Unity/archive/refs/tags/v2.6.1.tar.gz"
    URL_HASH SHA256=b41a66d45a6b99758fb3202ace6178177014d52fc524bf1f72687d93e9867292
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

set(GG_WITH_SDL OFF)
if(GG_SIM_VIEWER)
    find_package(SDL2 QUIET)
    if(SDL2_FOUND)
        set(GG_WITH_SDL ON)
    endif()
endif()

# One LVGL configuration for device and host: config/lvgl.defconfig, plus the
# simulator's own fragments (ADR-0002).
set(GG_LVGL_DEFCONFIGS ${GG_ROOT}/config/lvgl.defconfig ${CMAKE_CURRENT_SOURCE_DIR}/lvgl.sim.defconfig)
if(GG_GOLDEN_UPDATE)
    list(APPEND GG_LVGL_DEFCONFIGS ${CMAKE_CURRENT_SOURCE_DIR}/lvgl.golden-update.defconfig)
endif()
if(GG_WITH_SDL)
    list(APPEND GG_LVGL_DEFCONFIGS ${CMAKE_CURRENT_SOURCE_DIR}/lvgl.sdl.defconfig)
endif()
set(LV_BUILD_USE_KCONFIG ON CACHE BOOL "" FORCE)
set(LV_BUILD_DEFCONFIG_PATH "${GG_LVGL_DEFCONFIGS}" CACHE STRING "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_THORVG OFF CACHE BOOL "" FORCE)
set(LV_BUILD_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(lvgl unity)

# lv_test_screenshot_compare() resolves reference names against this prefix.
target_compile_definitions(lvgl PRIVATE REF_IMGS_PATH="${GG_ROOT}/ui/test/golden/")

add_subdirectory(${GG_ROOT}/core ${CMAKE_BINARY_DIR}/core)
add_subdirectory(${GG_ROOT}/ui ${CMAKE_BINARY_DIR}/ui)

enable_testing()

foreach(name input civil battery app)
    add_executable(test_${name} ${GG_ROOT}/core/test/test_${name}.c)
    target_link_libraries(test_${name} PRIVATE gg_core unity gg_warnings)
    add_test(NAME core_${name} COMMAND test_${name})
endforeach()

add_executable(test_screens ${GG_ROOT}/ui/test/test_screens.c)
target_link_libraries(test_screens PRIVATE gg_ui unity gg_warnings)
add_test(NAME ui_screens COMMAND test_screens)

if(GG_WITH_SDL)
    add_executable(gg_sim sim_main.c)
    target_link_libraries(gg_sim PRIVATE gg_ui gg_warnings)
    message(STATUS "gogadget: SDL viewer enabled (gg_sim)")
else()
    message(STATUS "gogadget: SDL2 not found or GG_SIM_VIEWER=OFF, viewer skipped")
endif()
