option(USE_BUILTIN_CEREAL "Build with built-in cereal" YES)
macro(libcereal target)
  include(CheckIncludeFileCXX)

  if(USE_BUILTIN_CEREAL)
    find_path(cereal_INCLUDE_DIR cereal/cereal.hpp PATHS ${CMAKE_CURRENT_LIST_DIR}/..)
  else()
    find_path(cereal_INCLUDE_DIR cereal/cereal.hpp)
  endif()

  mark_as_advanced(cereal_INCLUDE_DIR)

  set(CMAKE_REQUIRED_INCLUDES "${cereal_INCLUDE_DIR}")
  check_include_file_cxx("cereal/cereal.hpp" cereal_FOUND)

  if(NOT cereal_FOUND)
    MESSAGE(FATAL_ERROR "cereal missing. install e.g. libcereal-dev")
  endif()

  target_include_directories(${target} PRIVATE SYSTEM ${cereal_INCLUDE_DIR})
endmacro()
