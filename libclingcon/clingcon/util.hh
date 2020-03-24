// {{{ MIT License
//
// Copyright 2020 Roland Kaminski
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//
// }}}

#ifndef CLINGCON_UTIL_H
#define CLINGCON_UTIL_H

#include <chrono>
#include <vector>
#include <map>

namespace Clingcon {

//! Calculate the midpoint of two values of integral type.
template<typename I>
I midpoint(I a, I b) noexcept {
    using U = std::make_unsigned_t<I>;
    return static_cast<I>(static_cast<U>(a) + (static_cast<U>(b) - static_cast<U>(a)) / 2);
}

//! Simple timer that adds the elapsed time to the given `double` upon
//! destruction.
class Timer {
    using Clock = std::chrono::high_resolution_clock;
    using Time = std::chrono::time_point<Clock>;
    using Duration = std::chrono::duration<double>;
public:
    //! Start the timer given a target where to store the elapsed time.
    Timer(double &target)
    : target_{target}
    , start_{Clock::now()} { }

    //! Stop the timer storing the result.
    ~Timer() {
        target_ += Duration{Clock::now() - start_}.count();
    }

    Timer(Timer const &) = delete;
    Timer(Timer &&) = delete;
    Timer &operator=(Timer const &) = delete;
    Timer &operator=(Timer &&) = delete;

private:
    double &target_;
    Time start_;
};

//! Helper to mark/detect if an element is contained in a `UniqueVector`.
template <class T>
struct FlagUnique {
    [[nodiscard]] bool get_flag_unique(T const *x) const {
        return x->flag_unique;
    }
    void set_flag_unique(T *x, bool flag) const {
        x->flag_unique = flag;
    }
};

//! Like a vector but only adds an element if it is not contained yet.
template <typename T, class Flagger=FlagUnique<T>>
class UniqueVector : private Flagger {
public:
    using Vector = typename std::vector<T*>;
    using Iterator = typename Vector::iterator;
    using ConstIterator = typename Vector::const_iterator;

    //! Create an empty vector.
    UniqueVector(const Flagger &m = Flagger())
    : Flagger(m) {
    }

    //! Destroy the vector unmarking all contained elements.
    ~UniqueVector() {
        clear();
    }

    UniqueVector(UniqueVector const &) = delete;
    UniqueVector(UniqueVector &&) noexcept = default;
    UniqueVector &operator=(UniqueVector const &) = delete;
    UniqueVector &operator=(UniqueVector &&) noexcept = default;

    //! Check if the vector is empty.
    [[nodiscard]] bool empty() const {
        return vec_.empty();
    }

    //! Get the size of the vector.
    [[nodiscard]] size_t size() const {
        return vec_.size();
    }

    //! Get the element at the given position.
    [[nodiscard]] T *operator[](size_t i) const {
        return vec_[i];
    }

    //! Check if the vector contains an element.
    [[nodiscard]] bool contains(T const *x) const {
        return Flagger::get_flag_unique(x);
    }

    //! Check if the vector contains an element.
    [[nodiscard]] bool contains(T const &x) const {
        return contains(&x);
    }

    //! Iterator to the beginning of the vector.
    [[nodiscard]] ConstIterator begin() const {
        return vec_.begin();
    }

    //! Iterator to the end of the vector.
    [[nodiscard]] ConstIterator end() const {
        return vec_.end();
    }

    //! Iterator to the beginning of the vector.
    [[nodiscard]] Iterator begin() {
        return vec_.begin();
    }

    //! Iterator to the end of the vector.
    [[nodiscard]] Iterator end() {
        return vec_.end();
    }

    //! Remove an element from the vector.
    void erase(ConstIterator it) {
        Flagger::set_flag_unique(*it, false);
        vec_.erase(it);
    }

    //! Clear the vector.
    void clear() {
        for (auto &x : vec_) {
            Flagger::set_flag_unique(x, false);
        }
        vec_.clear();
    }

    //! Add elements from a sequence to the vector.
    template <typename It>
    size_t extend(It begin, It end) {
        size_t n = 0;
        for (auto it = begin; it != end; ++it) {
            if (append(*it)) {
                ++n;
            }
        }
        return n;
    }

    //! Append an element to the vector.
    bool append(T *x) {
        if (Flagger::get_flag_unique(x)) {
            return false;
        }
        vec_.emplace_back(x);
        Flagger::set_flag_unique(x, true);
        return true;
    }

    //! Append an element to the vector.
    bool append(T &x) {
        return append(&x);
    }

private:
    Vector vec_;
};

//! Simplistic interval set class restricted to methods needed to implement
//! `&dom` statements.
template <typename T>
class IntervalSet {
public:
    using Map = typename std::map<T, T>;
    using Iterator = typename Map::const_iterator;

    //! Create an empty interval set.
    IntervalSet() = default;

    IntervalSet(const IntervalSet &) = default;
    IntervalSet(IntervalSet &&) noexcept = default;
    IntervalSet& operator=(const IntervalSet &) = default;
    IntervalSet& operator=(IntervalSet &&) noexcept = default;
    ~IntervalSet() = default;

    //! Iterator to the beginning of the set.
    [[nodiscard]] Iterator begin() const {
        return map_.begin();
    }

    //! Iterator to the end of the set.
    [[nodiscard]] Iterator end() const {
        return map_.end();
    }

    //! Check if the set is empty.
    [[nodiscard]] bool empty() const {
        return map_.empty();
    }

    //! Check if the set contains in interval.
    [[nodiscard]] bool contains(T const &a, T const &b) const {
        //           v
        //   |-------|
        // |---|  |---|  |---|
        //               ^
        auto it = map_.upper_bound(a);
        if (it == map_.begin()) {
            return false;
        }
        --it;
        //   v       v
        //   |-------|
        // |-----------|
        // ^           ^
        return !(a < it->first) && !(it->second < b);
    }

    //! Add an interval to the set.
    void add(T a, T b) {
        if (!(a < b)) {
            return;
        }
        //           v
        //   |-------|
        // |---|  |---|  |---|
        //               ^
        auto it = map_.upper_bound(b);
        while (it != map_.begin()) {
            --it;
            //       v
            //       |-------|
            // |---|            |---|
            //     ^
            if (it->second < a) {
                break;
            }
            // Note: This can only apply in the first iteration.
            //        v
            // -------|
            // -------------|
            //              ^
            if (b < it->second) {
                b = std::move(it->second);
            }
            //    v
            //    |-------|
            // |-----|
            // ^
            if (it->first < a) {
                it->second = std::move(b);
                return;
            }
            // |-------|
            //   |---|
            it = map_.erase(it);
        }
        map_.emplace_hint(it, std::move(a), std::move(b));
    }

    //! Inplace union with given interval set.
    void extend(IntervalSet const &other) {
        for (auto &[x, y] : other) {
            add(x, y);
        }
    }

    //! Clear the set.
    void clear() {
        map_.clear();
    }

    //! Enumerate the values in the interval.
    //!
    //! This shoud be used on types that behave like integers.
    template <typename F>
    void enumerate(F f) {
        for (const auto &[x, y] : map_) {
            for (auto i = x; i < y; ++i) {
                f(i);
            }
        }
    }

private:
    std::map<T, T> map_;
};

} // namespace Clingcon

#endif // CLINGCON_UTIL_H
