cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{VITASDK})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
    else()
        message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
    endif()
endif()

project(VITA-Homebrew-Sorter)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "VITA Homebrew Sorter")
set(VITA_TITLEID "VITAHBSRT")
set(VITA_VERSION "01.25")

add_definitions(-DAPP_VERSION="${VITA_VERSION}")
add_definitions(
    -DIMGUI_DISABLE_OBSOLETE_FUNCTIONS -DIMGUI_DISABLE_DEMO_WINDOWS -DIMGUI_DISABLE_DEBUG_TOOLS
    -DIMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS -DIMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS
    -DIMGUI_DISABLE_WIN32_FUNCTIONS -DIMGUI_DISABLE_STB_TRUETYPE_IMPLEMENTATION -DIMGUI_ENABLE_FREETYPE
)
add_definitions(
    -DSQLITE_OMIT_WAL -DSQLITE_OS_OTHER
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -ffast-math -mtune=cortex-a9 -mfpu=neon -Wall -Wno-psabi -fno-rtti -std=gnu++17")
set(VITA_MKSFOEX_FLAGS "${VITA_MKSFOEX_FLAGS} -d PARENTAL_LEVEL=1")

include_directories(
    ${VITASDK}/arm-vita-eabi/include/freetype2/
    libs/imgui
    libs/sqlite3
    include
)

link_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/libs/lib
)

add_executable(${PROJECT_NAME}
    libs/imgui/imgui.cpp
    libs/imgui/imgui_draw.cpp
    libs/imgui/imgui_tables.cpp
    libs/imgui/imgui_widgets.cpp
    libs/imgui/misc/freetype/imgui_freetype.cpp
    libs/sqlite3/sqlite3.c
    source/applist.cpp
    source/config.cpp
    source/fs.cpp
    source/gui.cpp
    source/imgui_impl_vitagl.cpp
    source/keyboard.cpp
    source/loadouts.cpp
    source/log.cpp
    source/main.cpp
    source/power.cpp
    source/sqlite.cpp
    source/textures.cpp
    source/utils.cpp
)

target_link_libraries(${PROJECT_NAME}
    freetype
    png
    pthread
    lzma
    z
    vitaGL
    vitashark
    mathneon
    SceAppMgr_stub
    SceAppUtil_stub
    SceCommonDialog_stub
    SceCtrl_stub
    SceDisplay_stub
    SceGxm_stub
    SceKernelDmacMgr_stub
    SceLibJson_stub
    SceLibKernel_stub
    ScePower_stub
    SceShaccCg_stub
    SceShaccCgExt
    SceSysmodule_stub
    SceTouch_stub
    taihen_stub
)

## Create Vita files
vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME} UNSAFE)
# The FILE directive lets you add additional files to the VPK, the syntax is
# FILE src_path dst_path_in_vpk. In this case, we add the LiveArea paths.
vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
    VERSION ${VITA_VERSION}
    NAME ${VITA_APP_NAME}
    FILE sce_sys/icon0.png sce_sys/icon0.png
    FILE sce_sys/pic0.png sce_sys/pic0.png
    FILE sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
    FILE sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
    FILE sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
    FILE res/app.png res/app.png
    FILE res/db.png res/db.png
    FILE res/folder.png res/folder.png
    FILE res/page.png res/page.png
    FILE res/checked.png res/checked.png
    FILE res/unchecked.png res/unchecked.png
    FILE res/trash.png res/trash.png
)
