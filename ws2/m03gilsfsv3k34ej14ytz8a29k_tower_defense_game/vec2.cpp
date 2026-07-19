#include "vec2.h"

namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game {

bool vec2_t::operator==(const vec2_t& other) const {
    return x == other.x && y == other.y;
}

vec2_t& vec2_t::operator+=(const vec2_t& other) {
    x += other.x;
    y += other.y;
    return *this;
}

vec2_t& vec2_t::operator-=(const vec2_t& other) {
    x -= other.x;
    y -= other.y;
    return *this;
}

vec2_t& vec2_t::operator*=(const vec2_t& other) {
    x *= other.x;
    y *= other.y;
    return *this;
}

vec2_t& vec2_t::operator/=(const vec2_t& other) {
    x /= other.x;
    y /= other.y;
    return *this;
}

vec2_t vec2_t::operator+(const vec2_t& other) const {
    vec2_t result = *this;
    result += other;
    return result;
}

vec2_t vec2_t::operator-(const vec2_t& other) const {
    vec2_t result = *this;
    result -= other;
    return result;
}

vec2_t vec2_t::operator*(const vec2_t& other) const {
    vec2_t result = *this;
    result *= other;
    return result;
}

vec2_t vec2_t::operator/(const vec2_t& other) const {
    vec2_t result = *this;
    result /= other;
    return result;
}

} // namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game
