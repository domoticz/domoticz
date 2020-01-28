option(USE_BUILTIN_LUA "Use builtin lua library" YES)
macro(liblua target)
if(USE_BUILTIN_LUA)
  message(STATUS "Use builtin lua library" )
  add_subdirectory (../lua build_lua)
  get_directory_property (LUA_LIBRARIES DIRECTORY ../lua DEFINITION LUA_LIBRARIES)
else()
  find_package(Lua REQUIRED)
endif()

  message(STATUS "LUA library found at: ${LUA_LIBRARIES}")
  target_link_libraries(${target} ${LUA_LIBRARIES})

  message(STATUS "LUA includes found at: ${LUA_INCLUDE_DIR}")
  target_include_directories(${target} PRIVATE ${LUA_INCLUDE_DIR})
endmacro()
