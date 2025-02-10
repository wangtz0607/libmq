// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <functional>
#include <iterator>
#include <list>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "mq/utils/IndirectEqual.h"
#include "mq/utils/IndirectHash.h"

namespace mq {

template <typename K, typename V, typename Hash = std::hash<K>, typename Equal = std::equal_to<K>>
class LinkedHashMap {
    using List = std::list<std::pair<const K, V>>;

    using Map = std::unordered_map<const K *,
                                   typename List::iterator,
                                   IndirectHash<const K *, Hash>,
                                   IndirectEqual<const K *, Equal>>;

public:
    using key_type = K;
    using mapped_type = V;
    using value_type = std::pair<const K, V>;
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using reference = value_type &;
    using const_reference = const value_type &;
    using pointer = value_type *;
    using const_pointer = const value_type *;
    using iterator = typename List::iterator;
    using const_iterator = typename List::const_iterator;
    using reverse_iterator = typename List::reverse_iterator;
    using const_reverse_iterator = typename List::const_reverse_iterator;

    LinkedHashMap() = default;

    template <typename InputIter>
    LinkedHashMap(InputIter first, InputIter last) : LinkedHashMap() {
        insert(first, last);
    }

    LinkedHashMap(const LinkedHashMap &other) : LinkedHashMap() {
        insert(other.begin(), other.end());
    }

    LinkedHashMap(LinkedHashMap &&other) noexcept = default;

    LinkedHashMap &operator=(LinkedHashMap other) {
        swap(other);
        return *this;
    }

    iterator begin() {
        return list_.begin();
    }

    const_iterator begin() const {
        return list_.begin();
    }

    const_iterator cbegin() const {
        return list_.cbegin();
    }

    iterator end() {
        return list_.end();
    }

    const_iterator end() const {
        return list_.end();
    }

    const_iterator cend() const {
        return list_.cend();
    }

    reverse_iterator rbegin() {
        return list_.rbegin();
    }

    const_reverse_iterator rbegin() const {
        return list_.rbegin();
    }

    const_reverse_iterator crbegin() const {
        return list_.crbegin();
    }

    reverse_iterator rend() {
        return list_.rend();
    }

    const_reverse_iterator rend() const {
        return list_.rend();
    }

    const_reverse_iterator crend() const {
        return list_.crend();
    }

    reference front() {
        return list_.front();
    }

    const_reference front() const {
        return list_.front();
    }

    reference back() {
        return list_.back();
    }

    const_reference back() const {
        return list_.back();
    }

    bool empty() const {
        return list_.empty();
    }

    size_type size() const {
        return list_.size();
    }

    iterator find(const key_type &key) {
        if (auto j = map_.find(&key); j != map_.end()) {
            return j->second;
        }

        return list_.end();
    }

    const_iterator find(const key_type &key) const {
        if (auto j = map_.find(&key); j != map_.end()) {
            return j->second;
        }

        return list_.end();
    }

    std::pair<iterator, bool> insert(value_type value) {
        if (auto j = map_.find(&value.first); j != map_.end()) {
            return {j->second, false};
        }

        list_.push_back(std::move(value));
        auto i = std::prev(list_.end());
        map_.emplace(&i->first, i);
        return {i, true};
    }

    template <typename InputIter>
    void insert(InputIter first, InputIter last) {
        for (; first != last; ++first) {
            insert(*first);
        }
    }

    template <typename... Args>
    std::pair<iterator, bool> emplace(Args &&...args) {
        list_.emplace_back(std::forward<Args>(args)...);
        auto i = std::prev(list_.end());

        if (auto j = map_.find(&i->first); j != map_.end()) {
            list_.erase(i);
            return {j->second, false};
        }

        map_.emplace(&i->first, i);
        return {i, true};
    }

    iterator erase(const_iterator i) {
        map_.erase(map_.find(&i->first));
        return list_.erase(i);
    }

    iterator erase(const_iterator first, const_iterator last) {
        while (first != last) {
            first = erase(first);
        }

        return last;
    }

    size_type erase(const key_type &key) {
        if (auto j = map_.find(&key); j != map_.end()) {
            auto i = j->second;
            map_.erase(j);
            list_.erase(i);
            return 1;
        }

        return 0;
    }

    void clear() {
        list_.clear();
        map_.clear();
    }

    void swap(LinkedHashMap &other) noexcept {
        using std::swap;
        swap(list_, other.list_);
        swap(map_, other.map_);
    }

private:
    List list_;
    Map map_;
};

template <typename K, typename V, typename Hash, typename Equal>
    requires std::is_scalar_v<K>
class LinkedHashMap<K, V, Hash, Equal> {
    using List = std::list<std::pair<const K, V>>;
    using Map = std::unordered_map<K, typename List::iterator, Hash, Equal>;

public:
    using key_type = K;
    using mapped_type = V;
    using value_type = std::pair<const K, V>;
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using reference = value_type &;
    using const_reference = const value_type &;
    using pointer = value_type *;
    using const_pointer = const value_type *;
    using iterator = typename List::iterator;
    using const_iterator = typename List::const_iterator;
    using reverse_iterator = typename List::reverse_iterator;
    using const_reverse_iterator = typename List::const_reverse_iterator;

    LinkedHashMap() = default;

    template <typename InputIter>
    LinkedHashMap(InputIter first, InputIter last) : LinkedHashMap() {
        insert(first, last);
    }

    iterator begin() {
        return list_.begin();
    }

    const_iterator begin() const {
        return list_.begin();
    }

    const_iterator cbegin() const {
        return list_.cbegin();
    }

    iterator end() {
        return list_.end();
    }

    const_iterator end() const {
        return list_.end();
    }

    const_iterator cend() const {
        return list_.cend();
    }

    reverse_iterator rbegin() {
        return list_.rbegin();
    }

    const_reverse_iterator rbegin() const {
        return list_.rbegin();
    }

    const_reverse_iterator crbegin() const {
        return list_.crbegin();
    }

    reverse_iterator rend() {
        return list_.rend();
    }

    const_reverse_iterator rend() const {
        return list_.rend();
    }

    const_reverse_iterator crend() const {
        return list_.crend();
    }

    reference front() {
        return list_.front();
    }

    const_reference front() const {
        return list_.front();
    }

    reference back() {
        return list_.back();
    }

    const_reference back() const {
        return list_.back();
    }

    bool empty() const {
        return list_.empty();
    }

    size_type size() const {
        return list_.size();
    }

    iterator find(const key_type &key) {
        if (auto j = map_.find(key); j != map_.end()) {
            return j->second;
        }

        return list_.end();
    }

    const_iterator find(const key_type &key) const {
        if (auto j = map_.find(key); j != map_.end()) {
            return j->second;
        }

        return list_.end();
    }

    std::pair<iterator, bool> insert(value_type value) {
        if (auto j = map_.find(value.first); j != map_.end()) {
            return {j->second, false};
        }

        list_.push_back(std::move(value));
        auto i = std::prev(list_.end());
        map_.emplace(i->first, i);
        return {i, true};
    }

    template <typename InputIter>
    void insert(InputIter first, InputIter last) {
        for (; first != last; ++first) {
            insert(*first);
        }
    }

    template <typename... Args>
    std::pair<iterator, bool> emplace(Args &&...args) {
        list_.emplace_back(std::forward<Args>(args)...);
        auto i = std::prev(list_.end());

        if (auto j = map_.find(i->first); j != map_.end()) {
            list_.erase(i);
            return {j->second, false};
        }

        map_.emplace(i->first, i);
        return {i, true};
    }

    iterator erase(const_iterator i) {
        map_.erase(map_.find(i->first));
        return list_.erase(i);
    }

    iterator erase(const_iterator first, const_iterator last) {
        while (first != last) {
            first = erase(first);
        }

        return last;
    }

    size_type erase(const key_type &key) {
        if (auto j = map_.find(key); j != map_.end()) {
            auto i = j->second;
            map_.erase(j);
            list_.erase(i);
            return 1;
        }

        return 0;
    }

    void clear() {
        list_.clear();
        map_.clear();
    }

    void swap(LinkedHashMap &other) noexcept {
        using std::swap;
        swap(list_, other.list_);
        swap(map_, other.map_);
    }

private:
    List list_;
    Map map_;
};

template <typename K, typename V, typename Hash, typename Equal>
void swap(LinkedHashMap<K, V, Hash, Equal> &lhs, LinkedHashMap<K, V, Hash, Equal> &rhs) noexcept {
    lhs.swap(rhs);
}

} // namespace mq
