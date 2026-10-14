#ifndef COORDINATE_HPP
#define COORDINATE_HPP
namespace tetris {
class Coordinate {
  private:
    int x;
    int y;

  public:
    Coordinate() : x(0), y(0) {}
    Coordinate(int x, int y) : x(x), y(y) {}
    Coordinate(Coordinate const &other) : x(other.x), y(other.y) {}
    Coordinate operator+(const Coordinate &other) const {
        return Coordinate(x + other.x, y + other.y);
    }
    Coordinate operator-(const Coordinate &other) const {
        return Coordinate(x - other.x, y - other.y);
    }
    bool operator==(const Coordinate &other) const {
        return x == other.x && y == other.y;
    }
    bool operator!=(const Coordinate &other) const {
        return !(*this == other);
    }
    int getX() const {
        return x;
    }
    int getY() const {
        return y;
    }
    void setX(int x) {
        this->x = x;
    }
    void setY(int y) {
        this->y = y;
    }
};
} // namespace tetris
#endif // COORDINATE_HPP
