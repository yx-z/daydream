// either/maybe/just Monad in C++17

#include <optional>
#include <type_traits>

namespace daydream {

    // is_instance<X, BaseTemplate>::value is true iff. X is of type
    // BaseTemplate<Param...> for some Param...
    template<typename, template<typename...> typename>
    struct is_instance : public std::false_type {
    };

    template<typename... Param, template<typename...> typename BaseTemplate>
    struct is_instance<BaseTemplate<Param...>, BaseTemplate> : public std::true_type {
    };

    template<typename T, template<typename...> typename BaseTemplate>
    constexpr static auto is_instance_v = is_instance<T, BaseTemplate>::value;

    template<typename L, typename R>
    struct either;

    template<typename T>
    struct maybe;

    template<typename T>
    struct just;

    template<typename T>
    constexpr static inline auto is_monadic_v =
            is_instance_v<T, either> || is_instance_v<T, maybe> || is_instance_v<T, just>;

    template<typename LeftCont, typename RightCont>
    struct continue_either;

    template<typename L, typename R>
    struct either {
        constexpr either(L l, std::nullptr_t) : _left{std::move(l)} {}

        constexpr either(std::nullptr_t, R r) : _right{std::move(r)} {}

        template<typename L2, typename R2>
        constexpr either(const either<L2, R2>& e) : _left{e.left()}, _right{e.right()} {}

        template<typename L2, typename R2>
        constexpr either(either<L2, R2>&& e) : _left{std::move(e.left())}, _right{std::move(e.right())} {}

        constexpr bool has_left() const { return bool(_left); }

        constexpr bool has_right() const { return bool(_right); }

        constexpr const L& left() const noexcept(false) { return *_left; }

        constexpr const R& right() const noexcept(false) { return *_right; }

        template<typename U>
        constexpr L left_or(const U& l) const { return _left ? *_left : l; }

        template<typename Func>
        constexpr L left_or_eval(const Func& func) const { return _left ? _left : func(); }

        template<typename U>
        constexpr R right_or(const U& r) const { return _right ? *_right : r; }

        template<typename Func>
        constexpr R right_or_eval(const Func& func) const { return _right ? _right : func(); }

        template<typename Func>
        constexpr auto operator|(const Func& cont) const { return cont(*this); }

        constexpr either<R, L> swap() const { return {_right, _left}; }

    private:
        const std::optional<L> _left;
        const std::optional<R> _right;
    };

    struct drop_right {
        template<typename L, typename R>
        constexpr maybe<L> operator()(const either<L, R>& either) const {
            if (either.has_left()) {
                return either.left();
            }
            return {};
        }
    };

    struct drop_left {
        template<typename L, typename R>
        constexpr maybe<R> operator()(const either<L, R>& either) const {
            if (either.has_right()) {
                return either.right();
            }
            return {};
        }
    };

    template<typename T>
    struct maybe {
        constexpr maybe() {}

        constexpr maybe(T value) : _val{std::move(value)} {}

        constexpr maybe(std::optional<T> value) : _val{std::move(value)} {}

        template<typename U>
        constexpr maybe(const maybe<U>& maybe) : _val{maybe} {}

        template<typename U>
        constexpr maybe(maybe<U>&& maybe) : _val{std::move(maybe)} {}

        constexpr operator std::optional<T>() const { return _val; }

        constexpr const T& operator*() const noexcept(false) { return *_val; }

        constexpr operator bool() const { return bool(_val); }

        template<typename LeftCont, typename RightCont>
        constexpr auto operator&&(const continue_either<LeftCont, RightCont>& cont) const { return cont(*this); }

        template<typename Func>
        constexpr auto operator&&(const Func& cont) const {
            using Res = decltype(cont(*_val));
            if constexpr (is_monadic_v<Res>) {
                return _val ? cont(*_val) : Res{};
            } else {
                using Ret = maybe<Res>;
                return _val ? Ret{cont(*_val)} : Ret{};
            }
        }

        template<typename Func>
        constexpr auto operator|(const Func& func) const { return *this && func; }

        template<typename U>
        constexpr maybe<T> operator||(const maybe<U>& other) const { return _val ? *this : other; }

        template<typename U, std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
        constexpr T operator||(const U& t) const { return _val ? *_val : t; }

        template<typename Func, std::enable_if_t<std::is_convertible_v<std::invoke_result_t<Func>, T>, int> = 0>
        constexpr T operator||(const Func& func) const { return _val ? *_val : func(); }

    private:
        const std::optional<T> _val;
    };

    template<typename T>
    struct just {
        constexpr just(T t) : _val{std::move(t)} {}

        template<typename U>
        constexpr just(const just<U>& t) : _val{t} {}

        template<typename U>
        constexpr just(just<U>&& t) : _val{std::move(t)} {}

        constexpr const T& operator*() const { return _val; }

        constexpr operator T() const { return _val; }

        template<typename Func>
        constexpr auto operator|(const Func& cont) const {
            auto res = cont(_val);
            if constexpr (is_monadic_v<decltype(res)>) {
                return res;
            } else {
                return just{res};
            }
        }

    private:
        const T _val;
    };

    template<typename LeftCont, typename RightCont>
    struct continue_either {
        constexpr continue_either(LeftCont leftCont, RightCont rightCont) : left{std::move(leftCont)},
                                                                            right{std::move(rightCont)} {}

        template<typename L2, typename R2>
        constexpr continue_either(const continue_either<L2, R2>& cont) : left{cont.left()}, right{cont.right()} {}

        template<typename L2, typename R2>
        constexpr
        continue_either(continue_either<L2, R2>&& cont) : left{std::move(cont.left())},
                                                          right{std::move(cont.right())} {}

        template<typename LeftContOther, typename RightContOther>
        constexpr auto operator|(const continue_either<LeftContOther, RightContOther>& other) const {
            const auto chainLeft = [other, *this](const auto& l) { return other.left(left(l)); };
            const auto chainRight = [other, *this](const auto& r) { return other.right(right(r)); };
            return continue_either<decltype(chainLeft), decltype(chainRight)>{chainLeft, chainRight};
        }

        template<typename Func>
        constexpr auto operator|(const Func& other) const {
            const auto chain = [other, *this](const auto& l) { return other(left(l)); };
            return continue_either<decltype(chain), RightCont>{chain, right};
        }

        template<typename E1, typename E2>
        constexpr auto operator()(const either<E1, E2>& other) const {
            using L = decltype(left(other.left()));
            using R = decltype(right(other.right()));

            if (other.has_left()) {
                return either<L, R>{left(other.left()), nullptr};
            }
            return either<L, R>{nullptr, right(other.right())};
        }

        template<typename T>
        constexpr auto operator()(const maybe<T>& other) const {
            using Res = decltype(left(*other));
            if constexpr (is_monadic_v<Res>) {
                if (other) {
                    return left(*other);
                }
                right(nullptr);
                return Res{};
            } else {
                using Ret = maybe<Res>;
                if (other) {
                    return Ret{left(*other)};
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

    constexpr inline static auto identity = [](const auto& t) { return t; };
    using identity_t = decltype(identity);

    template<typename Func>
    struct continue_left : continue_either<Func, identity_t> {
        constexpr continue_left(Func func) : continue_either<Func, identity_t>{std::move(func), identity} {}
    };

    template<typename Func>
    struct continue_right : continue_either<identity_t, Func> {
        constexpr continue_right(Func func) : continue_either<identity_t, Func>{identity, std::move(func)} {}
    };

    template<typename Predicate>
    struct check {
        constexpr check(Predicate pred) : _pred{std::move(pred)} {}

        template<typename T>
        constexpr auto operator()(const T& input) const {
            using Ret = maybe<std::remove_cv_t<std::remove_reference_t<decltype(input)>>>;
            if (_pred(input)) {
                return Ret{input};
            }
            return Ret{};
        }

        const Predicate _pred;
    };

    // Test

    // chain operations
    constexpr static auto basicUsage =
            just{12}
            | [](const auto i) { return i + 1; }
            | [](const auto i) -> either<int, float> {
                if (i == 12) {
                    return {14, nullptr};
                }
                return {nullptr, 12.0};
            }
            | continue_either{[](const auto i) { return i; },
                              [](const auto f) { return f + 2.0; }}
            | continue_right{[](const auto f) { return f * 2.0; }};

    static_assert(!basicUsage.has_left());
    static_assert(basicUsage.right_or(0.0) == 28.0);

    // chain then drop
    constexpr static auto dropped =
            either<int, float>{12, nullptr}
            | drop_right{}
            | [](const auto i) { return i + 1; };
    static_assert((dropped || 12) == 13);

    // can chain `continue_either` first, then apply to different input
    constexpr static auto justOperations =
            continue_left{[](const auto i) { return i + 1; }} | [](const auto i) { return i + 2; };
    constexpr static auto res1 = just{0} | justOperations;
    static_assert(*res1 == 3);
    constexpr static auto res2 = just{1} | justOperations;
    static_assert(*res2 == 4);

    constexpr static auto eitherOperations =
            continue_either{[](const auto i) { return i + 1; },
                            [](const auto f) { return f + 2.0; }}
            | continue_either{[](const auto i) { return i + 2; },
                              [](const auto f) { return f + 3.0; }};
    constexpr static auto resL = either<int, float>{1, nullptr} | eitherOperations;
    static_assert(!resL.has_right());
    static_assert(resL.left_or(0) == 4);

    constexpr static auto resR = either<int, float>{nullptr, 10.0} | eitherOperations;
    static_assert(!resR.has_left());
    static_assert(resR.right_or(0.0) == 15.0);

    // maybe monad
    constexpr static auto empty = maybe<int>{};
    static_assert(!empty);
    static_assert((empty || 13) == 13);
    static_assert((empty || [] { return 14; }) == 14);
    static_assert(!(empty && justOperations));

    constexpr static auto hasValue = maybe{12};
    static_assert(*hasValue == 12);
    static_assert(*(empty || hasValue) == 12);

    static_assert((hasValue || 13) == 12);
    static_assert(*(hasValue || maybe{13}) == 12);
    static_assert(*(hasValue && justOperations) == 15);
    static_assert(*(hasValue && check{[](const auto i) { return i > 10; }}) == 12);
    static_assert(!(hasValue && check{[](const auto i) { return i > 100; }}));

} // namespace daydream

int main() { return 0; }
