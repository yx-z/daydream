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

int main() { return 0; }
