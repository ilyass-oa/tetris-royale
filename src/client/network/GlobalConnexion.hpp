#ifndef GLOBAL_CONNECTION_HPP
#define GLOBAL_CONNECTION_HPP
#include "connexion.hpp"
#include <cstdlib>
#include <string>

namespace tetris {
class GlobalConnection {
  public:
    // Explicit addresses avoid discovery delays and ambiguous LAN servers.
    static Client &getInstance(const std::string &defaultServerAddress = "127.0.0.1") {
        static Client instance([&] {
            if (const char *address = std::getenv("TETRIS_SERVER"); address && *address)
                return std::string(address);
            if (const char *discover = std::getenv("TETRIS_DISCOVER");
                discover && std::string(discover) == "1") {
                auto address = Client::decouvrirServeurLAN(46588, 1000);
                if (!address.empty())
                    return address;
            }
            return defaultServerAddress;
        }());
        return instance;
    }
};
} // namespace tetris

#endif // GLOBAL_CONNECTION_HPP
