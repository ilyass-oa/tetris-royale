add_library(tetris_server_core STATIC
    gestion_serveur/serveur.cpp gestion_serveur/player.cpp
    gestion_serveur/room.cpp gestion_serveur/player_manager.cpp gestion_serveur/room_manager.cpp
    game/commun_class.cpp game/endless.cpp game/classique.cpp game/duel.cpp game/br.cpp
    database/database_manager.cpp)
target_link_libraries(tetris_server_core PUBLIC shared OpenSSL::Crypto Threads::Threads)
target_include_directories(tetris_server_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
add_executable(server main.cpp)
target_link_libraries(server PRIVATE tetris_server_core)
