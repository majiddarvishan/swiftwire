add_library(swiftwire
  ${CMAKE_CURRENT_LIST_DIR}/../include/swiftwire/protocol.hpp
  client.cpp
  server.cpp
)

target_include_directories(swiftwire
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(swiftwire
  PUBLIC Boost::system Threads::Threads
)

target_compile_definitions(swiftwire PRIVATE BOOST_ASIO_NO_DEPRECATED)
