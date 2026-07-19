#ifndef M03GIN6LTE1AZ5KJ36AJ9SUK6T_INTERVAL_MODULE_H
# define M03GIN6LTE1AZ5KJ36AJ9SUK6T_INTERVAL_MODULE_H

# include <m03ginuqujr8cbfieco8r61u54_safe_arithmetic/api.h>

# include <algorithm>

namespace m03gin6lte1az5kj36aj9suk6t_interval {

/**
 * @brief Half-open interval [start, end) of type T.
 */
template <typename T>
class interval_t {
public:
    interval_t();
    interval_t(const T& start, const T& end);

    void start(const T& start);
    void end(const T& end);

    const T& start() const;
    const T& end() const;

    interval_t& operator+=(const T& value);
    interval_t& operator-=(const T& value);
    interval_t operator+(const T& value) const;
    interval_t operator-(const T& value) const;

    T clamp(const T& value) const;

    interval_t inflate(const T& value) const;
    interval_t deflate(const T& value) const;
    interval_t intersect(const interval_t& other) const;

    bool is_empty() const;
    bool contains(const T& value) const;

    T size() const;

private:
    T m_start;
    T m_end;
};

} // namespace m03gin6lte1az5kj36aj9suk6t_interval

namespace m03gin6lte1az5kj36aj9suk6t_interval {

template <typename T>
interval_t<T>::interval_t():
    m_start(std::numeric_limits<T>::max()),
    m_end(std::numeric_limits<T>::lowest())
{
}

template <typename T>
interval_t<T>::interval_t(const T& start, const T& end):
    m_start(start),
    m_end(end)
{
}

template <typename T>
void interval_t<T>::start(const T& start) {
    m_start = start;
}

template <typename T>
void interval_t<T>::end(const T& end) {
    m_end = end;
}

template <typename T>
const T& interval_t<T>::start() const {
    return m_start;
}

template <typename T>
const T& interval_t<T>::end() const {
    return m_end;
}

template <typename T>
interval_t<T>& interval_t<T>::operator+=(const T& value) {
    m_start = m03ginuqujr8cbfieco8r61u54_safe_arithmetic::add(m_start, value);
    m_end = m03ginuqujr8cbfieco8r61u54_safe_arithmetic::add(m_end, value);
    return *this;
}

template <typename T>
interval_t<T>& interval_t<T>::operator-=(const T& value) {
    m_start = m03ginuqujr8cbfieco8r61u54_safe_arithmetic::sub(m_start, value);
    m_end = m03ginuqujr8cbfieco8r61u54_safe_arithmetic::sub(m_end, value);
    return *this;
}

template <typename T>
interval_t<T> interval_t<T>::operator+(const T& value) const {
    interval_t<T> result = *this;
    result += value;
    return result;
}

template <typename T>
interval_t<T> interval_t<T>::operator-(const T& value) const {
    interval_t<T> result = *this;
    result -= value;
    return result;
}

template <typename T>
T interval_t<T>::clamp(const T& value) const {
    if (value < m_start) {
        return m_start;
    } else if (m_end < value) {
        return m_end;
    }
    return value;
}

template <typename T>
interval_t<T> interval_t<T>::inflate(const T& value) const {
    interval_t<T> result = *this;
    result.m_start = m03ginuqujr8cbfieco8r61u54_safe_arithmetic::sub(result.m_start, value);
    result.m_end = m03ginuqujr8cbfieco8r61u54_safe_arithmetic::add(result.m_end, value);
    return result;
}

template <typename T>
interval_t<T> interval_t<T>::deflate(const T& value) const {
    interval_t<T> result = *this;
    result.m_start = m03ginuqujr8cbfieco8r61u54_safe_arithmetic::add(result.m_start, value);
    result.m_end = m03ginuqujr8cbfieco8r61u54_safe_arithmetic::sub(result.m_end, value);
    return result;
}

template <typename T>
interval_t<T> interval_t<T>::intersect(const interval_t<T>& other) const {
    interval_t<T> result = *this;
    result.m_start = std::max(result.m_start, other.m_start);
    result.m_end = std::min(result.m_end, other.m_end);
    return result;
}

template <typename T>
bool interval_t<T>::is_empty() const {
    return m_end <= m_start;
}

template <typename T>
bool interval_t<T>::contains(const T& value) const {
    return m_start <= value && value < m_end;
}

template <typename T>
T interval_t<T>::size() const {
    return m03ginuqujr8cbfieco8r61u54_safe_arithmetic::sub(m_end, m_start);
}

} // namespace m03gin6lte1az5kj36aj9suk6t_interval

#endif // M03GIN6LTE1AZ5KJ36AJ9SUK6T_INTERVAL_MODULE_H
