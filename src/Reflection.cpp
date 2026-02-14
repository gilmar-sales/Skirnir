export module Skirnir:Reflection;

import std;

export namespace skr
{

#if defined(__GNUC__) && !defined(__clang__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wnon-template-friend"
#endif

    // tag<T, N> generates friend declarations and helps with overload
    // resolution. There are two types: one with the auto return type, which is
    // the way we read types later. The second one is used in the detection of
    // instantiations without which we'd get multiple definitions.
    template <typename T, int N>
    struct tag
    {
        inline friend auto          loophole(tag<T, N>);
        inline constexpr friend int cloophole(tag<T, N>);
    };

    // The definitions of friend functions.
    template <typename T, typename U, int N, bool B,
              typename = typename std::enable_if_t<!std::is_same_v<
                  std::remove_cv_t<std::remove_reference_t<T>>,
                  std::remove_cv_t<std::remove_reference_t<U>>>>>
    struct fn_def
    {
        inline friend auto          loophole(tag<T, N>) { return U {}; }
        inline constexpr friend int cloophole(tag<T, N>) { return 0; }
    };

    // This specialization is to avoid multiple definition errors.
    template <typename T, typename U, int N>
    struct fn_def<T, U, N, true>
    {
    };

    // This has a templated conversion operator which in turn triggers
    // instantiations. Important point, using sizeof seems to be more reliable.
    // Also default template arguments are "cached" (I think). To fix that I
    // provide a U template parameter to the ins functions which do the
    // detection using constexpr friend functions and SFINAE.
    template <typename T, int N>
    struct c_op
    {
        template <typename U, int M>
        static auto ins(...) -> int;
        template <typename U, int M, int = cloophole(tag<T, M> {})>
        static auto ins(int) -> char;

        template <
            typename U,
            int = sizeof(fn_def<T, U, N, sizeof(ins<U, N>(0)) == sizeof(char)>)>
        operator U();
    };

    // Here we detect the data type field number. The byproduct is
    // instantiations. Uses list initialization. Won't work for types with
    // user-provided constructors. In C++17 there is std::is_aggregate which can
    // be added later.
    template <typename T, int... Ns>
    constexpr int fields_number(...)
    {
        return sizeof...(Ns) - 1;
    }

    template <typename T, int... Ns>
    constexpr auto fields_number(int) -> decltype(T { c_op<T, Ns> {}... }, 0)
    {
        return fields_number<T, Ns..., sizeof...(Ns)>(0);
    }
    
    // Here is a version of fields_number to handle user-provided ctor.
    // NOTE: It finds the first ctor having the shortest unambigious set
    //       of parameters.
    template <typename T, int... Ns>
    constexpr auto fields_number_ctor(int) -> decltype(T(c_op<T, Ns> {}...), 0)
    {
        return sizeof...(Ns);
    }

    template <typename T, int... Ns>
    constexpr int fields_number_ctor(...)
    {
        return fields_number_ctor<T, Ns..., sizeof...(Ns)>(0);
    }

    // This is a helper to turn a ctor into a tuple type.
    // Usage is: refl::as_tuple<data_t>
    template <typename T, typename U>
    struct loophole_tuple;

    template <typename T, int... Ns>
    struct loophole_tuple<T, std::integer_sequence<int, Ns...>>
    {
        using type = std::tuple<decltype(loophole(tag<T, Ns> {}))...>;
    };

    template <typename T>
    using get_constructor_args = typename loophole_tuple<
        T, std::make_integer_sequence<int, fields_number_ctor<T>(0)>>::type;

#if defined(__GNUC__) && !defined(__clang__)
    #pragma GCC diagnostic pop
#endif

    template <typename T>
    constexpr char* type_name()
    {
#if defined(__clang__)
        // __PRETTY_FUNCTION__ on Clang/GCC looks like:
        //   std::string type_name() [with T = MyNamespace::MyType]
        static std::string pretty = __PRETTY_FUNCTION__;
        static auto        start  = pretty.find("T = ") + 4;
        static auto        end    = pretty.rfind(']');
        static auto        result = pretty.substr(start, end - start);
        return result.data();
#elif defined(__GNUC__)
        // __PRETTY_FUNCTION__ on Clang/GCC looks like:
        //   std::string type_name() [with T = MyNamespace::MyType]
        static std::string pretty = __PRETTY_FUNCTION__;
        static auto        start  = pretty.find("T = ") + 4;
        static auto        end = std::min(pretty.rfind(']'), pretty.rfind(';'));
        static auto        result = pretty.substr(start, end - start);
        return result.data();
#elif defined(_MSC_VER)
        // __FUNCSIG__ on MSVC looks like:
        //   std::string __cdecl type_name<TYPE>(void)
        static std::string pretty = __FUNCSIG__;
        static auto        start  = pretty.find("type_name<") + 16;
        static auto        end    = pretty.rfind(">(void)");
        static auto        result = pretty.substr(start, end - start);
        return result.data();
#else
        return "Unknown Compiler";
#endif
    }

    template <typename T>
    struct callable_traits;

    template <typename Ret, typename... Args>
    struct callable_traits<Ret (*)(Args...)>
    {
        using return_type = Ret;
        using args_tuple = std::tuple<Args...>;
        static constexpr std::size_t arity = sizeof...(Args);
        static constexpr bool is_const = false;
        static constexpr bool is_noexcept = false;
        static constexpr bool is_member = false;
    };

    template <typename Ret, typename... Args>
    struct callable_traits<Ret (*)(Args...) noexcept>
    {
        using return_type = Ret;
        using args_tuple = std::tuple<Args...>;
        static constexpr std::size_t arity = sizeof...(Args);
        static constexpr bool is_const = false;
        static constexpr bool is_noexcept = true;
        static constexpr bool is_member = false;
    };

    template <typename Class, typename Ret, typename... Args>
    struct callable_traits<Ret (Class::*)(Args...)>
    {
        using class_type = Class;
        using return_type = Ret;
        using args_tuple = std::tuple<Args...>;
        static constexpr std::size_t arity = sizeof...(Args);
        static constexpr bool is_const = false;
        static constexpr bool is_noexcept = false;
        static constexpr bool is_member = true;
    };

    template <typename Class, typename Ret, typename... Args>
    struct callable_traits<Ret (Class::*)(Args...) const>
    {
        using class_type = Class;
        using return_type = Ret;
        using args_tuple = std::tuple<Args...>;
        static constexpr std::size_t arity = sizeof...(Args);
        static constexpr bool is_const = true;
        static constexpr bool is_noexcept = false;
        static constexpr bool is_member = true;
    };

    template <typename Class, typename Ret, typename... Args>
    struct callable_traits<Ret (Class::*)(Args...) volatile>
    {
        using class_type = Class;
        using return_type = Ret;
        using args_tuple = std::tuple<Args...>;
        static constexpr std::size_t arity = sizeof...(Args);
        static constexpr bool is_const = false;
        static constexpr bool is_noexcept = false;
        static constexpr bool is_member = true;
    };

    template <typename Class, typename Ret, typename... Args>
    struct callable_traits<Ret (Class::*)(Args...) const volatile>
    {
        using class_type = Class;
        using return_type = Ret;
        using args_tuple = std::tuple<Args...>;
        static constexpr std::size_t arity = sizeof...(Args);
        static constexpr bool is_const = true;
        static constexpr bool is_noexcept = false;
        static constexpr bool is_member = true;
    };

    template <typename Class, typename Ret, typename... Args>
    struct callable_traits<Ret (Class::*)(Args...) noexcept>
    {
        using class_type = Class;
        using return_type = Ret;
        using args_tuple = std::tuple<Args...>;
        static constexpr std::size_t arity = sizeof...(Args);
        static constexpr bool is_const = false;
        static constexpr bool is_noexcept = true;
        static constexpr bool is_member = true;
    };

    template <typename Class, typename Ret, typename... Args>
    struct callable_traits<Ret (Class::*)(Args...) const noexcept>
    {
        using class_type = Class;
        using return_type = Ret;
        using args_tuple = std::tuple<Args...>;
        static constexpr std::size_t arity = sizeof...(Args);
        static constexpr bool is_const = true;
        static constexpr bool is_noexcept = true;
        static constexpr bool is_member = true;
    };

    template <typename Class, typename Ret, typename... Args>
    struct callable_traits<Ret (Class::*)(Args...) volatile noexcept>
    {
        using class_type = Class;
        using return_type = Ret;
        using args_tuple = std::tuple<Args...>;
        static constexpr std::size_t arity = sizeof...(Args);
        static constexpr bool is_const = false;
        static constexpr bool is_noexcept = true;
        static constexpr bool is_member = true;
    };

    template <typename Class, typename Ret, typename... Args>
    struct callable_traits<Ret (Class::*)(Args...) const volatile noexcept>
    {
        using class_type = Class;
        using return_type = Ret;
        using args_tuple = std::tuple<Args...>;
        static constexpr std::size_t arity = sizeof...(Args);
        static constexpr bool is_const = true;
        static constexpr bool is_noexcept = true;
        static constexpr bool is_member = true;
    };

    template <typename Class, typename Ret, typename... Args>
    struct callable_traits<Ret (Class::*)(Args...) &>
    {
        using class_type = Class;
        using return_type = Ret;
        using args_tuple = std::tuple<Args...>;
        static constexpr std::size_t arity = sizeof...(Args);
        static constexpr bool is_const = false;
        static constexpr bool is_noexcept = false;
        static constexpr bool is_member = true;
    };

    template <typename Class, typename Ret, typename... Args>
    struct callable_traits<Ret (Class::*)(Args...) const &>
    {
        using class_type = Class;
        using return_type = Ret;
        using args_tuple = std::tuple<Args...>;
        static constexpr std::size_t arity = sizeof...(Args);
        static constexpr bool is_const = true;
        static constexpr bool is_noexcept = false;
        static constexpr bool is_member = true;
    };

    template <typename Class, typename Ret, typename... Args>
    struct callable_traits<Ret (Class::*)(Args...) &&>
    {
        using class_type = Class;
        using return_type = Ret;
        using args_tuple = std::tuple<Args...>;
        static constexpr std::size_t arity = sizeof...(Args);
        static constexpr bool is_const = false;
        static constexpr bool is_noexcept = false;
        static constexpr bool is_member = true;
    };

    template <typename Class, typename Ret, typename... Args>
    struct callable_traits<Ret (Class::*)(Args...) const &&>
    {
        using class_type = Class;
        using return_type = Ret;
        using args_tuple = std::tuple<Args...>;
        static constexpr std::size_t arity = sizeof...(Args);
        static constexpr bool is_const = true;
        static constexpr bool is_noexcept = false;
        static constexpr bool is_member = true;
    };

    template <typename Class, typename Ret, typename... Args>
    struct callable_traits<Ret (Class::*)(Args...) & noexcept>
    {
        using class_type = Class;
        using return_type = Ret;
        using args_tuple = std::tuple<Args...>;
        static constexpr std::size_t arity = sizeof...(Args);
        static constexpr bool is_const = false;
        static constexpr bool is_noexcept = true;
        static constexpr bool is_member = true;
    };

    template <typename Class, typename Ret, typename... Args>
    struct callable_traits<Ret (Class::*)(Args...) const & noexcept>
    {
        using class_type = Class;
        using return_type = Ret;
        using args_tuple = std::tuple<Args...>;
        static constexpr std::size_t arity = sizeof...(Args);
        static constexpr bool is_const = true;
        static constexpr bool is_noexcept = true;
        static constexpr bool is_member = true;
    };

    template <typename Class, typename Ret, typename... Args>
    struct callable_traits<Ret (Class::*)(Args...) && noexcept>
    {
        using class_type = Class;
        using return_type = Ret;
        using args_tuple = std::tuple<Args...>;
        static constexpr std::size_t arity = sizeof...(Args);
        static constexpr bool is_const = false;
        static constexpr bool is_noexcept = true;
        static constexpr bool is_member = true;
    };

    template <typename Class, typename Ret, typename... Args>
    struct callable_traits<Ret (Class::*)(Args...) const && noexcept>
    {
        using class_type = Class;
        using return_type = Ret;
        using args_tuple = std::tuple<Args...>;
        static constexpr std::size_t arity = sizeof...(Args);
        static constexpr bool is_const = true;
        static constexpr bool is_noexcept = true;
        static constexpr bool is_member = true;
    };

    template <typename T>
    concept has_call_operator = requires {
        &T::operator();
    };

    template <has_call_operator T>
    struct callable_traits<T> : callable_traits<decltype(&T::operator())>
    {
    };

    template <typename Ret, typename... Args>
    struct callable_traits<Ret(Args...)>
    {
        using return_type = Ret;
        using args_tuple = std::tuple<Args...>;
        static constexpr std::size_t arity = sizeof...(Args);
        static constexpr bool is_const = false;
        static constexpr bool is_noexcept = false;
        static constexpr bool is_member = false;
    };

    template <typename Ret, typename... Args>
    struct callable_traits<Ret(Args...) noexcept>
    {
        using return_type = Ret;
        using args_tuple = std::tuple<Args...>;
        static constexpr std::size_t arity = sizeof...(Args);
        static constexpr bool is_const = false;
        static constexpr bool is_noexcept = true;
        static constexpr bool is_member = false;
    };

    template <typename T>
    using callable_return = typename callable_traits<T>::return_type;

    template <typename T>
    using callable_args = typename callable_traits<T>::args_tuple;

    template <typename T>
    inline constexpr std::size_t callable_arity = callable_traits<T>::arity;

    template <typename T>
    inline constexpr bool is_const_callable = callable_traits<T>::is_const;

    template <typename T>
    inline constexpr bool is_noexcept_callable = callable_traits<T>::is_noexcept;

    template <typename T, std::size_t N>
    using callable_arg = std::tuple_element_t<N, callable_args<T>>;
} // namespace skr
