#ifndef WS_PROFILER_CONTAINERS_HPP
#define WS_PROFILER_CONTAINERS_HPP

#include <array>
#include <cstddef>
#include <exception>

#include "container_utils.hpp"

// non-resizable array that doesn't allocate memory after creation
template<typename T, size_t N>
class profiler_array
{
private:
    std::array<T, N> m_profiles{};
    size_t m_size = 0;

public:
    CONTAINER_TYPE_ALIASES(m_profiles);

    using iterator = typename decltype(m_profiles)::iterator;
    using const_iterator = typename decltype(m_profiles)::const_iterator;

    void push_back(const_reference block)
    {
        if (m_size >= N)
            throw std::exception{ "Profiler array overflow during profiler_array::push_back()" };

        m_profiles[m_size] = block;
        ++m_size;
    }

    void pop_back()
    {
        if (m_size == 0)
            throw std::exception{ "Profiler array underflow during profiler_array::pop_back()" };

        --m_size;
    }

    reference back()
    {
        return const_cast<reference>(static_cast<const profiler_array*>(this)->back());
    }

    const_reference back() const
    {
        if (m_size == 0)
            throw std::exception{ "Profiler array underflow during profiler_array::back()" };

        return m_profiles[m_size - 1];
    }

    reference front()
    {
        return const_cast<reference>(static_cast<const profiler_array*>(this)->front());
    }

    const_reference front() const
    {
        if (m_size == 0)
            throw std::exception{ "Profiler array underflow during profiler_array::front()" };

        return m_profiles[0];
    }

    void clear()
    {
        m_size = 0;
    }

    size_type size() const
    {
        return m_size;
    }

    static constexpr size_type max_size()
    {
        return N;
    }

    [[nodiscard]]
    bool empty() const
    {
        return m_size == 0;
    }

    reference operator[](size_type index)
    {
        return m_profiles[index];
    }

    const_reference operator[](size_type index) const
    {
        return m_profiles[index];
    }

    reference at(size_type index)
    {
        return const_cast<reference>(static_cast<const profiler_array*>(this)->at(index));
    }

    const_reference at(size_type index) const
    {
        if (index >= m_size)
            throw std::exception{ "Index out of range in profiler_array::at()" };

        return m_profiles.at(index);
    }

    T* data()
    {
        return m_profiles.data();
    }

    const T* data() const
    {
        return m_profiles.data();
    }

    iterator begin()
    {
        return m_profiles.begin();
    }

    const_iterator cbegin() const
    {
        return m_profiles.cbegin();
    }

    iterator end()
    {
        return m_profiles.begin() + m_size;
    }

    const_iterator cend() const
    {
        return m_profiles.cbegin() + m_size;
    }
};

// non-resizable stack that doesn't allocate memory after creation
template<typename T, size_t N>
class profiler_stack
{
private:
    profiler_array<T, N> m_profiles{};

public:
    CONTAINER_TYPE_ALIASES(m_profiles);
    FORWARD_ITERATOR_SUPPORT(m_profiles);

    void push(const_reference block)
    {
        m_profiles.push_back(block);
    }

    void pop()
    {
        m_profiles.pop_back();
    }

    reference top()
    {
        return m_profiles.back();
    }

    const_reference top() const
    {
        return m_profiles.back();
    }

    void clear()
    {
        m_profiles.clear();
    }

    size_type size() const
    {
        return m_profiles.size();
    }

    static constexpr size_type max_size()
    {
        return decltype(m_profiles)::max_size();
    }

    [[nodiscard]]
    bool empty() const
    {
        return m_profiles.empty();
    }
};

#endif
