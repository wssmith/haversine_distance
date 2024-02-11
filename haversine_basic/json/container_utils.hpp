#ifndef WS_CONTAINERUTILS_HPP
#define WS_CONTAINERUTILS_HPP

#define ITERATOR_SUPPORT(container) \
using iterator = decltype(container)::iterator; \
using const_iterator = decltype(container)::const_iterator; \
using reverse_iterator = decltype(container)::reverse_iterator; \
using const_reverse_iterator = decltype(container)::const_reverse_iterator; \
\
iterator begin() { return (container).begin(); } \
const_iterator begin() const { return (container).begin(); } \
iterator end() { return (container).end(); } \
const_iterator end() const { return (container).end(); } \
const_iterator cbegin() const { return (container).cbegin(); } \
const_iterator cend() const { return (container).cend(); } \
reverse_iterator rbegin() { return (container).rbegin(); } \
const_reverse_iterator rbegin() const { return (container).rbegin(); } \
reverse_iterator rend() { return (container).rend(); } \
const_reverse_iterator rend() const { return (container).rend(); } \
const_reverse_iterator crbegin() const { return (container).crbegin(); } \
const_reverse_iterator crend() const { return (container).crend(); }

#define CONTAINER_TYPE_ALIASES(container) \
using value_type = decltype(container)::value_type; \
using size_type = decltype(container)::size_type; \
using difference_type = decltype(container)::difference_type; \
using pointer = decltype(container)::pointer; \
using const_pointer = decltype(container)::const_pointer; \
using reference = decltype(container)::reference; \
using const_reference = decltype(container)::const_reference;

#endif
