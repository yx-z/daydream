// Either/Maybe/Just Monad in C++17

#include <optional>
#include <type_traits>

namespace daydream {

    // is_instance<X, BaseTemplate>::value is true iff. X is of type
    // BaseTemplate<T...> for some T...
    template<typename, template<typename...> typename>
    struct is_instance : public std::false_type {
    };

    template<typename... T, template<typename...> typename BaseTemplate>
    struct is_instance<BaseTemplate<T...>, BaseTemplate> : public std::true_type {
    };

    template<typename L, typename R>
    struct Either;

    template<typename T>
    struct Maybe;

    template<typename T>
    struct Just;

    template<typename T>
    constexpr static inline auto is_monadic_v =
            is_instance<T, Either>::value || is_instance<T, Maybe>::value || is_instance<T, Just>::value;

    template<typename LeftCont, typename RightCont>
    struct Continue;

    template<typename L, typename R>
    struct Either {
        constexpr Either(L l, std::nullptr_t) : _left{std::move(l)} {}

        constexpr Either(std::nullptr_t, R r) : _right{std::move(r)} {}

        template<typename L2, typename R2>
        constexpr Either(const Either<L2, R2>& e) : _left{e.left()}, _right{e.right()} {}

        template<typename L2, typename R2>
        constexpr Either(Either<L2, R2>&& e) : _left{std::move(e.left())}, _right{std::move(e.right())} {}

        constexpr bool has_left() const { return bool(_left); }

        constexpr bool has_right() const { return bool(_right); }

        constexpr const L& left() const noexcept(false) { return *_left; }

        constexpr const R& right() const noexcept(false) { return *_right; }

        template<typename U>
        constexpr L left_or(const U& l) const { return has_left() ? left() : l; }

        template<typename Func>
        constexpr L left_or_eval(Func&& func) const { return has_left() ? left() : func(); }

        template<typename U>
        constexpr R right_or(const U& r) const { return has_right() ? right() : r; }

        template<typename Func>
        constexpr R right_or_eval(Func&& func) const { return has_right() ? right() : func(); }

        template<typename Func>
        constexpr auto operator|(Func&& cont) const { return cont(*this); }

    private:
        const std::optional<L> _left;
        const std::optional<R> _right;
    };

    struct DropRight {
        constexpr DropRight() {}

        template<typename L, typename R>
        constexpr Maybe<L> operator()(const Either<L, R>& either) {
            if (either.has_left()) {
                return either.left();
            }
            return {};
        }
    };

    struct DropLeft {
        constexpr DropLeft() {}

        template<typename L, typename R>
        constexpr Maybe<R> operator()(const Either<L, R>& either) {
            if (either.has_right()) {
                return either.right();
            }
            return {};
        }
    };

    template<typename T>
    struct Maybe {
        constexpr Maybe() {}

        constexpr Maybe(T value) : _val{std::move(value)} {}

        constexpr Maybe(std::optional<T> value) : _val{std::move(value)} {}

        template<typename U>
        constexpr Maybe(const Maybe<U>& maybe) : _val{maybe} {}

        template<typename U>
        constexpr Maybe(Maybe<U>&& maybe) : _val{std::move(maybe)} {}

        constexpr operator std::optional<T>() const { return _val; }

        constexpr const T& value() const noexcept(false) { return *_val; }

        constexpr const T& operator*() const noexcept(false) { return value(); }

        constexpr bool has_value() const { return bool(_val); }

        constexpr operator bool() const { return has_value(); }

        template<typename LeftCont, typename RightCont>
        constexpr auto operator&&(const Continue<LeftCont, RightCont>& cont) const { return cont(*this); }

        template<typename Func>
        constexpr auto operator&&(Func&& cont) const {
            using Res = decltype(cont(value()));
            if constexpr (is_monadic_v<Res>) {
                return has_value() ? cont(value()) : Res{};
            } else {
                using Ret = Maybe<Res>;
                return has_value() ? Ret{cont(value())} : Ret{};
            }
        }

        template<typename Func>
        constexpr auto operator|(Func&& func) const { return *this && func; }

        template<typename U>
        constexpr Maybe<T> value_or(const Maybe<U>& other) const { return has_value() ? *this : other; }

        template<typename U>
        constexpr Maybe<T> operator||(const Maybe<U>& other) const { return value_or(other); }

        template<typename U>
        constexpr T value_or(const U& t) const { return has_value() ? value() : t; }

        template<typename U>
        constexpr T operator||(const U& t) const { return value_or(t); }

        template<typename Func>
        constexpr T value_or_eval(Func&& func) const { return has_value() ? value() : func(); }

        template<typename Func, std::enable_if_t<std::is_convertible_v<std::invoke_result_t<Func>, T>, int> = 0>
        constexpr T operator||(Func&& func) const { return value_or_eval(func); }

    private:
        const std::optional<T> _val;
    };

    template<typename T>
    struct Just {
        constexpr Just(T t) : _val{std::move(t)} {}

        template<typename U>
        constexpr Just(const Just<U>& t) : _val{t} {}

        template<typename U>
        constexpr Just(Just<U>&& t) : _val{std::move(t)} {}

        constexpr const T& value() const { return _val; }

        constexpr const T& operator*() const { return value(); }

        constexpr operator T() const { return value(); }

        template<typename Func>
        constexpr auto operator|(Func&& cont) const {
            auto res = cont(value());
            if constexpr (is_monadic_v<decltype(res)>) {
                return res;
            } else {
                return Just{res};
            }
        }

    private:
        const T _val;
    };

    constexpr inline static auto identity = [](const auto& t) { return t; };

    template<typename LeftCont, typename RightCont = decltype(identity)>
    struct Continue {
        constexpr Continue(LeftCont leftCont, RightCont rightCont = identity) : left{std::move(leftCont)},
                                                                                right{std::move(rightCont)} {}

        template<typename L2, typename R2>
        constexpr Continue(const Continue<L2, R2>& cont) : left{cont.left()}, right{cont.right()} {}

        template<typename L2, typename R2>
        constexpr Continue(Continue<L2, R2>&& cont) : left{std::move(cont.left())}, right{std::move(cont.right())} {}

        template<typename LeftContOther, typename RightContOther>
        constexpr auto operator|(const Continue<LeftContOther, RightContOther>& other) const {
            const auto chainLeft = [other, *this](const auto& l) { return other.left(left(l)); };
            const auto chainRight = [other, *this](const auto& r) { return other.right(right(r)); };
            return Continue<decltype(chainLeft), decltype(chainRight)>{chainLeft, chainRight};
        }

        template<typename E1, typename E2>
        constexpr auto operator()(const Either<E1, E2>& either) const {
            using L = decltype(left(either.left()));
            using R = decltype(right(either.right()));

            if (either.has_left()) {
                return Either<L, R>{left(either.left()), nullptr};
            }
            return Either<L, R>{nullptr, right(either.right())};
        }

        template<typename T>
        constexpr auto operator()(const Maybe<T>& maybe) const {
            using Res = decltype(left(*maybe));
            if constexpr (is_monadic_v<Res>) {
                if (maybe) {
                    return left(*maybe);
                }
                right(nullptr);
                return Res{};
            } else {
                using Ret = Maybe<Res>;
                if (maybe) {
                    return Ret{left(*maybe)};
                }
                right(nullptr);
                return Ret{};
            }
        }

        template<typename T>
        constexpr auto operator()(const T& t) const { return left(t); }

        const LeftCont left;
        const RightCont right;
    };

    template<typename Func>
    constexpr static auto ContinueRight(const Func& rightFunc) { return Continue{identity, rightFunc}; }

    constexpr static auto noOp = [](const auto&) {};

    template<typename Predicate, typename OnEmpty = decltype(noOp)>
    constexpr auto check(Predicate predicate, OnEmpty onEmpty = noOp) {
        return Continue{[pred = std::move(predicate), empty = std::move(onEmpty)](const auto& input) {
            using Ret = Maybe<std::decay_t<decltype(input)> >;
            if (pred(input)) {
                return Ret{input};
            }
            empty(input);
            return Ret{};
        }};
    }

    // Test

    // chain operations
    constexpr static auto basicUsage =
            Just{12}
            | [](const auto i) { return i + 1; }
            | [](const auto i) -> Either<int, float> {
                if (i == 12) {
                    return {14, nullptr};
                }
                return {nullptr, 12.0};
            }
            | Continue{[](const auto i) { return i; },
                       [](const auto f) { return f + 2.0; }}
            | ContinueRight([](const auto f) { return f * 2.0; });

    static_assert(!basicUsage.has_left());
    static_assert(basicUsage.right_or(0.0) == 28.0);

    // chain then drop
    constexpr static auto dropped =
            Either<int, float>{12, nullptr}
            | DropRight{}
            | [](const auto i) { return i + 1; };
    static_assert((dropped || 12) == 13);

    // can chain `Continue` first, then apply to different input
    constexpr static auto justOperations =
            Continue{[](const auto i) { return i + 1; }}
            | Continue{[](const auto i) { return i + 2; }};
    constexpr static auto res1 = Just{0} | justOperations;
    static_assert(*res1 == 3);
    constexpr static auto res2 = Just{1} | justOperations;
    static_assert(*res2 == 4);

    constexpr static auto eitherOperations =
            Continue{[](const auto i) { return i + 1; },
                     [](const auto f) { return f + 2.0; }}
            | Continue{[](const auto i) { return i + 2; },
                       [](const auto f) { return f + 3.0; }};
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
    static_assert((empty || [] { return 14; }) == 14);
    static_assert(!(empty && justOperations));

    constexpr static auto hasValue = Maybe{12};
    static_assert(*hasValue == 12);
    static_assert(*(empty || hasValue) == 12);

    static_assert((hasValue || 13) == 12);
    static_assert(*(hasValue || Maybe{13}) == 12);
    static_assert(*(hasValue && justOperations) == 15);
    static_assert(*(hasValue && check([](const auto i) { return i > 10; })) == 12);
    static_assert(!(hasValue && check([](const auto i) { return i > 100; })));

} // namespace daydream

int main() { return 0; }
