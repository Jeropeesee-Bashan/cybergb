cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

project(visualizer VERSION 0.1 LANGUAGES C)

find_package(PkgConfig REQUIRED)

pkg_search_module(SDL3 REQUIRED sdl3)
pkg_search_module(GIO REQUIRED gio-unix-2.0 gio-windows-2.0)
pkg_search_module(POPT REQUIRED popt)

add_executable(visualizer WIN32 visualizer.c geometry.c io.c)

target_include_directories(visualizer
  PRIVATE ${SDL3_INCLUDE_DIRS}
          ${GIO_INCLUDE_DIRS}
          ${POPT_INCLUDE_DIRS})

target_link_libraries(visualizer
  PRIVATE ${SDL3_LINK_LIBRARIES}
          ${GIO_LINK_LIBRARIES}
          ${POPT_LINK_LIBRARIES})
