add_library(shared STATIC
./serverbound_packets/hello.cpp
./serverbound_packets/login.cpp
./serverbound_packets/signin.cpp
./serverbound_packets/general_one.cpp
./serverbound_packets/grid_data.cpp
./serverbound_packets/message.cpp
./serverbound_packets/message_room.cpp
./serverbound_packets/malus_data.cpp
./serverbound_packets/create_room.cpp
./serverbound_packets/join_room.cpp
./serverbound_packets/delete_room.cpp
./serverbound_packets/room_infos.cpp
./serverbound_packets/owner_start.cpp
./serverbound_packets/set_status.cpp
./serverbound_packets/end_game.cpp
./serverbound_packets/score_packet.cpp
./serverbound_packets/score_request.cpp
./serverbound_packets/send_request.cpp
./serverbound_packets/get_my_requests.cpp
./serverbound_packets/handle_friend_request.cpp
./serverbound_packets/friendslist_request.cpp
./serverbound_packets/scorelist_request.cpp
./serverbound_packets/input.cpp


./clientbound_packets/hello.cpp
./clientbound_packets/login_response.cpp
./clientbound_packets/signin_response.cpp
./clientbound_packets/general_response.cpp
./clientbound_packets/grid_data.cpp
./clientbound_packets/message.cpp
./clientbound_packets/malus_data.cpp
./clientbound_packets/room_infos.cpp
./clientbound_packets/game_start.cpp
./clientbound_packets/end_game.cpp
./clientbound_packets/score_response.cpp
./clientbound_packets/get_my_requests.cpp
./clientbound_packets/friendslist_response.cpp
./clientbound_packets/scorelist_response.cpp
./clientbound_packets/next_tile.cpp
./clientbound_packets/score_packet.cpp

./game/Coordinate.cpp
./game/Grid.cpp
./game/Piece.cpp
./game/PieceFactory.cpp
./game/TetrisGame.cpp

./write.cpp
./packet_read.cpp
)

target_include_directories(shared PUBLIC ./include)
target_link_libraries(shared PUBLIC nlohmann_json::nlohmann_json Threads::Threads)

