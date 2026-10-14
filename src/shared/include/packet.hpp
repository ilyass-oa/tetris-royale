#pragma once

namespace tetris {
class Packet {
  public:
    virtual ~Packet() = default;
    void send(int socket) const;

  protected:
    virtual void serialize(int socket) const = 0;
};

// When a packet is added here, add logic for treating it in `packet_read.cpp`!
enum struct ClientboundPacket {
    Hello,
    LoginResponse,
    SignInResponse,
    GeneralResponse,
    Message,
    GridData,
    Malus,
    RoomInfos,
    GameStart,
    AddFriendResponse,
    RemoveFriendResponse,
    GameEnd,
    ScoreResponse,
    GetMyRequests,
    FriendsListResponse,
    ScoreListResponse,
    NextTile,
    ScorePacket

};
enum struct ServerboundPacket {
    Hello,
    Login,
    SignIn,
    GenO,
    Message,
    MessageRoom,
    GridData,
    MalusPacket,
    CreateRoom,
    JoinRoom,
    DelRoom,
    RoomInfos,
    SetStatus,
    SetReady,
    GameEnd,
    ScorePacket,
    ScoreRequest,
    SendRequest,
    GetMyRequests,
    HandleFriendRequest,
    FriendsListRequest,
    ScoreListRequest,
    Input
};
} // namespace tetris
