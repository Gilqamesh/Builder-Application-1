#ifndef M03GINTXCZOHR63Y44O77B4PYJ_HYPERCUBE_MODULE_H
# define M03GINTXCZOHR63Y44O77B4PYJ_HYPERCUBE_MODULE_H

# include <array>

# include <m03gin6lte1az5kj36aj9suk6t_interval/api.h>
# include <m03ginwy24ng8o487c4beoms6l_vector/api.h>

namespace m03gintxczohr63y44o77b4pyj_hypercube {

template <typename T, std::size_t N>
class hypercube_t {
public:
    hypercube_t();
    hypercube_t(const std::array<m03gin6lte1az5kj36aj9suk6t_interval::interval_t<T>, N>& intervals);

    m03gin6lte1az5kj36aj9suk6t_interval::interval_t<T>& operator[](std::size_t index);
    const m03gin6lte1az5kj36aj9suk6t_interval::interval_t<T>& operator[](std::size_t index) const;

    hypercube_t intersect(const hypercube_t& other) const;

    T volume() const;

    bool is_empty() const;
    bool contains(const m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, N>& point) const;

private:
    std::array<m03gin6lte1az5kj36aj9suk6t_interval::interval_t<T>, N> m_intervals;
};

} // namespace m03gintxczohr63y44o77b4pyj_hypercube

namespace m03gintxczohr63y44o77b4pyj_hypercube {

template <typename T, std::size_t N>
hypercube_t<T, N>::hypercube_t() {
}

template <typename T, std::size_t N>
hypercube_t<T, N>::hypercube_t(const std::array<m03gin6lte1az5kj36aj9suk6t_interval::interval_t<T>, N>& intervals):
    m_intervals(intervals)
{
}

template <typename T, std::size_t N>
m03gin6lte1az5kj36aj9suk6t_interval::interval_t<T>& hypercube_t<T, N>::operator[](std::size_t index) {
    return m_intervals[index];
}

template <typename T, std::size_t N>
const m03gin6lte1az5kj36aj9suk6t_interval::interval_t<T>& hypercube_t<T, N>::operator[](std::size_t index) const {
    return m_intervals[index];
}

template <typename T, std::size_t N>
hypercube_t<T, N> hypercube_t<T, N>::intersect(const hypercube_t<T, N>& other) const {
    std::array<m03gin6lte1az5kj36aj9suk6t_interval::interval_t<T>, N> intersected_intervals;
    for (std::size_t i = 0; i < N; ++i) {
        intersected_intervals[i] = m_intervals[i].intersect(other.m_intervals[i]);
    }
    return hypercube_t<T, N>(intersected_intervals);
}

template <typename T, std::size_t N>
T hypercube_t<T, N>::volume() const {
    T vol = static_cast<T>(1);
    for (const auto& interval : m_intervals) {
        if (interval.is_empty()) {
            return static_cast<T>(0);
        }
        vol *= interval.size();
    }
    return vol;
}

template <typename T, std::size_t N>
bool hypercube_t<T, N>::is_empty() const {
    for (const auto& interval : m_intervals) {
        if (interval.is_empty()) {
            return true;
        }
    }
    return false;
}

template <typename T, std::size_t N>
bool hypercube_t<T, N>::contains(const m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, N>& point) const {
    for (std::size_t i = 0; i < N; ++i) {
        if (!m_intervals[i].contains(point[i])) {
            return false;
        }
    }
    return true;
}

} // namespace m03gintxczohr63y44o77b4pyj_hypercube

#endif // M03GINTXCZOHR63Y44O77B4PYJ_HYPERCUBE_MODULE_H
