cmake_minimum_required(VERSION 3.16)

project(YGUninstaller LANGUAGES CXX RC)

# 强制使用 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 默认使用 Release
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# MinGW/GCC 下启用多核编译（部分生成器支持）
if(MSVC)
  message(FATAL_ERROR "This project is configured for GCC/MinGW. Please use MinGW Makefiles or Ninja with GCC.")
endif()

# 体积优化选项
option(SIZE_OPTIMIZED "Enable size-optimized build flags and optional UPX" ON)
option(ENABLE_LTO "Enable Link Time Optimization (may trigger GCC bugs)" OFF)

if(SIZE_OPTIMIZED AND MINGW)
  # 编译期：更小体积和死代码分离
  add_compile_options(
    $<$<CONFIG:Release>:-Os>
    $<$<CONFIG:Release>:-ffunction-sections>
    $<$<CONFIG:Release>:-fdata-sections>
  )
  # 链接期：剔除未使用的节和符号
  add_link_options(
    $<$<CONFIG:Release>:-Wl,--gc-sections>
    $<$<CONFIG:Release>:-s>
  )
  # LTO（可选，默认关闭以规避部分 GCC 版本的 ICE）
  if(ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT _ipo_supported OUTPUT _ipo_msg)
    if(_ipo_supported)
      set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
      # 个别源文件在特定 GCC 版本上会触发 ICE，单独禁用其 IPO
      set_source_files_properties(src/services/ProgramDetector.cpp PROPERTIES INTERPROCEDURAL_OPTIMIZATION FALSE)
    endif()
  endif()
endif()

# 源文件
file(GLOB_RECURSE CORE_SRC CONFIGURE_DEPENDS
  src/core/*.cpp
)
file(GLOB_RECURSE SERVICES_SRC CONFIGURE_DEPENDS
  src/services/*.cpp
)
file(GLOB_RECURSE UI_SRC CONFIGURE_DEPENDS
  src/ui/*.cpp
)
file(GLOB_RECURSE UTILS_SRC CONFIGURE_DEPENDS
  src/utils/*.cpp
)
set(MAIN_SRC src/main.cpp)
set(SRC_FILES ${MAIN_SRC} ${CORE_SRC} ${SERVICES_SRC} ${UI_SRC} ${UTILS_SRC})

# 资源文件（Windows）
set(RESOURCE_FILES
  resources/app.rc
  resources/version.rc
)

# 头文件目录
include_directories(
  include
  resources
)

# WIN32 子系统，避免控制台窗口
add_executable(YGUninstaller WIN32 ${SRC_FILES} ${RESOURCE_FILES})

# 定义宏
target_compile_definitions(YGUninstaller PRIVATE
  UNICODE
  _UNICODE
  NOMINMAX
  WIN32_LEAN_AND_MEAN
)

target_include_directories(YGUninstaller PRIVATE resources)

# Windows 下链接的系统库
if(WIN32)
  target_link_libraries(YGUninstaller PRIVATE
    version
    comctl32
    comdlg32
    uxtheme
    shell32
    shlwapi
    user32
    gdi32
    advapi32
    ole32
    oleaut32
    uuid
    ws2_32
  )
endif()

# MinGW windres 资源编译器
if(MINGW)
  enable_language(RC)
  # 确保 windres 可用
  find_program(WINDRES windres)
  if(WINDRES)
    set(CMAKE_RC_COMPILER ${WINDRES})
  endif()
  # 使用 Unicode 入口（wWinMain）
  target_link_options(YGUninstaller PRIVATE -municode)
endif()

# 输出目录到 output/
# 允许通过预设将不同架构输出到不同目录
set(OUTPUT_SUBDIR "" CACHE STRING "Output subdirectory (e.g., x86 or x64)")
set(_OUT_DIR "${CMAKE_SOURCE_DIR}/output")
if(OUTPUT_SUBDIR)
  set(_OUT_DIR "${_OUT_DIR}/${OUTPUT_SUBDIR}")
endif()

set_target_properties(YGUninstaller PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${_OUT_DIR}"
  OUTPUT_NAME "YGUninstaller"
  ENABLE_EXPORTS OFF
)

# 可选：使用 upx 进一步压缩可执行文件
if(SIZE_OPTIMIZED)
  find_program(UPX_EXECUTABLE NAMES upx upx.exe)
  if(UPX_EXECUTABLE)
    add_custom_command(TARGET YGUninstaller POST_BUILD
      COMMAND "${UPX_EXECUTABLE}" --best --lzma "$<TARGET_FILE:YGUninstaller>"
      COMMENT "Compressing executable with UPX for smaller size" VERBATIM)
  endif()
endif()

# 构建后仅保留可执行文件：删除可能生成的导入库等副产物
add_custom_command(TARGET YGUninstaller POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E rm -f
          "${CMAKE_BINARY_DIR}/libYGUninstaller.dll.a"
          "${CMAKE_BINARY_DIR}/YGUninstaller.lib"
          "${CMAKE_BINARY_DIR}/YGUninstaller.exp"
  COMMENT "Removing import libraries to keep only the executable")

# 可选：最小化构建产物（清理编译中间文件），仅保留 output 下的 exe
add_custom_target(minimal_artifacts
  DEPENDS YGUninstaller
  COMMAND ${CMAKE_COMMAND} -E rm -rf
          "${CMAKE_BINARY_DIR}/CMakeFiles"
  COMMAND ${CMAKE_COMMAND} -E rm -f
          "${CMAKE_BINARY_DIR}/CMakeCache.txt"
          "${CMAKE_BINARY_DIR}/cmake_install.cmake"
          "${CMAKE_BINARY_DIR}/build.ninja"
          "${CMAKE_BINARY_DIR}/rules.ninja"
          "${CMAKE_BINARY_DIR}/cmake.verify_globs"
          "${CMAKE_BINARY_DIR}/CMakeConfigureLog.yaml"
  COMMENT "Cleaning build intermediates; executable remains in ${_OUT_DIR}")

# 安装（可选）
install(TARGETS YGUninstaller RUNTIME DESTINATION bin)


