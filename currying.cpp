#include <type_traits>

template<typename, typename = void>
struct self_invokable : std::false_type {
};

template<typename T>
struct self_invokable<T, std::void_t<std::invoke_result_t<T>>> : std::true_type {
};

template<typename Func>
constexpr auto currying(Func&& func) {
    if constexpr (self_invokable<Func>::value) {
        // at the last step, we have to produce result instead of returning new functions
        return func();
    } else {
        return [func](auto&& arg) {
            return currying(
                    [func, arg](auto&& ...args) -> decltype(func(arg, args...)) {
                        return func(arg, args...);
                    }
            );
        };
    }
}


constexpr static auto addThenMult = [](int i1, int i2, int i3) { return (i1 + i2) * i3; };
constexpr static auto res = currying(addThenMult)(1)(2)(3);
static_assert(res == 9);


// separate implementation involving <tuple>
// curry of zero-and-one-arg functions are trivial: curry(f) = f for such functions.
// hence their specializations are ignored.
// TODO: make below constexpr and better template argument deductible?

#include <functional>
#include <tuple>
#include <iostream>

template <typename Ret, typename Arg1, typename Arg2, typename... Args>
auto curry(std::function<Ret(std::tuple<Arg1, Arg2, Args...>)> f) {
    if constexpr (sizeof...(Args) == 0) {
        return [=](Arg1 arg1) { return [=](Arg2 arg2) { return f(std::tuple{arg1, arg2}); }; };
    } else {
        return [=](Arg1 arg1) {
            return curry(std::function{[=](std::tuple<Arg2, Args...> tup) { 
                return std::apply([=](auto... args){ return f(std::tuple{arg1, args...}); }, tup);
            }});
        };
    }
}

const static auto two_args = curry(
    std::function([](std::tuple<int, int> pair) { return std::get<0>(pair) + std::get<1>(pair); })
);
const static auto three_args = curry(
    std::function([](std::tuple<int, int, int> triple) {
        return std::get<0>(triple) * std::get<1>(triple) * std::get<2>(triple); })
);

int main() {
    std::cout << two_args(1)(2) << std::endl; // 3
    std::cout << three_args(1)(2)(3) << std::endl; // 6
    return 0;
}
