# Pure C core (ADR-0003): an ESP-IDF component inside firmware/, a plain static
# library in the host build, where any esp_* or lvgl include fails to compile.
set(GG_CORE_SRCS
    src/gg_app.c
    src/gg_battery.c
    src/gg_civil.c
    src/gg_input.c
)

if(ESP_PLATFORM)
    idf_component_register(SRCS ${GG_CORE_SRCS} INCLUDE_DIRS include)
else()
    add_library(gg_core STATIC ${GG_CORE_SRCS})
    target_include_directories(gg_core PUBLIC include)
    target_link_libraries(gg_core PRIVATE gg_warnings)
endif()
