#ifndef DP_DEFER
#define DP_DEFER

#include <type_traits>
#include <utility>
#include <functional>

namespace dp {
    template<typename Callable, std::enable_if_t<std::is_invocable_v<Callable>, bool> = true>
    class Defer {

        using cleanup_type = std::conditional_t<std::is_function_v<Callable>, std::add_pointer_t<Callable>, Callable>;

        cleanup_type cleanup;

    public:
        Defer(Callable&& inFunc) : cleanup{ std::forward<decltype(inFunc)>(inFunc) } {}

        //By definition, this is a scope-local construct. So moving/copying it makes no sense.
        Defer() = delete;
        Defer(const Defer&) = delete;
        Defer(Defer&&) = delete;
        Defer& operator=(const Defer&) = delete;
        Defer& operator=(Defer&&) = delete;

        ~Defer() noexcept(noexcept(cleanup)) {
            std::invoke(cleanup);
        }
    };

}

/*
*   In the macro case, we want to be able to generate implicit Defer instances with unique names.
*   As such we use __COUNTER__ if it's a available and __LINE__ if not, and concat each one such that
*   the macro generates classes called Defer_Struct1, Defer_Struct2, Defer_Struct3. Unique but still
*   understandable and diagnosable if needed.
*/
#define DEFER_CONCAT_IMPL(x,y) x##y
#define DEFER_CONCAT_MACRO( x, y ) DEFER_CONCAT_IMPL( x, y )

#ifdef __COUNTER__
#define DEFER_COUNT __COUNTER__
#else
#define DEFER_COUNT __LINE__
#endif

#define DEFER(ARGS) [[maybe_unused]] auto DEFER_CONCAT_MACRO(Defer_Struct, DEFER_COUNT) = dp::Defer([&](){ARGS ;});



#endif