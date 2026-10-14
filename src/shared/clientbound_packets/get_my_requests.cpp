#include <cstdint>
#include <limits>
#include <nlohmann/json.hpp>

#include <clientbound_packets/get_my_requests.hpp>
#include <read.hpp>
#include <transport.hpp>
#include <vector>
#include <write.hpp>

using json = nlohmann::json; // Namespace JSON

namespace tetris::clientbound_packets {

std::shared_ptr<GetMyRequests> GetMyRequests::recv(int socket) {
    // Lire la taille des données
    uint32_t dataSize;
    read(socket, dataSize);
    if (dataSize > transport::kMaxStringBytes) {
        throw transport::ProtocolError("friend request payload exceeds the protocol limit");
    }

    // Lire le JSON
    std::vector<char> buffer(dataSize);
    if (!buffer.empty()) {
        transport::read_exact(socket, buffer.data(), buffer.size());
    }

    // Désérialiser
    json jsonData = json::parse(buffer);
    std::vector<std::string> requests =
        jsonData["reçoit demande de"].get<std::vector<std::string>>(); // Clé cohérente

    return std::make_shared<GetMyRequests>(requests);
}

void GetMyRequests::serialize(int socket) const {
    // Sérialiser en JSON
    json jsonData;
    jsonData["reçoit demande de"] = friendRequests; // Clé cohérente (pas "reçoit demande de")

    std::string serialized = jsonData.dump();
    if (serialized.size() > transport::kMaxStringBytes ||
        serialized.size() > std::numeric_limits<std::uint32_t>::max()) {
        throw transport::ProtocolError("friend request payload exceeds the protocol limit");
    }
    uint32_t size = serialized.size();

    write(socket, ID);
    write(socket, size);
    if (size != 0) {
        ensure_write(socket, serialized.data(), size);
    }
}

} // namespace tetris::clientbound_packets
