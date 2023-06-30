#include <type_traits>
#include <functional>
#include <iostream>

template<typename Func>
constexpr auto currying(Func func) {
    if constexpr (std::is_invocable_v<Func>) {
        // at the last step, we have to produce result instead of returning new functions
        return func();
    } else {
        return [func = std::move(func)](auto arg) {
            return currying(
                    [func = std::move(func), arg = std::move(arg)](auto&& ...args) -> decltype(func(arg, args...)) {
                        return func(arg, args...);
                    }
            );
        };
    }
}

static_assert(currying([](auto i1) { return i1 + 1; })(11) == 12);

constexpr static auto add_then_mult = currying([](auto i1, auto i2, auto i3) { return (i1 + i2) * i3; });
static_assert(add_then_mult(1)(2)(3) == 9);

// separate implementation involving std::function
template<typename Ret, typename Arg, typename... Args>
auto curry(std::function<Ret(Arg, Args...)> f) {
    if constexpr (sizeof...(Args) == 0) {
        return f;
    } else {
        return [f](Arg arg) {
            return curry(std::function{[f, arg](Args... args) { return f(arg, args...); }});
        };
    }
}

int main() {
    const static auto one_arg = curry(std::function{[](int i1) { return i1 + 1; }});
    std::cout << one_arg(2) << std::endl; // 3

    const static auto two_args = curry(std::function{[](int i1, int i2) { return i1 + i2; }});
    std::cout << two_args(1)(2) << std::endl; // 3

    const static auto three_args = curry(std::function{[](int i1, int i2, int i3) { return i1 + i2 * i3; }});
    std::cout << three_args(1)(2)(3) << std::endl; // 7

    return 0;
}
