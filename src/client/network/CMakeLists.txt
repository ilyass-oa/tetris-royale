add_library(tetris_network STATIC connexion.cpp)
target_link_libraries(tetris_network PUBLIC shared Threads::Threads)
target_include_directories(tetris_network PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
