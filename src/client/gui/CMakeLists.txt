add_library(tetris_gui STATIC engine/SFMLEngine.cpp engine/MouseHandler.cpp engine/InputHandler.cpp
    game/GameInterface.cpp game/GameRenderer.cpp game/GameInputHandler.cpp
    game/modes/EndlessGUI.cpp game/modes/ClassicGUI.cpp game/modes/DuelGUI.cpp game/modes/RoyalCompetGUI.cpp
    menu/Room.cpp menu/Menu.cpp)
target_link_libraries(tetris_gui PUBLIC tetris_network SFML::Graphics SFML::Window SFML::System)
target_include_directories(tetris_gui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(tetris_gui PRIVATE ASSETS="${PROJECT_SOURCE_DIR}/assets" ASSETS_SRC="${PROJECT_SOURCE_DIR}/assets")
add_executable(gui main.cpp)
target_link_libraries(gui PRIVATE tetris_gui)
set_target_properties(gui PROPERTIES OUTPUT_NAME TetrisRoyale)
