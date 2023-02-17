// An Either Monad in C++17

#include <optional>
#include <type_traits>

// is_instance<X, BaseTemplate>::value is true iff. X is of type BaseTemplate<T...> for some T...
template<typename, template<typename...> typename>
struct is_instance : public std::false_type {
};

template<typename... T, template<typename...> typename BaseTemplate>
struct is_instance<BaseTemplate<T...>, BaseTemplate> : public std::true_type {
};


template<typename X, typename Y>
struct Cont;

template<typename T>
struct Just {
    constexpr Just(T t) : _t{std::move(t)} {}

    template<typename X, typename Y>
    constexpr auto operator|(const Cont<X, Y> &cont) const {
        return cont(*this);
    }

    constexpr operator T() {
        return _t;
    }

    const T _t;
};

template<typename L, typename R>
struct Either {
    constexpr Either(L l, std::nullptr_t) : _l{std::move(l)} {}

    constexpr Either(std::nullptr_t, R r) : _r{std::move(r)} {}

    template<typename X, typename Y>
    constexpr auto operator|(const Cont<X, Y> &cont) const {
        return cont(*this);
    }

    constexpr L left_or(L l) const {
        return _l ? *_l : l;
    }

    template<typename Func>
    constexpr L left_or_eval(Func func) const {
        return _l ? *_l : func();
    }

    constexpr R right_or(R r) const {
        return _r ? *_r : r;
    }

    template<typename Func>
    constexpr R right_or_eval(Func func) const {
        return _r ? *_r : func();
    }

    bool constexpr is_left() const {
        return bool(_l);
    }

    bool constexpr is_right() const {
        return bool(_r);
    }

    const std::optional<L> _l;
    const std::optional<R> _r;
};

constexpr static auto identity = [](const auto &t) { return t; };

template<typename X, typename Y = decltype(identity)>
struct Cont {
    constexpr Cont(X x, Y y = identity) : _x{std::move(x)}, _y{std::move(y)} {}

    template<typename T>
    constexpr auto operator()(const Just<T> &j) const {
        auto res = _x(j._t);
        if constexpr (is_instance<decltype(res), Just>::value || is_instance<decltype(res), Either>::value) {
            return res;
        } else {
            return Just{res};
        }
    }

    template<typename L, typename R>
    constexpr auto operator()(const Either<L, R> &e) const
    -> Either<std::invoke_result_t<X, L>, std::invoke_result_t<Y, R>> {
        if (e._l) {
            return {_x(*e._l), nullptr};
        }
        return {nullptr, _y(*e._r)};
    }

    const X _x;
    const Y _y;
};

template<typename Func>
constexpr static auto continueRight(Func rightFunc) {
    return Cont{identity, std::move(rightFunc)};
}

template<typename T>
using Maybe = Either<T, std::nullptr_t>;

// test
int main() {
    constexpr static auto res =
            Just{12}
            | Cont{[](const auto i) { return i + 1; }}
            | Cont{[](const auto i) -> Either<int, float> {
                if (i == 12) return {14, nullptr};
                return {nullptr, 12.0};
            }}
            | Cont{[](const auto i) { return i; },
                   [](const auto f) { return f + 2.0; }};

    static_assert(res.right_or(0.0) == 14.0);

    return 0;
}