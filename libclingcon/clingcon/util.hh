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
#include <cstdlib>
#include <stdexcept>

//! @file clingcon/util.hh
//! Very general utility functions.
//!
//! @author Roland Kaminski

namespace Clingcon {

//! Calculate the midpoint of two values of integral type.
template<typename I>
I midpoint(I a, I b) noexcept {
    using U = std::make_unsigned_t<I>;
    return static_cast<I>(static_cast<U>(a) + (static_cast<U>(b) - static_cast<U>(a)) / 2);
}

template<typename I>
I floordiv(I n, I m) {
    auto a = std::div(n, m);
    if ((n < 0) ^ (m < 0) && a.rem != 0) {
        return a.quot-1;
    }
    return a.quot;
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
    [[nodiscard]] bool get_flag_unique(T const &x) const {
        return x->flag_unique;
    }
    bool set_flag_unique(T &x) const {
        auto ret = x->flag_unique;
        x->flag_unique = true;
        return ret;
    }
    void unset_flag_unique(T &x) const {
        x->flag_unique = false;
    }
};

//! Like a vector but only adds an element if it is not contained yet.
template <typename T, class Flagger=FlagUnique<T>>
class UniqueVector : private Flagger {
public:
    using Vector = typename std::vector<T>;
    using Iterator = typename Vector::iterator;
    using ConstIterator = typename Vector::const_iterator;

    //! Create an empty vector.
    UniqueVector(const Flagger &m = Flagger())
    : Flagger(m) {
    }

    //! Destroy the vector but does *not* unmarking the contained elements.
    ~UniqueVector() = default;

    //! Move construct the vector.
    //!
    //! Implemented via swap.
    UniqueVector(UniqueVector &&x) noexcept {
        *static_cast<Flagger*>(this) = std::move(*static_cast<Flagger*>(x));
        std::swap(vec_, x.vec_);
    }
    //! Move assign the vector.
    //!
    //! Implemented via swap.
    UniqueVector &operator=(UniqueVector &&x) noexcept {
        *static_cast<Flagger*>(this) = std::move(*static_cast<Flagger*>(x));
        std::swap(vec_, x.vec_);
    }

    UniqueVector(UniqueVector const &) = delete;
    UniqueVector &operator=(UniqueVector const &) = delete;

    //! Check if the vector is empty.
    [[nodiscard]] bool empty() const {
        return vec_.empty();
    }

    //! Get the size of the vector.
    [[nodiscard]] size_t size() const {
        return vec_.size();
    }

    //! Get the element at the given position.
    [[nodiscard]] T &operator[](size_t i) const {
        return vec_[i];
    }

    //! Check if the vector contains an element.
    template <typename... Args>
    [[nodiscard]] bool contains(T const &x, Args&&... args) const {
        return Flagger::get_flag_unique(x, std::forward<Args>(args)...);
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
    template <typename... Args>
    void erase(ConstIterator it, Args&&... args) {
        Flagger::unset_flag_unique(const_cast<T &>(*it), std::forward<Args>(args)...); // NOLINT
        vec_.erase(it);
    }

    //! Clear the vector.
    template <typename... Args>
    void clear(Args&&... args) {
        for (auto &x : vec_) {
            Flagger::unset_flag_unique(x, std::forward<Args>(args)...);
        }
        vec_.clear();
    }

    //! Add elements from a sequence to the vector.
    template <typename It, typename... Args>
    size_t extend(It begin, It end, Args... args) {
        size_t n = 0;
        for (auto it = begin; it != end; ++it) {
            if (append(*it, args...)) {
                ++n;
            }
        }
        return n;
    }

    //! Append an element to the vector.
    template <typename U, typename... Args>
    bool append(U&& x, Args&&... args) {
        vec_.reserve(vec_.size() + 1);
        if (Flagger::set_flag_unique(x, std::forward<Args>(args)...)) {
            return false;
        }
        vec_.emplace_back(std::forward<U>(x));
        return true;
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
    using ReverseIterator = typename Map::const_reverse_iterator;

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

    //! Iterator to the last element in the set.
    [[nodiscard]] ReverseIterator rbegin() const {
        return map_.rbegin();
    }

    //! Iterator to pointing before the first element.
    [[nodiscard]] ReverseIterator rend() const {
        return map_.rend();
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
        auto it = map_.lower_bound(b);
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

    //! Check if the set contains in interval.
    [[nodiscard]] bool intersects(T const &a, T const &b) const {
        if (b <= a) {
            return false;
        }
        //           v
        //   |-------|
        // |---|  |---|  |---|
        //               ^
        auto it = map_.lower_bound(b);
        if (it == map_.begin()) {
            return false;
        }
        --it;
        //   v
        //   |-------|
        // ------------|
        //             ^
        return it->second > a;
    }

    //! Check if the set contains a value.
    [[nodiscard]] bool contains(T const &a) const {
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
        return a < it->second;
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
                if (!f(i)) {
                    return;
                }
            }
        }
    }

private:
    std::map<T, T> map_;
};

// Some of the functions below could also be implemented using (much faster)
// compiler specific built-ins. For more information check the following links:
// - https://gcc.gnu.org/onlinedocs/gcc/Integer-Overflow-Builtins.html
// - https://wiki.sei.cmu.edu/confluence/display/c/INT32-C.+Ensure+that+operations+on+signed+integers+do+not+result+in+overflow

//! Safely add a and b throwing an exception in case of overflow/underflow.
template <typename Int>
Int safe_add(Int a, Int b) {
    if (b > 0) {
        if (a > std::numeric_limits<Int>::max() - b) {
            throw std::overflow_error("integer overflow");
        }
    }
    else if (b < 0) {
        if (a < std::numeric_limits<Int>::min() - b) {
            throw std::underflow_error("integer underflow");
        }
    }
    return a + b;
}

//! Safely subtract a and b throwing an exception in case of
//! overflow/underflow.
template <typename Int>
Int safe_sub(Int a, Int b) {
    if (b > 0) {
        if (a < std::numeric_limits<Int>::min() + b) {
            throw std::underflow_error("integer underflow");
        }
    }
    else if (b < 0) {
        if (a > std::numeric_limits<Int>::max() + b) {
            throw std::overflow_error("integer overflow");
        }
    }
    return a - b;
}

//! Safely multiply a and b throwing an exception in case of
//! overflow/underflow.
template <typename Int>
Int safe_mul(Int a, Int b) {
    if (a > 0) {
        if (b > 0) {
            if (a > (std::numeric_limits<Int>::max() / b)) {
                throw std::overflow_error("integer overflow");
            }
        }
        else if (b < (std::numeric_limits<Int>::min() / a)) {
            throw std::underflow_error("integer underflow");
        }
    } else {
        if (b > 0) {
            if (a < (std::numeric_limits<Int>::min() / b)) {
                throw std::underflow_error("integer underflow");
            }
        } else if (a < 0 && b < (std::numeric_limits<Int>::max() / a)) {
            throw std::overflow_error("integer overflow");
        }
    }
    return a * b;
}

//! Safely divide a and b throwing an exception in case of overflow/underflow.
template <typename Int>
Int safe_div(Int a, Int b) {
    if (a == std::numeric_limits<Int>::min() && b == -1) {
        throw std::overflow_error("integer overflow");
    }
    if (b == 0) {
        if (a < 0) {
            throw std::underflow_error("integer underflow");
        }
        throw std::overflow_error("integer overflow");
    }
    return a / b;
}

//! Safely calculate the modulo of a and b throwing an exception in case of
//! overflow/underflow.
template <typename Int>
Int safe_mod(Int a, Int b) {
    if (a == std::numeric_limits<Int>::min() && b == -1) {
        throw std::overflow_error("integer overflow");
    }
    if (b == 0) {
        if (a < 0) {
            throw std::underflow_error("integer underflow");
        }
        throw std::overflow_error("integer overflow");
    }
    return a % b;
}

//! Safely invert a throwing an exception in case of an underflow.
template <typename Int>
Int safe_inv(Int a) {
    if (a == std::numeric_limits<Int>::min()) {
        throw std::overflow_error("integer overflow");
    }
    return -a;
}

} // namespace Clingcon

#endif // CLINGCON_UTIL_H
