option(USE_STATIC_BOOST "Build with static BOOST libraries" YES)
macro(libboost target)
  find_package(Threads)

  #
  # Boost
  #
  set(Boost_USE_STATIC_LIBS ${USE_STATIC_BOOST})
  set(Boost_USE_MULTITHREADED ON)
  unset(Boost_INCLUDE_DIR CACHE)
  unset(Boost_LIBRARY_DIRS CACHE)


  # always off because of available boost setup on Travis
  set(Boost_USE_STATIC_RUNTIME OFF)

  if(USE_STATIC_BOOST)
    message(STATUS "Linking against boost static libraries")
  else()
    message(STATUS "Linking against boost dynamic libraries")
  endif()

  find_package(Boost ${DOMO_MIN_LIBBOOST_VERSION} REQUIRED COMPONENTS thread)
  MESSAGE(STATUS "BOOST libraries found at: ${Boost_LIBRARY_DIRS}")
  MESSAGE(STATUS "Boost includes found at: ${Boost_INCLUDE_DIR}")

  target_link_libraries(${target} Boost::thread ${Boost_LIBRARIES})
endmacro()

include_directories(${Boost_INCLUDE_DIRS})
