#ifndef INX_FRAC_HPP_INCLUDED
#define INX_FRAC_HPP_INCLUDED

#include "inx.hpp"
// #include <boost/integer/common_factor_rt.hpp>

namespace inx {

struct frac256;

template <typename T, typename Enabled = std::enable_if_t<std::is_integral<T>::value>>
struct frac
{
	using self = frac<T, Enabled>;
	template <typename T2>
	using promote_t = std::conditional_t<std::is_unsigned<T>::value && std::is_unsigned<T2>::value, uint64, int64>;

	T a, b;
	frac() noexcept = default;
	constexpr frac(const self&) noexcept = default;
	constexpr frac(self&&) noexcept = default;
	constexpr frac(T n) noexcept : a(n), b(1) { }
	constexpr frac(T n, T d) noexcept : a(n), b(d) { }
	template <typename T2>
	constexpr frac(const frac<T2>& o) : a(static_cast<T>(o.a)), b(static_cast<T>(o.b)) { }

	constexpr self& operator=(const self&) = default;
	constexpr self& operator=(self&&) = default;

	// constexpr self normalise() const noexcept
	// {
	// 	if (b == 0) return {std::clamp(a, -1, 1), 0};
	// 	T r = boost::integer::gcd(a, b);
	// 	if constexpr (std::is_signed<T>::value) if (b < 0) r = -r;
	// 	return {a/r, b/r};
	// }
	constexpr bool isnan() const noexcept
	{
		return a == 0 && b == 0;
	}
	constexpr bool isinf() const noexcept
	{
		return b == 0 && a != 0;
	}

	constexpr self inv() const noexcept
	{
		return {b, a};
	}
	constexpr self operator-() const noexcept
	{
		return {-a, b};
	}

	template <typename CT, typename = std::enable_if_t<std::is_integral<CT>::value>>
	explicit constexpr operator frac<CT>() const noexcept { return frac<CT>(static_cast<CT>(a), static_cast<CT>(b)); }

	static constexpr self nan() noexcept { return {0, 0}; }
	static constexpr self inf() noexcept { return {1, 0}; }
	static constexpr self zero() noexcept { return {0, 1}; }
};

#define GMLIB_FRAC_GEN(type,op,code) \
template <typename T1, typename T2> \
constexpr type operator op(frac<T1>& lhs, const frac<T2>& rhs) noexcept \
{ code }\
template <typename T1, typename T2> \
constexpr std::enable_if_t<std::is_integral<T2>::value, type> operator op(frac<T1>& lhs, T2 rhs) noexcept \
{ return lhs op frac<T2>(rhs); }

#define GMLIB_FRAC_GEN_CONST(type,op,code) \
template <typename T1, typename T2> \
constexpr type operator op(const frac<T1>& lhs, const frac<T2>& rhs) noexcept \
{ code }\
template <typename T1, typename T2> \
constexpr std::enable_if_t<std::is_integral<T2>::value, type> operator op(const frac<T1>& lhs, T2 rhs) noexcept \
{ return lhs op frac<T2>(rhs); }\
template <typename T1, typename T2> \
constexpr std::enable_if_t<std::is_integral<T1>::value, type> operator op(T1 lhs, const frac<T2>& rhs) noexcept \
{ return frac<T1>(lhs) op rhs; }

GMLIB_FRAC_GEN(frac<T1>&,+=,lhs.a = lhs.a * rhs.b + rhs.a * lhs.b; lhs.b *= rhs.b; return lhs;)
GMLIB_FRAC_GEN_CONST(frac<T1>,+,auto r = frac<T1>(lhs); return r += rhs;)
GMLIB_FRAC_GEN(frac<T1>&,-=,return lhs += -rhs;)
GMLIB_FRAC_GEN_CONST(frac<T1>,-,return lhs + -rhs;)
GMLIB_FRAC_GEN(frac<T1>&,*=,lhs.a *= rhs.a; lhs.b *= rhs.b; return lhs;)
GMLIB_FRAC_GEN_CONST(frac<T1>,*,auto r = frac<T1>(lhs); return r *= rhs;)
GMLIB_FRAC_GEN(frac<T1>&,/=,return lhs *= rhs.inv();)
GMLIB_FRAC_GEN_CONST(frac<T1>,/,return lhs * rhs.inv();)
GMLIB_FRAC_GEN_CONST(bool,==,return static_cast<typename frac<T1>::template promote_t<T2>>(lhs.a) * rhs.b == static_cast<typename frac<T1>::template promote_t<T2>>(rhs.a) * lhs.b;)
GMLIB_FRAC_GEN_CONST(bool,!=,return static_cast<typename frac<T1>::template promote_t<T2>>(lhs.a) * rhs.b != static_cast<typename frac<T1>::template promote_t<T2>>(rhs.a) * lhs.b;)
GMLIB_FRAC_GEN_CONST(bool,<, return static_cast<typename frac<T1>::template promote_t<T2>>(lhs.a) * rhs.b <  static_cast<typename frac<T1>::template promote_t<T2>>(rhs.a) * lhs.b;)
GMLIB_FRAC_GEN_CONST(bool,>, return static_cast<typename frac<T1>::template promote_t<T2>>(lhs.a) * rhs.b >  static_cast<typename frac<T1>::template promote_t<T2>>(rhs.a) * lhs.b;)
GMLIB_FRAC_GEN_CONST(bool,<=,return static_cast<typename frac<T1>::template promote_t<T2>>(lhs.a) * rhs.b <= static_cast<typename frac<T1>::template promote_t<T2>>(rhs.a) * lhs.b;)
GMLIB_FRAC_GEN_CONST(bool,>=,return static_cast<typename frac<T1>::template promote_t<T2>>(lhs.a) * rhs.b >= static_cast<typename frac<T1>::template promote_t<T2>>(rhs.a) * lhs.b;)

#undef GMLIB_FRAC_GEN
#undef GMLIB_FRAC_GEN_CONST

} // namespace inx

#endif // INX_FRAC_HPP_INCLUDED
