#ifndef M03GILSFSV3K34EJ14YTZ8A29K_VEC2_H
# define M03GILSFSV3K34EJ14YTZ8A29K_VEC2_H

# include <functional>

namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game {

struct vec2_t {
    int x;
    int y;

    bool operator==(const vec2_t& other) const;

    vec2_t& operator+=(const vec2_t& other);
    vec2_t& operator-=(const vec2_t& other);
    vec2_t& operator*=(const vec2_t& other);
    vec2_t& operator/=(const vec2_t& other);

    template <typename T>
    vec2_t& operator*=(T value);
    template <typename T>
    vec2_t& operator/=(T value);

    vec2_t operator+(const vec2_t& other) const;
    vec2_t operator-(const vec2_t& other) const;
    vec2_t operator*(const vec2_t& other) const;
    vec2_t operator/(const vec2_t& other) const;
    
    template <typename T>
    vec2_t operator*(T value) const;
    template <typename T>
    vec2_t operator/(T value) const;
};

} // namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game

namespace std {

template <>
struct hash<m03gilsfsv3k34ej14ytz8a29k_tower_defense_game::vec2_t>;

} // namespace std

namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game {

template <typename T>
vec2_t& vec2_t::operator*=(T value) {
    x *= value;
    y *= value;
    return *this;
}

template <typename T>
vec2_t& vec2_t::operator/=(T value) {
    x /= value;
    y /= value;
    return *this;
}

template <typename T>
vec2_t vec2_t::operator*(T value) const {
    vec2_t result = *this;
    result *= value;
    return result;
}

template <typename T>
vec2_t vec2_t::operator/(T value) const {
    vec2_t result = *this;
    result /= value;
    return result;
}

} // namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game

namespace std {

template <>
struct hash<m03gilsfsv3k34ej14ytz8a29k_tower_defense_game::vec2_t> {
    size_t operator()(const m03gilsfsv3k34ej14ytz8a29k_tower_defense_game::vec2_t& v) const noexcept {
        return hash<int>()(v.x) ^ (hash<int>()(v.y) << 1);
    }
};

} // namespace std

#endif // M03GILSFSV3K34EJ14YTZ8A29K_VEC2_H
