#include <cstdint>
#include <limits>
#include <nlohmann/json.hpp>

#include <clientbound_packets/friendslist_response.hpp>
#include <read.hpp>
#include <transport.hpp>
#include <vector>
#include <write.hpp>

using json = nlohmann::json; // Namespace JSON

namespace tetris::clientbound_packets {

std::shared_ptr<FriendsListResponse> FriendsListResponse::recv(int socket) {
    // Lire la taille des données
    uint32_t dataSize;
    read(socket, dataSize);
    if (dataSize > transport::kMaxStringBytes) {
        throw transport::ProtocolError("friend list payload exceeds the protocol limit");
    }

    // Lire le JSON
    std::vector<char> buffer(dataSize);
    if (!buffer.empty()) {
        transport::read_exact(socket, buffer.data(), buffer.size());
    }

    // Désérialiser
    json jsonData = json::parse(buffer);
    std::vector<std::string> requests =
        jsonData["amis"].get<std::vector<std::string>>(); // Clé cohérente

    return std::make_shared<FriendsListResponse>(requests);
}

void FriendsListResponse::serialize(int socket) const {
    // Sérialiser en JSON
    json jsonData;
    jsonData["amis"] = friendsList; // Clé cohérente (pas "amis")

    std::string serialized = jsonData.dump();
    if (serialized.size() > transport::kMaxStringBytes ||
        serialized.size() > std::numeric_limits<std::uint32_t>::max()) {
        throw transport::ProtocolError("friend list payload exceeds the protocol limit");
    }
    uint32_t size = serialized.size();

    // Envoyer avec les primitives qui gèrent les écritures partielles.
    write(socket, ID);
    write(socket, size);
    if (size != 0) {
        ensure_write(socket, serialized.data(), size);
    }
}

} // namespace tetris::clientbound_packets
