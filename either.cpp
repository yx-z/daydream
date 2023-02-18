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

template<typename L, typename R>
struct Either {
    constexpr Either(L l, std::nullptr_t) : _left{std::move(l)} {}

    template<typename L2, typename R2>
    constexpr Either(const Either<L2, R2>& e) : _left{e.get_left()}, _right{e.get_right()} {}

    constexpr Either(std::nullptr_t, R r) : _right{std::move(r)} {}

    constexpr bool has_left() const {
        return bool(_left);
    }

    constexpr bool has_right() const {
        return bool(_right);
    }

    constexpr const L& get_left() const {
        return *_left;
    }

    constexpr const R& get_right() const {
        return *_right;
    }

    constexpr L left_or(const L& l) const {
        return has_left() ? get_left() : l;
    }

    template<typename Func>
    constexpr L left_or_eval(const Func& func) const {
        return has_left() ? get_left() : func();
    }

    constexpr R right_or(const R& r) const {
        return has_right() ? get_right() : r;
    }

    template<typename Func>
    constexpr R right_or_eval(const Func& func) const {
        return has_right() ? get_right() : func();
    }

    template<typename LeftCont, typename RightCont>
    constexpr auto operator|(const Continuation<LeftCont, RightCont>& cont) const {
        return cont(*this);
    }

private:
    const std::optional<L> _left;
    const std::optional<R> _right;
};


template<typename T>
struct Maybe : private Either<T, std::nullptr_t> {
    constexpr Maybe() : Either<T, std::nullptr_t>{nullptr, nullptr} {}

    constexpr Maybe(T value) : Either<T, std::nullptr_t>{value, nullptr} {}

    template<typename U>
    constexpr Maybe(const Maybe<U>& maybe) : Either<T, std::nullptr_t>{maybe ? *maybe : nullptr, nullptr} {}

    constexpr const T& value() const {
        return this->get_left();
    }

    constexpr const T& operator*() const {
        return value();
    }

    constexpr bool has_value() const {
        return this->has_left();
    }

    constexpr operator bool() const {
        return has_value();
    }

    template<typename LeftCont, typename RightCont>
    constexpr auto operator&&(const Continuation<LeftCont, RightCont>& cont) const {
        return cont(*this);
    }

    constexpr Maybe<T> value_or(const Maybe<T>& other) const {
        return has_value() ? *this : other;
    }

    constexpr Maybe<T> operator||(const Maybe<T>& other) const {
        return value_or(other);
    }

    constexpr T value_or(const T& t) const {
        return has_value() ? value() : t;
    }

    constexpr T operator||(const T& t) const {
        return value_or(t);
    }

    template<typename Func>
    constexpr T value_or_eval(const Func& func) const {
        return has_value() ? value() : func();
    }

    template<typename Func, std::enable_if_t<std::is_convertible_v<std::invoke_result_t<Func>, T>, int> = 0>
    constexpr T operator||(const Func& func) const {
        return value_or_eval(func);
    }
};


template<typename T>
struct Just : Maybe<T> {
    constexpr Just(T t) : Maybe<T>{std::move(t)} {}

    template<typename U>
    constexpr Just(const Just<U>& t) : Maybe<T>{std::move(t)} {}

    constexpr operator T() {
        return this->value();
    }

    template<typename LeftCont, typename RightCont>
    constexpr auto operator|(const Continuation<LeftCont, RightCont>& cont) const {
        return cont(*this);
    }
};


constexpr inline static auto identity = [](const auto& t) { return t; };

template<typename LeftCont, typename RightCont = decltype(identity)>
struct Continuation {
    constexpr Continuation(LeftCont leftCont, RightCont rightCont = identity)
            : _leftCont{std::move(leftCont)},
              _rightCont{std::move(rightCont)} {}

    template<typename L2, typename R2>
    constexpr Continuation(const Continuation<L2, R2>& cont) : _leftCont{cont._leftCont}, _rightCont{cont._rightCont} {}

    template<typename LeftContOther, typename RightContOther>
    constexpr auto operator|(const Continuation<LeftContOther, RightContOther>& other) const {
        const auto chainLeft = [other, *this](const auto& l) {
            return other._leftCont(_leftCont(l));
        };
        const auto chainRight = [other, *this](const auto& r) {
            return other._rightCont(_rightCont(r));
        };
        return Continuation<decltype(chainLeft), decltype(chainRight)>{chainLeft, chainRight};
    }


    template<typename E, std::enable_if_t<is_instance<E, Either>::value, int> = 0>
    constexpr auto operator()(const E& either) const {
        using L = decltype(_leftCont(either.get_left()));
        using R = decltype(_rightCont(either.get_right()));
        if (either.has_left()) {
            return Either<L, R>{_leftCont(either.get_left()), nullptr};
        }
        return Either<L, R>{nullptr, _rightCont(either.get_right())};
    }

    template<typename M, std::enable_if_t<is_instance<M, Maybe>::value, int> = 0>
    constexpr auto operator()(const M& maybe) const {
        using Res = decltype(_leftCont(*maybe));
        if constexpr (is_instance<Res, Maybe>::value) {
            return maybe ? _leftCont(*maybe) : Res{};
        } else {
            using Ret = Maybe<Res>;
            return maybe ? Ret{_leftCont(*maybe)} : Ret{};
        }
    }

    template<typename J, std::enable_if_t<is_instance<J, Just>::value, int> = 0>
    constexpr auto operator()(const J& just) const {
        auto res = _leftCont(*just);
        if constexpr (is_instance<decltype(res), Either>::value || is_instance<decltype(res), Maybe>::value) {
            return res;
        } else {
            return Just{res};
        }
    }

    const LeftCont _leftCont;
    const RightCont _rightCont;
};

template<typename Func>
constexpr static auto ContinueRight(const Func& rightFunc) {
    return Continuation{identity, std::move(rightFunc)};
}

template<typename Predicate>
constexpr auto checkThenContinue(const Predicate& predicate) {
    return Continuation{[predicate](const auto& input) {
        using Ret = Maybe<std::decay_t<decltype(input)>>;
        return predicate(input) ? Ret{input} : Ret{};
    }};
}

// Test

// chain operations
constexpr static auto basicUsage =
        Just{12}
        | Continuation{[](const auto i) { return i + 1; }}
        | Continuation{[](const auto i) -> Either<int, float> {
            if (i == 12) return {14, nullptr};
            return {nullptr, 12.0};
        }}
        | Continuation{[](const auto i) { return i; }, [](const auto f) { return f + 2.0; }}
        | ContinueRight([](const auto f) { return f * 2.0; });

static_assert(!basicUsage.has_left());
static_assert(basicUsage.right_or(0.0) == 28.0);

// can chain Continuations first, then apply to different input
constexpr static auto justOperations =
        Continuation{[](const auto i) { return i + 1; }} | Continuation{[](const auto i) { return i + 2; }};
constexpr static auto res1 = Just{0} | justOperations;
static_assert(*res1 == 3);
constexpr static auto res2 = Just{1} | justOperations;
static_assert(*res2 == 4);

constexpr static auto eitherOperations =
        Continuation{[](const auto i) { return i + 1; }, [](const auto f) { return f + 2.0; }} |
        Continuation{[](const auto i) { return i + 2; }, [](const auto f) { return f + 3.0; }};
constexpr static auto resL = Either<int, float>{1, nullptr} | eitherOperations;
static_assert(!resL.has_right());
static_assert(resL.left_or(0) == 4);

constexpr static auto resR = Either<int, float>{nullptr, 10.0} | eitherOperations;
static_assert(!resR.has_left());
static_assert(resR.right_or(0.0) == 15.0);

// Maybe monad
constexpr static auto empty = Maybe<int>{};
static_assert(!empty);
static_assert((empty || 13) == 13);
static_assert((empty || []() { return 14; }) == 14);

constexpr static auto hasValue = Maybe<int>{12};
static_assert(*hasValue == 12);
static_assert(*(empty || hasValue) == 12);

constexpr static auto anotherValue = Maybe<int>{13};
static_assert(*(hasValue || anotherValue) == 12);
static_assert(!(empty && justOperations));
static_assert(*(hasValue && justOperations) == 15);
static_assert(*(hasValue && checkThenContinue([](const auto i) { return i > 10; })) == 12);

int main() {}
