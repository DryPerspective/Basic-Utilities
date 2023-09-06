#ifndef DP_COROUTINE_HANDLES
#define DP_COROUTINE_HANDLES


#include <coroutine>
#include <utility>
#include <exception>
#include <variant>

namespace dp {

    //To preserve some forwarding and while fighting with templates' propensity to need to be absolutely exact about types and matching, we need to invent this C++23 feature a little early
    //I did consider shifting this into a module, but implementation is still in its infancy and this is a unique C++20 header in a C++17 lib.
    //Perhaps I'll shift it to the traits header. Time will tell.
    namespace detail {
        template<typename T, typename U>
        constexpr auto&& forward_like(U&& x) noexcept
        {
            constexpr bool add_const = std::is_const_v<std::remove_reference_t<T>>;
            if constexpr (std::is_lvalue_reference_v<T&&>)
            {
                if constexpr (add_const)
                    return std::as_const(x);
                else
                    return static_cast<U&>(x);
            }
            else
            {
                if constexpr (add_const)
                    return std::move(std::as_const(x));
                else
                    return std::move(x);
            }
        }

        template<typename T, typename U>
        concept not_same_as = !std::same_as<T, U>;
    }




    /*
    *   A common base for all promise classes, with the shared functionality needed for
    *   various types of coro; with the goal being not to limit possible uses.
    *   As such a reasonable amount of boilerplate will still be needed, but that's just where coroutines are right now.
    */
    template<typename T> requires detail::not_same_as<std::exception_ptr, T> 
    struct promise_base {

        constexpr promise_base() : m_HoldsValue{ false }, m_current{ nullptr } {}

        //Logically we should not simultaneously need an active return type and an active exception. exception_ptr first because it's probably cheaper to default-construct
        std::variant<std::exception_ptr, T> m_current;
        //We need to distinguish whether the coroutine currently holds an "active" value, or whether it is empty.
        bool m_HoldsValue;

        constexpr auto unhandled_exception() -> void {
            m_current = std::current_exception();
            set_holds_value();
        }

        constexpr auto rethrow_if_exception() noexcept(false) -> void {
            if (std::holds_alternative<std::exception_ptr>(m_current)) std::rethrow_exception(std::get<std::exception_ptr>(m_current));
        }

        constexpr auto holds_value() const noexcept -> bool {
            return m_HoldsValue;
        }

        constexpr auto set_holds_value(bool val = true) noexcept -> void {
            m_HoldsValue = val;
        }

        constexpr auto get() -> T& {

        //It may seem unconventional to not wrap this in a check against m_HoldsValue. That's because it is.
        //One unfortunate consequence of playing tennis between promises and handles is that we cannot eliminate
        //some responsibility on the part of the handle to interact nicely with the promise.
        //It is the handle's responsibility to let us know it's taken a value out of the promise and to update holds_value accordingly,
        //not the promise's responsibility to assume all access will leave us without a value.

            rethrow_if_exception();
            return std::get<T>(m_current);
        }

        template<std::convertible_to<T> ValType>
        constexpr auto set_value(ValType&& val) -> void {
            //Templates allow deduction which allows for good forwarding, but it also allows ValType and T to mismatch, which variant does not like
            //Unfortunately we need a cast; but to preserve forwarding we need to invent std::forward_like a standard early.
            m_current = detail::forward_like<ValType>(static_cast<T>(val));
            set_holds_value();
        }

    };

    /*
    *  A "Generator" style coroutine handle, intended for coroutines which progressively generate and co_yield new values.
    *  Complete with a basic iterator interface intended for range-for loops.
    */
    template<typename T>
    class Generator {
    public:
        struct promise_type;
        using handle_type = std::coroutine_handle<promise_type>;

        explicit constexpr Generator(handle_type coro) : m_coro{ coro } {}
        constexpr ~Generator() {
            if (m_coro) m_coro.destroy();
        }

        Generator(const Generator&) = delete;
        Generator& operator=(const Generator&) = delete;

        constexpr Generator(Generator&& in) noexcept : m_coro{ in.m_coro } {}
        constexpr Generator& operator=(Generator&& in) noexcept = default;

        struct promise_type : public promise_base<T> {

            constexpr auto get_return_object() -> Generator {
                return Generator{ handle_type::from_promise(*this) };
            }
            constexpr auto initial_suspend() noexcept -> std::suspend_always { return {}; }
            constexpr auto final_suspend() noexcept -> std::suspend_always { return {}; }

            template<std::convertible_to<T> ValType>
            constexpr auto yield_value(ValType&& val) -> std::suspend_always {
                this->set_value(std::forward<ValType>(val));
                return {};
            }

            constexpr auto return_void() -> void {}
        };

        constexpr auto operator()() -> T {  //NB: Return by value. References between the coro, through the promise and the handle, are a recipe for trouble

            if (!m_coro.promise().holds_value()) {
                m_coro.resume();
            }

            //We're extracting a value from the coro in the return statement, so we set it to false.
            m_coro.promise().set_holds_value(false);
            return std::move(m_coro.promise().get());
            
        }

        constexpr auto done() -> bool {
            /*
            * If we don't presently hold a value, the only real way to know if there's going to be a next value is to run the coroutine and see if we get one.
            */
            if (!m_coro.promise().holds_value()) {
                m_coro.resume();
                m_coro.promise().rethrow_if_exception();
            }

            return m_coro.done();
        }

        class iterator {
            handle_type m_coro;

        public:
            explicit constexpr iterator(const handle_type& coro) : m_coro{ coro } {}

            constexpr auto operator++() -> void {
                m_coro.resume();
            }


            constexpr auto operator *() -> T {        //Ditto to Generator::Operator(), we return by value as we move the data out of the coroutine
                m_coro.promise().set_holds_value(false);
                return std::move(m_coro.promise().get());
            }

            constexpr auto operator==(std::default_sentinel_t) const -> bool {
                return !m_coro || m_coro.done();
            }

        };

        constexpr auto begin() const -> iterator {
            if (m_coro) m_coro.resume();
            return iterator{ m_coro };
        }

        constexpr auto end() const -> std::default_sentinel_t {
            return {};
        }

    private:
        handle_type m_coro;

    };


    /*
    *  A "lazy initializer" class, to co_return a single particular value
    */
    template<typename T>
    class Lazy {
    public:
        struct promise_type;
        using handle_type = std::coroutine_handle<promise_type>;

        struct promise_type : promise_base<T> {
            constexpr auto get_return_object() -> Lazy {
                return Lazy{ handle_type::from_promise(*this) };
            }

            constexpr auto initial_suspend() noexcept -> std::suspend_always { return {}; }
            constexpr auto final_suspend() noexcept -> std::suspend_always { return {}; }

            template<std::convertible_to<T> ValType>
            constexpr auto return_value(ValType&& val) -> void {
                this->set_value(std::forward<ValType>(val));
            }

        };

        explicit constexpr Lazy(handle_type coro) : m_coro{ coro } {}
        constexpr ~Lazy() {
            if (m_coro) m_coro.destroy();
        }

        //I did debate whether to make this explicit, but I think implicit conversions will probably be the preferable option in user code.
        constexpr operator const T&() {  
            return this->get();
        }

        constexpr auto get() -> const T& {

            if (!m_coro.promise().holds_value()) {
                m_coro.resume();
            }

            //Unlike a generator, a lazy coroutine will only generate one final value. As such, we do not remove the value from the promise when we retrieve it
            return m_coro.promise().get();

        }

        constexpr auto done() -> bool {

            if (!m_coro.promise().holds_value()) {
                m_coro.resume();
                m_coro.promise().rethrow_if_exception();
            }

            return m_coro.done();
        }

    private:

        handle_type m_coro;


    };

}

#endif