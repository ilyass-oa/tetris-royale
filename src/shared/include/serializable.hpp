#pragma once

namespace tetris {
class Serializable {
  public:
    virtual void send(int socket) const = 0;
};
} // namespace tetris
