#ifndef FINALLY_H //! FINALLY_H
#define FINALLY_H

#include <type_traits>

template <typename callable_t>
class __FinallyCaller
{
public:
    __FinallyCaller(callable_t callable)
        : m_callable(std::move(callable))
    {
    }

    ~__FinallyCaller() noexcept
    {
        m_callable();
    }

private:
    callable_t m_callable;
};

struct __Helper
{
    template <typename callable_t>
    constexpr __FinallyCaller<callable_t> operator+(const callable_t &&callable)
    {
        static_assert(noexcept(callable), "The finally block cannot throw");

        return {callable};
    }
};

#define _MAKELAMBDA(x, y) x##y
#define MAKELAMBDA(x, y) _MAKELAMBDA(x, y)
#define finally auto &&MAKELAMBDA(lambda, __COUNTER__) = __Helper{} + [&]() noexcept

#endif //! FINALLY_H