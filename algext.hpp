//  Algorithm library extentions.
//
//  Copyright (C) 2025, Martin Young <martin_young@live.cn>
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program. If not, see <https://www.gnu.org/licenses/>.
//------------------------------------------------------------------------

// inContainer      - to judge if an element in a container
// subContainer     - to subtract the elements(or its keys) from one container contained in another (Difference or Except)
// uniContainer     - to add  all elements in   one container into another (union)
// mvtoContainer    - to move all elements from one container into another

#pragma once

#include <vector>
#include <list>
#include <forward_list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <type_traits>
#include <optional>
#include <algorithm>
#include <cmath>
#include <concepts>
#include <ranges>
#include <utility>

// ==================== Concepts ====================

#ifndef QQ_CONCEPT_Integer
#define QQ_CONCEPT_Integer
template <typename T> concept Integer = std::is_integral_v<T>;
#endif

#ifndef QQ_CONCEPT_Containers
#define QQ_CONCEPT_Containers

template <typename Container>
concept SequentialContainer
    = std::is_same_v<std::decay_t<Container>, std::vector<typename Container::value_type>> ||
      std::is_same_v<std::decay_t<Container>, std::list<typename Container::value_type>> ||
      std::is_same_v<std::decay_t<Container>, std::forward_list<typename Container::value_type>>;

template <typename Container>
concept OrderedAssociativeContainer
    = std::is_same_v<std::decay_t<Container>, std::map<typename Container::key_type, typename Container::mapped_type>> ||
      std::is_same_v<std::decay_t<Container>, std::multimap<typename Container::key_type, typename Container::mapped_type>> ||
      std::is_same_v<std::decay_t<Container>, std::set<typename Container::value_type>> ||
      std::is_same_v<std::decay_t<Container>, std::multiset<typename Container::value_type>>;

template <typename Container>
concept UnorderedAssociativeContainer
    = std::is_same_v<std::decay_t<Container>, std::unordered_map<typename Container::key_type, typename Container::mapped_type>> ||
      std::is_same_v<std::decay_t<Container>, std::unordered_multimap<typename Container::key_type, typename Container::mapped_type>> ||
      std::is_same_v<std::decay_t<Container>, std::unordered_set<typename Container::value_type>> ||
      std::is_same_v<std::decay_t<Container>, std::unordered_multiset<typename Container::value_type>>;

template <typename Container>
concept AssociativeContainer = OrderedAssociativeContainer<Container> || UnorderedAssociativeContainer<Container>;

template <typename Container>
concept ForwardListContainer = std::is_same_v<std::decay_t<Container>, std::forward_list<typename Container::value_type>>;

// ==================== Type Traits (for key extraction) ====================

template <typename Container>
struct container_key { using type = typename Container::value_type; };

// Specializations for key-value containers
template <typename Key, typename T, typename... Args>
struct container_key<std::map<Key, T, Args...>> { using type = Key; };
template <typename Key, typename T, typename... Args>
struct container_key<std::multimap<Key, T, Args...>> { using type = Key; };
template <typename Key, typename T, typename... Args>
struct container_key<std::unordered_map<Key, T, Args...>> { using type = Key; };
template <typename Key, typename T, typename... Args>
struct container_key<std::unordered_multimap<Key, T, Args...>> { using type = Key; };

// Specializations for set containers
template <typename Key, typename... Args>
struct container_key<std::set<Key, Args...>> { using type = Key; };
template <typename Key, typename... Args>
struct container_key<std::multiset<Key, Args...>> { using type = Key; };
template <typename Key, typename... Args>
struct container_key<std::unordered_set<Key, Args...>> { using type = Key; };
template <typename Key, typename... Args>
struct container_key<std::unordered_multiset<Key, Args...>> { using type = Key; };

template <typename Container>
using container_key_t = typename container_key<Container>::type;

// Key extraction ---------------------------------------
template <typename Container>
struct key_extractor {
    static const auto& get(const typename Container::value_type& elem) { return elem; }
};

// Specializations for map-like containers
template <typename Key, typename T, typename... Args>
struct key_extractor<std::map<Key, T, Args...>> {
    static const Key& get(const typename std::map<Key, T, Args...>::value_type& elem) { return elem.first; }
};
template <typename Key, typename T, typename... Args>
struct key_extractor<std::multimap<Key, T, Args...>> {
    static const Key& get(const typename std::multimap<Key, T, Args...>::value_type& elem) { return elem.first; }
};
template <typename Key, typename T, typename... Args>
struct key_extractor<std::unordered_map<Key, T, Args...>> {
    static const Key& get(const typename std::unordered_map<Key, T, Args...>::value_type& elem) { return elem.first; }
};
template <typename Key, typename T, typename... Args>
struct key_extractor<std::unordered_multimap<Key, T, Args...>> {
    static const Key& get(const typename std::unordered_multimap<Key, T, Args...>::value_type& elem) { return elem.first; }
};

// Concept to check if a type has a nested ::value_type
template <typename C>
concept HasValueType = requires { typename C::value_type; };

// Concept to check if a container is clearable (has a clear() method)
template <typename C>
concept Clearable = requires(C& c) { c.clear(); };

// Concept to check if ContainerA can have elements inserted/pushed_back (movably)
template <typename C, typename ElementType = typename C::value_type>
concept MovableElementSink =
    requires(C& c, ElementType&& e) { c.push_back(std::move(e)); } ||
    requires(C& c, ElementType&& e) { c.insert(std::move(e)); };

// Concept to check if ContainerA has a reserve() method and a size() method
template <typename C>
concept Reservable = requires(C& c, typename C::size_type n) {
    { c.reserve(n) };
    { c.size() } -> std::convertible_to<typename C::size_type>;
};

// Concept to check if the container can have elements inserted/pushed_back by copying
template <typename C, typename ElementType = typename C::value_type>
concept CopyableElementSink =
    requires(C& c, const ElementType& e) { c.push_back(e); } || // For vector-like containers, taking const ref
    requires(C& c, const ElementType& e) { c.insert(e); };   // For set-like containers, taking const ref

// Concept to check if ContainerB supports node extraction (C++17 feature)
template<typename ContainerB_t>
concept ExtractSupportingContainer =
    requires(ContainerB_t& cb) {
        typename ContainerB_t::node_type; // Has a node_type
        { cb.extract(cb.begin()) } -> std::same_as<typename ContainerB_t::node_type>; // extract(iterator) returns node_type
    } &&
    requires(typename ContainerB_t::node_type& nh) { // node_type properties
        { std::move(nh.value()) }; // Can get a (movable) value
        { !nh.empty() } -> std::convertible_to<bool>; // Can check if empty
    };

// ==================== Cost Calculation ====================
template <typename Container>
constexpr double lookup_cost_per_element(size_t container_size) {
    if constexpr (SequentialContainer<Container>) {
        return container_size; // O(n) linear search
    } else if constexpr (OrderedAssociativeContainer<Container>) {
        return std::log2(container_size + 1); // O(log n)
    } else if constexpr (UnorderedAssociativeContainer<Container>) {
        return 1.0; // O(1) hash lookup
    } else {
        return container_size; // Default to linear for unknown types
    }
}

#endif // QQ_CONCEPT_Containers

//------------------------------------------------------------------------------------------------
// Since C++ 20, most container types have contains(), and all container types have find().
template <typename Container, typename Element>
inline bool inContainer(const Container& c, const Element& e) {
    if constexpr (requires { c.contains(e); }) return c.contains(e);
    else  return std::find(c.begin(), c.end(), e) != c.end();
}

//------------------------------------------------------------------------------------------------
// subContainer - to subtract the elements(or its keys) from A contained in B (Difference or Except)
//
// Supported container types:
//      vector, list, forward_list,map, multimap, set, multiset,unordered_map, unordered_multimap, unordered_set, unordered_multiset
//
template <typename ContainerA, typename ContainerB>
ContainerA& subContainer(ContainerA& a, const ContainerB& b) {
    using KeyType = container_key_t<ContainerA>;
    static_assert(std::is_same_v<KeyType, container_key_t<ContainerB>>,
                  "Container key types must be compatible");

    const size_t a_size = std::distance(a.begin(), a.end());
    const size_t b_size = std::distance(b.begin(), b.end());

    // Calculate potential strategy costs
    enum Strategy { ScanA, HashB, ScanB };
    std::optional<double> costs[3];

    // Common cost for ScanA and ScanB
    const double scan_a_cost = a_size * lookup_cost_per_element<ContainerB>(b_size);
    const double scan_b_cost = b_size * lookup_cost_per_element<ContainerA>(a_size);

    costs[ScanA] = scan_a_cost;
    costs[ScanB] = scan_b_cost;

    // HashB strategy only valid for sequential containers
    if constexpr (SequentialContainer<ContainerB>) {
        costs[HashB] = b_size + a_size; // O(B) build + O(A) scan
    }

    // Find minimal cost strategy
    Strategy best_strategy = ScanA;
    double min_cost = scan_a_cost;

    for (int i = 1; i < 3; ++i)
        if (costs[i].has_value() && costs[i] < min_cost) {
            min_cost = *costs[i];
            best_strategy = static_cast<Strategy>(i);
        }

    // Execute selected strategy
    switch (best_strategy) {
    case ScanA: {
        // Scan A and remove elements existing in B
        auto predicate = [&](const auto& elem) {
            const KeyType& key = key_extractor<ContainerA>::get(elem);
            if constexpr (AssociativeContainer<ContainerB>)
                return b.find(key) != b.end();
            else
                return std::any_of(b.begin(), b.end(),
                    [&](const auto& belem) { return key_extractor<ContainerB>::get(belem) == key; });
        };

        if constexpr (SequentialContainer<ContainerA>)
            if constexpr (ForwardListContainer<ContainerA>) {
                auto prev = a.before_begin();
                auto curr = a.begin();
                while (curr != a.end())
                    if (predicate(*curr))  curr = a.erase_after(prev);
                    else prev = curr++;
            } else
                a.erase(std::remove_if(a.begin(), a.end(), predicate), a.end());
        else
            for (auto it = a.begin(); it != a.end(); )
                if (predicate(*it)) it = a.erase(it); else ++it;
        break; }
    case HashB: {
        // Build hash set from B
        std::unordered_set<KeyType> b_keys;
        for (const auto& elem : b)
            b_keys.insert(key_extractor<ContainerB>::get(elem));

        // Remove elements from A using hash set
        auto predicate = [&](const auto& elem) {
            return b_keys.contains(key_extractor<ContainerA>::get(elem));
        };

        if constexpr (SequentialContainer<ContainerA>)
            if constexpr (ForwardListContainer<ContainerA>) {
                auto prev = a.before_begin();
                auto curr = a.begin();
                while (curr != a.end())
                    if (predicate(*curr)) curr = a.erase_after(prev);
                    else { prev = curr; ++curr; }
            } else
                a.erase(std::remove_if(a.begin(), a.end(), predicate), a.end());
        else
            for (auto it = a.begin(); it != a.end(); )
                if (predicate(*it)) it = a.erase(it); else ++it;
        break; }
    case ScanB: {
        // Scan B and erase from A
        for (const auto& elem : b) {
            const KeyType& key = key_extractor<ContainerB>::get(elem);
            if constexpr (AssociativeContainer<ContainerA>)
                a.erase(key);
            else
                if constexpr (ForwardListContainer<ContainerA>) {
                    auto prev = a.before_begin();
                    auto curr = a.begin();
                    while (curr != a.end())
                        if (key_extractor<ContainerA>::get(*curr) == key) curr = a.erase_after(prev);
                        else prev = curr++;
                } else
                    a.erase(std::remove_if(a.begin(), a.end(),
                                           [&](const auto& e) { return key_extractor<ContainerA>::get(e) == key; }),
                                           a.end());
        }
        break; }
    }

    return a;
}

//------------------------------------------------------------------------------------------------
// mvtoContainer - to move all elements from container B into A
//
// Supported container types:
//      std::vector，std::set，std::multiset，std::unordered_set，std::unordered_multiset，std::map，std::multimap
//
template <typename ContainerA,typename ContainerB>
requires HasValueType<ContainerA> &&
         HasValueType<ContainerB> &&
         std::same_as<typename ContainerA::value_type, typename ContainerB::value_type> &&
         std::is_move_constructible_v<typename ContainerA::value_type> &&
         MovableElementSink<ContainerA, typename ContainerA::value_type> &&
         std::ranges::input_range<ContainerB> &&
         Clearable<ContainerB>
ContainerA& mvtoContainer(ContainerA& a, ContainerB& b)
{
    if constexpr (std::is_same_v<ContainerA, ContainerB>)
        if (a.empty()) { a = std::move(b); b.clear(); return a; }

    if constexpr (Reservable<ContainerA> && std::ranges::sized_range<ContainerB>)
        if (!std::ranges::empty(b)) a.reserve(a.size() + std::ranges::size(b));

    if constexpr (ExtractSupportingContainer<ContainerB>) {
        // ContainerB is a node-based container (e.g., std::set, std::map, etc.)
        while (!b.empty()) {
            typename ContainerB::node_type node_handle = b.extract(b.begin()); // Removes from b, gives ownership
            if (!node_handle.empty()) {
                // Try to insert the node_handle directly if A is a compatible node-based container.
                // This is the most efficient path for map-to-map, set-to-set, etc.
                if constexpr (requires(ContainerA& ca, typename ContainerB::node_type&& nh_from_b) { ca.insert(std::move(nh_from_b)); })
                    a.insert(std::move(node_handle));
                // Else, extract the value from the node and insert/push_back that.
                else if constexpr (requires(ContainerA& ca, typename ContainerA::value_type&& val) { ca.push_back(std::move(val)); })
                    a.push_back(std::move(node_handle.value()));
                else if constexpr (requires(ContainerA& ca, typename ContainerA::value_type&& val) { ca.insert(std::move(val)); })
                    a.insert(std::move(node_handle.value()));
                // One of the above insert/push_back must be true due to the MovableElementSink constraint on ContainerA for its value_type
            }
        }
    } else {
        // ContainerB is likely a sequence container (e.g., std::vector) or does not support extract.
        // Elements are moved one by one.
        if constexpr (requires(ContainerA& ca, typename ContainerA::value_type&& val) { ca.push_back(std::move(val)); }) {
            for (auto&& elem : b) a.push_back(std::move(elem));
        } else if constexpr (requires(ContainerA& ca, typename ContainerA::value_type&& val) { ca.insert(std::move(val)); }) {
            for (auto&& elem : b) a.insert(std::move(elem));
        }
    }

    b.clear();
    return a;
}

//------------------------------------------------------------------------------------------------
// uniContainer - to add all elements in container B into A
//
// Supported container types:
//      std::vector，std::set，std::multiset，std::unordered_set，std::unordered_multiset，std::map，std::multimap
//
template <typename ContainerA, typename ContainerB>
requires HasValueType<ContainerA> &&
         HasValueType<ContainerB> &&
         std::same_as<typename ContainerA::value_type, typename ContainerB::value_type> && // Elements must be of the same type
         std::is_copy_constructible_v<typename ContainerA::value_type> && // Elements in 'a' must be copy-constructible
         CopyableElementSink<ContainerA, typename ContainerA::value_type> &&  // 'a' must be able to accept copied elements
         std::ranges::input_range<const ContainerB> // 'b' must be iterable when const (as it's passed as const ContainerB&)
ContainerA& uniContainer(ContainerA& a, const ContainerB& b)
{
    if constexpr (Reservable<ContainerA> && std::ranges::sized_range<const ContainerB>)
        if (!std::ranges::empty(b))  // std::ranges::empty works with const ContainerB
            a.reserve(a.size() + std::ranges::size(b)); // std::ranges::size works with const ContainerB

    // Perform element-wise copy from ContainerB to ContainerA.
    // The CopyableElementSink concept ensures ContainerA supports one of these operations for const value_type& elements.
    if constexpr (requires(ContainerA& ca, const typename ContainerA::value_type& val) { ca.push_back(val); })
        // Use push_back if available (e.g., for std::vector).
        for (const auto& elem : b)  a.push_back(elem); // This will copy the element.

    else if constexpr (requires(ContainerA& ca, const typename ContainerA::value_type& val) { ca.insert(val); })
        // Use insert if available (e.g., for std::set, std::multiset, etc.).
        // For unique-key sets, insert will correctly ignore duplicates from 'b' (standard set behavior).
        for (const auto& elem : b)  a.insert(elem); // This will copy the element.

    return a;
}
//------------------------------------------------------------------------------------------------
