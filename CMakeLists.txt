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
set(VITA_VERSION "01.00")

add_definitions(-DAPP_VERSION="${VITA_VERSION}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -g -mtune=cortex-a9 -mfpu=neon -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-psabi -Wno-unused-but-set-variable -fno-exceptions -fno-rtti -std=gnu++17 -DIMGUI_DISABLE_OBSOLETE_FUNCTIONS")
set(VITA_MKSFOEX_FLAGS "${VITA_MKSFOEX_FLAGS} -d PARENTAL_LEVEL=1")

add_definitions(-DSQLITE_OS_OTHER=1 -DSQLITE_TEMP_STORE=3 -DSQLITE_THREADSAFE=0)

include_directories(
  libs/imgui
  libs/libsqlite
  include
)

link_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/libs/lib
)

add_executable(${PROJECT_NAME}
  libs/imgui/imgui.cpp
  libs/imgui/imgui_draw.cpp
  libs/imgui/imgui_impl_vitagl.cpp
  libs/imgui/imgui_tables.cpp
  libs/imgui/imgui_widgets.cpp
  libs/libsqlite/sqlite.c
  source/applist.cpp
  source/fs.cpp
  source/gui.cpp
  source/keyboard.cpp
  source/loadouts.cpp
  source/log.cpp
  source/main.cpp
  source/textures.cpp
  source/utils.cpp
)

target_link_libraries(${PROJECT_NAME}
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
  SceLibKernel_stub
  ScePower_stub
  SceShaccCg_stub
  SceSqlite_stub
  SceSysmodule_stub
  SceTouch_stub
)

## Create Vita files
vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME} UNSAFE)
# The FILE directive lets you add additional files to the VPK, the syntax is
# FILE src_path dst_path_in_vpk. In this case, we add the LiveArea paths.
vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
  FILE sce_sys/icon0.png sce_sys/icon0.png
  FILE sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
  FILE sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
  FILE sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
  FILE res/app.png res/app.png
  FILE res/db.png res/db.png
  FILE res/folder.png res/folder.png
  FILE res/splashscreen.png res/splashscreen.png
)
