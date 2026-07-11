# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/kevin-james/Documents/GitHub/raylib-gamejam-game/build_desktop/_deps/flecs-src"
  "/home/kevin-james/Documents/GitHub/raylib-gamejam-game/build_desktop/_deps/flecs-build"
  "/home/kevin-james/Documents/GitHub/raylib-gamejam-game/build_desktop/_deps/flecs-subbuild/flecs-populate-prefix"
  "/home/kevin-james/Documents/GitHub/raylib-gamejam-game/build_desktop/_deps/flecs-subbuild/flecs-populate-prefix/tmp"
  "/home/kevin-james/Documents/GitHub/raylib-gamejam-game/build_desktop/_deps/flecs-subbuild/flecs-populate-prefix/src/flecs-populate-stamp"
  "/home/kevin-james/Documents/GitHub/raylib-gamejam-game/build_desktop/_deps/flecs-subbuild/flecs-populate-prefix/src"
  "/home/kevin-james/Documents/GitHub/raylib-gamejam-game/build_desktop/_deps/flecs-subbuild/flecs-populate-prefix/src/flecs-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/kevin-james/Documents/GitHub/raylib-gamejam-game/build_desktop/_deps/flecs-subbuild/flecs-populate-prefix/src/flecs-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/kevin-james/Documents/GitHub/raylib-gamejam-game/build_desktop/_deps/flecs-subbuild/flecs-populate-prefix/src/flecs-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
