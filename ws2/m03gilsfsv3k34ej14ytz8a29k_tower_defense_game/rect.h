#ifndef M03GILSFSV3K34EJ14YTZ8A29K_RECT_H
# define M03GILSFSV3K34EJ14YTZ8A29K_RECT_H

# include "vec2.h"

namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game {

class rect_t {
public:
    rect_t();
    rect_t(const vec2_t& top_left, const vec2_t& bottom_right);

    void top_left(const vec2_t& top_left);
    void bottom_right(const vec2_t& bottom_right);

    vec2_t top_left() const;
    vec2_t bottom_right() const;

    vec2_t& top_left();
    vec2_t& bottom_right();

    vec2_t size() const;
    bool is_empty() const;

    bool is_point_inside(const vec2_t& point) const;

    // inflate the rectangle by the given delta in all sides from its center
    void inflate(const vec2_t& delta);

    // deflate the rectangle by the given delta in all sides from its center
    void deflate(const vec2_t& delta);

    rect_t intersect(const rect_t& other) const;

    rect_t& operator+=(const vec2_t& delta);
    rect_t& operator-=(const vec2_t& delta);
    rect_t operator+(const vec2_t& delta) const;
    rect_t operator-(const vec2_t& delta) const;

private:
    vec2_t m_top_left;
    vec2_t m_bottom_right;
};

} // namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game

#endif // M03GILSFSV3K34EJ14YTZ8A29K_RECT_H
