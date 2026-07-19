#include "rect.h"

namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game {

rect_t::rect_t() {
}

rect_t::rect_t(const vec2_t& top_left, const vec2_t& bottom_right):
    m_top_left(top_left),
    m_bottom_right(bottom_right)
{
}

void rect_t::top_left(const vec2_t& top_left) {
    m_top_left = top_left;
}

void rect_t::bottom_right(const vec2_t& bottom_right) {
    m_bottom_right = bottom_right;
}

vec2_t rect_t::top_left() const {
    return m_top_left;
}

vec2_t rect_t::bottom_right() const {
    return m_bottom_right;
}

vec2_t& rect_t::top_left() {
    return m_top_left;
}

vec2_t& rect_t::bottom_right() {
    return m_bottom_right;
}

vec2_t rect_t::size() const {
    return m_bottom_right - m_top_left;
}

bool rect_t::is_empty() const {
    if (m_bottom_right.x <= m_top_left.x || m_bottom_right.y <= m_top_left.y) {
        return true;
    }
    return false;
}

bool rect_t::is_point_inside(const vec2_t& point) const {
    return m_top_left.x <= point.x && point.x <= m_bottom_right.x &&
           m_top_left.y <= point.y && point.y <= m_bottom_right.y;
}

void rect_t::inflate(const vec2_t& delta) {
    m_top_left.x -= delta.x;
    m_top_left.y -= delta.y;
    m_bottom_right.x += delta.x;
    m_bottom_right.y += delta.y;
}

void rect_t::deflate(const vec2_t& delta) {
    m_top_left.x += delta.x;
    m_top_left.y += delta.y;
    m_bottom_right.x -= delta.x;
    m_bottom_right.y -= delta.y;

    // todo: temporary hack, these should be half-open intervals
    m_bottom_right.x = std::max(m_bottom_right.x, m_top_left.x);
    m_bottom_right.y = std::max(m_bottom_right.y, m_top_left.y);
}

rect_t rect_t::intersect(const rect_t& other) const {
    rect_t result;
    result.top_left({std::max(m_top_left.x, other.top_left().x),
                     std::max(m_top_left.y, other.top_left().y)});
    result.bottom_right({std::min(m_bottom_right.x, other.bottom_right().x),
                         std::min(m_bottom_right.y, other.bottom_right().y)});
    return result;
}

rect_t& rect_t::operator+=(const vec2_t& delta) {
    m_top_left += delta;
    m_bottom_right += delta;
    return *this;
}

rect_t& rect_t::operator-=(const vec2_t& delta) {
    m_top_left -= delta;
    m_bottom_right -= delta;
    return *this;
}

rect_t rect_t::operator+(const vec2_t& delta) const {
    rect_t result = *this;
    result += delta;
    return result;
}

rect_t rect_t::operator-(const vec2_t& delta) const {
    rect_t result = *this;
    result -= delta;
    return result;
}

} // namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game
