// An Either Monad in C++17

#include <optional>
#include <type_traits>

// is_instance<LeftCont, BaseTemplate>::value is true iff. LeftCont is of type BaseTemplate<T...> for some T...
template<typename, template<typename...> typename>
struct is_instance : public std::false_type {
};

template<typename... T, template<typename...> typename BaseTemplate>
struct is_instance<BaseTemplate<T...>, BaseTemplate> : public std::true_type {
};


template<typename X, typename Y>
struct Continuation;

template<typename T>
struct Just {
    constexpr Just(T t) : _value{std::move(t)} {}

    constexpr T operator*() const {
        return get();
    }

    constexpr T get() const {
        return _value;
    }

    template<typename X, typename Y>
    constexpr auto operator|(const Continuation<X, Y> &cont) const {
        return cont(*this);
    }

    constexpr operator T() {
        return get();
    }

private:
    const T _value;
};

template<typename L, typename R>
struct Either {
    constexpr Either(L l, std::nullptr_t) : _left{std::move(l)} {}

    constexpr Either(std::nullptr_t, R r) : _right{std::move(r)} {}

    template<typename X, typename Y>
    constexpr auto operator|(const Continuation<X, Y> &cont) const {
        return cont(*this);
    }


    constexpr bool has_left() const {
        return bool(_left);
    }

    constexpr bool has_right() const {
        return bool(_right);
    }

    constexpr L get_left() const {
        return *_left;
    }

    constexpr R get_right() const {
        return *_right;
    }

    constexpr L left_or(L l) const {
        return has_left() ? get_left() : l;
    }

    template<typename Func>
    constexpr L left_or_eval(Func func) const {
        return has_left() ? get_left() : func();
    }

    constexpr R right_or(R r) const {
        return has_right() ? get_right() : r;
    }

    template<typename Func>
    constexpr R right_or_eval(Func func) const {
        return has_right() ? get_right() : func();
    }

private:
    const std::optional<L> _left;
    const std::optional<R> _right;
};

constexpr static auto identity = [](const auto &t) { return t; };

template<typename LeftCont, typename RightCont = decltype(identity)>
struct Continuation {
    constexpr Continuation(LeftCont leftCont, RightCont rightCont = identity)
            : _leftCont{std::move(leftCont)},
              _rightCont{std::move(rightCont)} {}

    template<typename T>
    constexpr auto operator()(const Just<T> &just) const {
        auto res = _leftCont(*just);
        if constexpr (is_instance<decltype(res), Just>::value || is_instance<decltype(res), Either>::value) {
            return res;
        } else {
            return Just{res};
        }
    }

    template<typename L, typename R>
    constexpr auto operator()(const Either<L, R> &either) const
    -> Either<std::invoke_result_t<LeftCont, L>, std::invoke_result_t<RightCont, R>> {
        if (either.has_left()) {
            return {_leftCont(either.get_left()), nullptr};
        }
        return {nullptr, _rightCont(either.get_right())};
    }

private:
    const LeftCont _leftCont;
    const RightCont _rightCont;
};

template<typename Func>
constexpr static auto continueRight(Func rightFunc) {
    return Continuation{identity, std::move(rightFunc)};
}

template<typename T>
using Maybe = Either<T, std::nullptr_t>;

// test
int main() {
    constexpr static auto res =
            Just{12}
            | Continuation{[](const auto i) { return i + 1; }}
            | Continuation{[](const auto i) -> Either<int, float> {
                if (i == 12) return {14, nullptr};
                return {nullptr, 12.0};
            }}
            | Continuation{[](const auto i) { return i; },
                           [](const auto f) { return f + 2.0; }}
            | continueRight([](const auto f) { return f * 2.0; });

    static_assert(!res.has_left());

    static_assert(res.right_or(0.0) == 28.0);

    return 0;
}