#ifndef INX_POINT_HPP_INCLUDED
#define INX_POINT_HPP_INCLUDED

#include "inx.hpp"
#include "frac.hpp"
#include <numeric>


namespace inx {

enum class Dir : int8
{
	CW = -1, // clockwise
	COLIN = 0, // doubles as line-string
	CCW = 1, // counter-clockwise
	FWD, // forward/same direction/0 deg
	BACK, // backward/opposide direction/180 deg
	INV // invalid, point on origion, has no angle
};

enum class Between : int8
{
	NARROW = -1,
	WHOLE = 0,
	WIDE = 1
};

inline constexpr Dir inv_dir(Dir d) noexcept
{
	assert(d == Dir::CW || d == Dir::CCW);
	return d == Dir::CW ? Dir::CCW : Dir::CW;
}

template <typename CoordType>
struct Point;

namespace details {

template <typename Cd>
union PointAlign
{
	struct {
		Cd x, y;
	} c;
};
template <>
union PointAlign<int8>
{
	struct {
		int8 x, y;
	} c;
	int16 p;
	PointAlign(int8 x, int8 y) : c{x, y} { }
	PointAlign(int16 lp) : p(lp) { }
};
template <>
union PointAlign<int16>
{
	struct {
		int16 x, y;
	} c;
	int32 p;
	PointAlign(int16 x, int16 y) : c{x, y} { }
	PointAlign(int32 lp) : p(lp) { }
};
template <>
union PointAlign<int32>
{
	struct {
		int32 x, y;
	} c;
	int64 p;
	PointAlign(int32 x, int32 y) : c{x, y} { }
	PointAlign(int64 lp) : p(lp) { }
};

template <typename Type, typename Enabled = void>
struct point_traits;

template <typename Type>
struct point_traits<Type, std::enable_if_t<std::is_integral_v<Type>>>
{
	// type, -1 is int, 0 is float, 1 is fix float
	static_assert(std::is_signed_v<Type>, "Type must be a signed integer");
	using unsigned_type = std::make_unsigned_t<Type>;
	using result_type = std::conditional_t<sizeof(Type)<=2, int32, int64>;
	using long_result_type = int64;
	using scale_type = frac<result_type>;

	constexpr static int type() noexcept { return -1; }
	constexpr static result_type epsilon() noexcept { return 0; }
	constexpr static long_result_type long_epsilon() noexcept { return 0; }
	constexpr static scale_type scale_inf() noexcept { return scale_type(1, 0); }
	constexpr static scale_type scale_ninf() noexcept { return scale_type(-1, 0); }
};

template <typename Type>
struct point_traits<Type, std::enable_if_t<std::is_floating_point_v<Type>>>
{
	// type, -1 is int, 0 is float, 1 is fix float
	using unsigned_type = Type;
	using result_type = std::conditional_t<std::is_same_v<Type, long double>, long double, double>;
	using long_result_type = long double;
	using scale_type = result_type;

	constexpr static int type() noexcept { return 0; }
	constexpr static result_type epsilon() noexcept { return inx::epsilon<result_type>; }
	constexpr static long_result_type long_epsilon() noexcept { return inx::epsilon<long_result_type>; }
	constexpr static scale_type scale_inf() noexcept { return inx::inf<result_type>; }
	constexpr static scale_type scale_ninf() noexcept { return -inx::inf<result_type>; }
};

}

template <typename CoordType>
struct alignas(details::PointAlign<CoordType>) Point
{
	using self = Point<CoordType>;
	static_assert(std::is_same_v<std::decay_t<CoordType>, CoordType>, "CoordType must not be reference or const");
	using coord_type = CoordType;
	static constexpr bool is_floating_point() noexcept { return details::point_traits<coord_type>::type() == 0; }
	static constexpr bool is_integral() noexcept { return details::point_traits<coord_type>::type() == -1; }
	using unsigned_type = typename details::point_traits<coord_type>::unsigned_type;
	using result_type = typename details::point_traits<coord_type>::result_type;
	using long_result_type = typename details::point_traits<coord_type>::long_result_type;
	using scale_type = typename details::point_traits<coord_type>::scale_type;
	coord_type x;
	coord_type y;
	Point() noexcept = default;
	constexpr Point(const self&) noexcept = default;
	constexpr Point(self&&) noexcept = default;
	constexpr explicit Point(Point c[2]) noexcept : x(c[0]), y(c[1])
	{ }
	constexpr Point(CoordType lX, CoordType lY) noexcept : x(lX), y(lY)
	{ }
	template <typename T, typename = std::enable_if_t<!std::is_same_v<CoordType, T> && (!is_integral() || std::is_integral_v<T>)>>
	constexpr Point(T lX, T lY) noexcept : x(static_cast<CoordType>(lX)), y(static_cast<CoordType>(lY))
	{ }
	template <typename T, typename = std::enable_if_t<!std::is_same_v<CoordType, T> && (!is_integral() || std::is_integral_v<T>)>>
	constexpr Point(const Point<T>& lO) noexcept : Point(lO.x, lO.y)
	{ }
	~Point() noexcept = default;

	template <typename C, typename Enable = std::enable_if_t<!std::is_same_v<CoordType, C>>>
	explicit constexpr operator Point<C>() const noexcept { return Point<C>(x, y); }

	bool isIntegerX() const noexcept
	{
		if constexpr (is_integral()) {
			return true;
		} else {
			return std::abs(x - std::round(x)) < pos_epsilon();
		}
	}
	bool isIntegerY() const noexcept
	{
		if constexpr (is_integral()) {
			return true;
		} else {
			return std::abs(y - std::round(y)) < pos_epsilon();
		}
	}
	bool isInteger() const noexcept
	{
		if constexpr (is_integral()) {
			return true;
		} else {
			return isIntegerX() && isIntegerY();
		}
	}
	
	constexpr self& operator=(const self&) noexcept = default;
	constexpr self& operator=(self&&) noexcept = default;
	template <typename C>
	constexpr self& operator=(const Point<C>& c) noexcept
	{
		x = static_cast<coord_type>(c.x);
		y = static_cast<coord_type>(c.y);
		return *this;
	}

	constexpr self normalise() const noexcept
	{
		if constexpr (is_integral()) {
			if (isZero())
				return self::zero();
			CoordType g = std::gcd(static_cast<CoordType>(x), static_cast<CoordType>(y));
			return self(x / g, y / g);
		} else if constexpr (is_floating_point()) {
			if (isZero())
				return self::zero();
			double len = length();
			return self(x / len, y / len);
		} else {
			assert(false);
			return *this;
		}
	}

	constexpr Point<result_type> pair_mult(self a) const noexcept
	{
		return {static_cast<result_type>(x) * static_cast<result_type>(a.x), static_cast<result_type>(y) * static_cast<result_type>(a.y)};
	}

	static constexpr self zero() noexcept { return self(0,0); }
	static constexpr result_type pos_epsilon() noexcept { return details::point_traits<coord_type>::epsilon(); }
	static constexpr result_type neg_epsilon() noexcept { return -details::point_traits<coord_type>::epsilon(); }
	static constexpr scale_type scale_inf() noexcept { return details::point_traits<coord_type>::scale_inf(); }
	static constexpr scale_type scale_ninf() noexcept { return details::point_traits<coord_type>::scale_ninf(); }
	static constexpr result_type high_epsilon() noexcept { return details::point_traits<coord_type>::high_epsilon(); }

	constexpr bool isZero() const noexcept
	{
		if constexpr (is_integral()) {
			if constexpr (std::is_same_v<coord_type, int8>) {
				return *reinterpret_cast<const int16*>(this) == 0;
			} else if constexpr (std::is_same_v<coord_type, int16>) {
				return *reinterpret_cast<const int32*>(this) == 0;
			} else if constexpr (std::is_same_v<coord_type, int32>) {
				return *reinterpret_cast<const int64*>(this) == 0;
			} else {
				return (x|y) == 0;
			}
		} else {
			return square() < (2.0*(pos_epsilon()*pos_epsilon()));
		}
	}

	constexpr coord_type& operator[](std::size_t idx) noexcept
	{
		assert(idx < 2);
		return reinterpret_cast<coord_type*>(this)[idx];
	}
	constexpr coord_type operator[](std::size_t idx) const noexcept
	{
		assert(idx < 2);
		return reinterpret_cast<const coord_type*>(this)[idx];
	}

	template <size_t I>
	constexpr coord_type& get() noexcept
	{
		static_assert(I < 2, "I must be 0 (x) or 1 (y)");
		if constexpr (I == 0) {
			return x;
		} else {
			return y;
		}
	}
	template <size_t I>
	constexpr coord_type get() const noexcept
	{
		static_assert(I < 2, "I must be 0 (x) or 1 (y)");
		if constexpr (I == 0) {
			return x;
		} else {
			return y;
		}
	}

	static constexpr bool isColin(result_type v) noexcept
	{
		if constexpr (is_floating_point())
			return std::abs(v) < pos_epsilon();
		else
			return v == 0;
	}
	static constexpr bool isCW(result_type v) noexcept
	{
		return v < neg_epsilon();
	}
	static constexpr bool isCCW(result_type v) noexcept
	{
		return v > pos_epsilon();
	}
	static constexpr bool isFwd(result_type x, result_type y) noexcept
	{
		if constexpr (is_integral())
			return (x|y) > 0;
		else
			return x > pos_epsilon() || y > pos_epsilon();
	}
	static constexpr bool isBack(result_type x, result_type y) noexcept
	{
		if constexpr (is_integral())
			return (x|y) < 0;
		else
			return x < neg_epsilon() || y < neg_epsilon();
	}

	template <Dir D>
	static bool isDir(result_type v) noexcept
	{
		static_assert(D == Dir::COLIN || D == Dir::CW || D == Dir::CCW, "D must be COLIN, CW or CCW");
		if constexpr (D == Dir::CW)
			return isCW(v);
		else if constexpr (D == Dir::CCW)
			return isCCW(v);
		else
			return isColin(v);
	}

	// static constexpr bool isHighColin(result_type v) noexcept
	// {
	// 	if (is_floating_point())
	// 		return std::abs(v) < high_epsilon();
	// 	else
	// 		return v == 0;
	// }
	// static constexpr bool isHighCW(result_type v) noexcept
	// {
	// 	return v < -high_epsilon();
	// }
	// static constexpr bool isHighCCW(result_type v) noexcept
	// {
	// 	return v > high_epsilon();
	// }
	// static constexpr bool isHighFwd(result_type x, result_type y) noexcept
	// {
	// 	if constexpr (is_integral())
	// 		return (x|y) > 0;
	// 	else
	// 		return x > high_epsilon() || y > high_epsilon();
	// }
	// static constexpr bool isHighBack(result_type x, result_type y) noexcept
	// {
	// 	if constexpr (is_integral())
	// 		return (x|y) < 0;
	// 	else
	// 		return x < -high_epsilon() || y < -high_epsilon();
	// }
	// template <Dir D>
	// static bool isHighDir(result_type v) noexcept
	// {
	// 	static_assert(D == Dir::COLIN || D == Dir::CW || D == Dir::CCW, "D must be COLIN, CW or CCW");
	// 	if constexpr (D == Dir::CW)
	// 		return isHighCW(v);
	// 	else if constexpr (D == Dir::CCW)
	// 		return isHighCCW(v);
	// 	else
	// 		return isHighColin(v);
	// }

	template <typename... T>
	static constexpr bool isConjunctiveColin(T... v) noexcept
	{
		if constexpr (is_integral() && std::conjunction_v<std::is_integral<T>...>)
			return (v | ...) == 0;
		else
			return (isColin(v) && ...);
	}
	template <typename... T>
	static constexpr bool isConjunctiveNotColin(T... v) noexcept
	{
		if constexpr (is_integral() && std::conjunction_v<std::is_integral<T>...>)
			return ((v|-v) & ...) < 0;
		else
			return (!isColin(v) && ...);
	}
	template <typename... T>
	static constexpr bool  isConjunctiveCW(T... v) noexcept
	{
		if constexpr (is_integral() && std::conjunction_v<std::is_integral<T>...>)
			return (v & ...) < 0;
		else
			return (isCW(v) && ...);
	}
	template <typename... T>
	static constexpr bool isConjunctiveNotCW(T... v) noexcept
	{
		if constexpr (is_integral() && std::conjunction_v<std::is_integral<T>...>)
			return (v | ...) >= 0;
		else
			return (!isCW(v) && ...);
	}
	template <typename... T>
	static constexpr bool isConjunctiveCCW(T... v) noexcept
	{
		if constexpr (is_integral() && std::conjunction_v<std::is_integral<T>...>)
			return (-v & ...) < 0;
		else
			return (isCCW(v) && ...);
	}
	template <typename... T>
	static constexpr bool isConjunctiveNotCCW(T... v) noexcept
	{
		if constexpr (is_integral() && std::conjunction_v<std::is_integral<T>...>)
			return (-v | ...) >= 0;
		else
			return (!isCCW(v) && ...);
	}
	template <Dir D, typename... T>
	static constexpr bool isConjunctiveDir(T... v) noexcept
	{
		static_assert(D == Dir::CW || D == Dir::CCW || D == Dir::COLIN, "D must be CW, CCW or COLIN");
		if constexpr (D == Dir::CW)
			return isConjunctiveCW(v...);
		else if constexpr (D == Dir::CCW)
			return isConjunctiveCCW(v...);
		else
			return isConjunctiveColin(v...);
	}
	template <Dir D, typename... T>
	static constexpr bool isConjunctiveNotDir(T... v) noexcept
	{
		static_assert(D == Dir::CW || D == Dir::CCW || D == Dir::COLIN, "D must be CW, CCW or COLIN");
		if constexpr (D == Dir::CW)
			return isConjunctiveNotCW(v...);
		else if constexpr (D == Dir::CCW)
			return isConjunctiveNotCCW(v...);
		else
			return isConjunctiveNotColin(v...);
	}

	template <typename... T>
	static constexpr bool isDisjunctiveColin(T... v) noexcept
	{
		if constexpr (is_integral() && std::conjunction_v<std::is_integral<T>...>)
			return ((v|-v) & ...) == 0;
		else
			return (isColin(v) || ...);
	}
	template <typename... T>
	static constexpr bool isDisjunctiveNotColin(T... v) noexcept
	{
		if constexpr (is_integral() && std::conjunction_v<std::is_integral<T>...>)
			return (v | ...) != 0;
		else
			return (!isColin(v) || ...);
	}
	template <typename... T>
	static constexpr bool isDisjunctiveCW(T... v) noexcept
	{
		if constexpr (is_integral() && std::conjunction_v<std::is_integral<T>...>)
			return (v | ...) < 0;
		else
			return (isCW(v) || ...);
	}
	template <typename... T>
	static constexpr bool isDisjunctiveNotCW(T... v) noexcept
	{
		if constexpr (is_integral() && std::conjunction_v<std::is_integral<T>...>)
			return (v & ...) >= 0;
		else
			return (!isCW(v) || ...);
	}
	template <typename... T>
	static constexpr bool isDisjunctiveCCW(T... v) noexcept
	{
		if constexpr (is_integral() && std::conjunction_v<std::is_integral<T>...>)
			return (-v | ...) < 0;
		else
			return (isCCW(v) || ...);
	}
	template <typename... T>
	static constexpr bool isDisjunctiveNotCCW(T... v) noexcept
	{
		if constexpr (is_integral() && std::conjunction_v<std::is_integral<T>...>)
			return (-v & ...) >= 0;
		else
			return (!isCCW(v) || ...);
	}
	template <Dir D, typename... T>
	static bool isDisjunctiveDir(T... v) noexcept
	{
		static_assert(D == Dir::CW || D == Dir::CCW || D == Dir::COLIN, "D must be CW, CCW or COLIN");
		if constexpr (D == Dir::CW)
			return isDisjunctiveCW(v...);
		else if constexpr (D == Dir::CCW)
			return isDisjunctiveCCW(v...);
		else
			return isDisjunctiveColin(v...);
	}
	template <Dir D, typename... T>
	static constexpr std::enable_if_t<is_integral() && std::conjunction_v<std::is_integral<T>...>, bool> isDisjunctiveNotDir(T... v) noexcept
	{
		static_assert(D == Dir::CW || D == Dir::CCW || D == Dir::COLIN, "D must be CW, CCW or COLIN");
		if constexpr (D == Dir::CW)
			return isDisjunctiveNotCW(v...);
		else if constexpr (D == Dir::CCW)
			return isDisjunctiveNotCCW(v...);
		else
			return isDisjunctiveNotColin(v...);
	}

	double length() const
	{
		return std::sqrt(square());
	}
	double distance(self b) const
	{
		return (b - *this).length();
	}
	constexpr result_type square() const
	{
		const result_type x1(x), y1(y);
		return static_cast<result_type>(x1 * x1 + y1 * y1);
	}
	constexpr result_type square(self b) const
	{
		return (b - *this).square();
	}

	constexpr result_type cross(self a) const noexcept
	{
		return static_cast<result_type>(static_cast<result_type>(x) * static_cast<result_type>(a.y) - static_cast<result_type>(y) * static_cast<result_type>(a.x));
	}
	constexpr result_type cross(self a, self b) const noexcept
	{
		return (a - *this).cross(b - *this); 
	}
	constexpr bool isCW(self a) const noexcept
	{
		return isCW(cross(a));
	}
	constexpr bool isCW(self a, self b) const noexcept
	{
		return (a - *this).isCW(b - *this);
	}
	constexpr bool isCCW(self a) const noexcept
	{
		return isCCW(cross(a));
	}
	constexpr bool isCCW(self a, self b) const noexcept
	{
		return (a - *this).isCCW(b - *this);
	}
	constexpr bool isColin(self a) const noexcept
	{
		return isColin(cross(a));
	}
	constexpr bool isColin(self a, self b) const noexcept
	{
		return (a - *this).isColin(b - *this);
	}
	constexpr bool isFwd(self a) const noexcept
	{
		assert(isColin(a));
		auto p = pair_mult(a);
		return isFwd(p.x, p.y);
	}
	constexpr bool isFwd(self a, self b) const noexcept
	{
		return (a - *this).isFwd(b - *this);
	}
	constexpr bool isColinFwd(self a) const noexcept
	{
		return isColin(a) && isFwd(a);
	}
	constexpr bool isColinFwd(self a, self b) const noexcept
	{
		return (a - *this).isColinFwd(b - *this);
	}
	constexpr bool isBack(self a) const noexcept
	{
		assert(isColin(a));
		auto p = pair_mult(a);
		return isBack(p.x, p.y);
	}
	constexpr bool isBack(self a, self b) const noexcept
	{
		return (a - *this).isBack(b - *this);
	}
	constexpr bool isColinBack(self a) const noexcept
	{
		return isColin(a) && isBack(a);
	}
	constexpr bool isColinBack(self a, self b) const noexcept
	{
		return (a - *this).isColinBack(b - *this);
	}
	template <Dir D>
	constexpr bool isDir(self a) const noexcept
	{
		static_assert(D != Dir::INV, "D must be either CW, CCW or COLIN, FWD, BACK");
		if constexpr (D == Dir::CW)
			return isCW(a);
		else if constexpr (D == Dir::CCW)
			return isCCW(a);
		else if constexpr (D == Dir::COLIN)
			return isColin(a);
		else if constexpr (D == Dir::FWD) {
			return isFwd(a);
		} else if constexpr (D == Dir::BACK) {
			return isBack(a);
		}
	}
	template <Dir D>
	constexpr bool isDir(self a, self b) const noexcept
	{
		return (a - *this).template isDir<D>(b - *this);
	}
	template <Dir D>
	constexpr bool isDirx(self a) const noexcept
	{
		static_assert(D != Dir::COLIN, "D must be either CW, CCW or COLIN, FWD, BACK");
		if constexpr (D == Dir::CW)
			return isCW(a);
		else if constexpr (D == Dir::CCW)
			return isCCW(a);
		else if constexpr (D == Dir::FWD) {
			return isColinFwd(a);
		} else if constexpr (D == Dir::BACK) {
			return isColinBack(a);
		} else if constexpr (D == Dir::INV) {
			return isZero() || a.isZero();
		}
	}
	template <Dir D>
	constexpr bool isDirx(self a, self b) const noexcept
	{
		return (a - *this).template isDirx<D>(b - *this);
	}
	constexpr Dir dirx(self a) const noexcept
	{
		if (result_type d = cross(a); isCW(d))
			return Dir::CW;   // clockwise
		else if (isCCW(d))
			return Dir::CCW;  // counter clockwise
		else if (auto dx = pair_mult(a);
			self::isFwd(dx.x, dx.y))
			return Dir::FWD;  // colinear points on same side from origin
		else if (self::isBack(dx.x, dx.y))
			return Dir::BACK; // colinear points on opposite side from origin
		else
			return Dir::INV;  // at least one point on the origin
	}
	constexpr Dir dirx(self a, self b) const noexcept
	{
		return (a - *this).dirx(b - *this); 
	}
	constexpr Dir dir(self a) const noexcept
	{
		if (result_type d = cross(a); isCW(d))
			return Dir::CW;
		else if (isCCW(d))
			return Dir::CCW;
		else
			return Dir::COLIN;
	}
	constexpr Dir dir(self a, self b) const noexcept
	{
		return (a - *this).dir(b - *this); 
	}

	// constexpr Dir highDirx(self a) const noexcept
	// {
	// 	if (result_type d = cross(a); isHighCW(d))
	// 		return Dir::CW;   // clockwise
	// 	else if (isHighCCW(d))
	// 		return Dir::CCW;  // counter clockwise
	// 	else if (auto dx = pair_mult(a);
	// 		self::isHighFwd(dx.x, dx.y))
	// 		return Dir::FWD;  // colinear points on same side from origin
	// 	else if (self::isHighBack(dx.x, dx.y))
	// 		return Dir::BACK; // colinear points on opposite side from origin
	// 	else
	// 		return Dir::INV;  // at least one point on the origin
	// }
	// constexpr Dir highDirx(self a, self b) const noexcept
	// {
	// 	return (a - *this).highDirx(b - *this); 
	// }
	// constexpr Dir highDir(self a) const noexcept
	// {
	// 	if (result_type d = cross(a); isHighCW(d))
	// 		return Dir::CW;
	// 	else if (isHighCCW(d))
	// 		return Dir::CCW;
	// 	else
	// 		return Dir::COLIN;
	// }
	// constexpr Dir highDir(self a, self b) const noexcept
	// {
	// 	return (a - *this).highDir(b - *this); 
	// }

	constexpr self turn90CCW() const noexcept
	{
		return self(-y, x);
	}
	constexpr self turn90CW() const noexcept
	{
		return self(y, -x);
	}
	template <Dir D>
	constexpr self turn90() const noexcept
	{
		static_assert(D == Dir::CW || D == Dir::CCW, "D must be CW or CCW");
		if constexpr (D == Dir::CW)
			return turn90CW();
		else
			return turn90CCW();
	}

	constexpr bool isLeftOf(self a) const noexcept
	{
		return isCW(a);
	}
	constexpr bool isRightOf(self a) const noexcept
	{
		return isCCW(a);
	}

	// a and b must be CCW, checks if b is between
	constexpr bool isNarrowBetweenCCW(self a, self b) const noexcept
	{
		assert(!a.isCW(b));
		if constexpr (is_integral())
			return isConjunctiveCCW(a.cross(*this), cross(b));
		else
			return a.isCCW(*this) && b.isCW(*this);
	}
	constexpr bool isNarrowBetweenCCW(self o, self a, self b) const noexcept
	{
		return isNarrowBetweenCCW(a - o, b - o);
	}
	constexpr bool isNarrowBetweenCW(self a, self b) const noexcept
	{
		assert(!a.isCCW(b));
		if constexpr (is_integral())
			return isConjunctiveCW(a.cross(*this), cross(b));
		else
			return a.isCW(*this) && b.isCCW(*this);
	}
	constexpr bool isNarrowBetweenCW(self o, self a, self b) const noexcept
	{
		return isNarrowBetweenCW(a - o, b - o);
	}
	template <Dir D>
	constexpr bool isNarrowBetween(self a, self b) const noexcept
	{
		static_assert(D == Dir::CW || D == Dir::CCW, "D must be either CW or CCW");
		if constexpr (D == Dir::CW)
			return isNarrowBetweenCW(a, b);
		else
			return isNarrowBetweenCCW(a, b);
	}
	template <Dir D>
	constexpr bool isNarrowBetween(self o, self a, self b) const noexcept
	{
		static_assert(D == Dir::CW || D == Dir::CCW, "D must be either CW or CCW");
		if constexpr (D == Dir::CW)
			return isNarrowBetweenCW(o, a, b);
		else
			return isNarrowBetweenCCW(o, a, b);
	}

	// wide between
	constexpr bool isWideBetweenCCW(self a, self b) const noexcept
	{
		assert(!a.isCCW(b));
		if constexpr (is_integral())
			return isDisjunctiveCCW(a.cross(*this), cross(b));
		else
			return a.isCCW(*this) || b.isCW(*this);
	}
	constexpr bool isWideBetweenCCW(self o, self a, self b) const noexcept
	{
		return isWideBetweenCCW(a - o, b - o);
	}
	constexpr bool isWideBetweenCW(self a, self b) const noexcept
	{
		assert(!a.isCW(b));
		if constexpr (is_integral())
			return isDisjunctiveCW(a.cross(*this), cross(b));
		else
			return a.isCW(*this) || b.isCCW(*this);
	}
	constexpr bool isWideBetweenCW(self o, self a, self b) const noexcept
	{
		return isWideBetweenCW(a - o, b - o);
	}
	template <Dir D>
	constexpr bool isWideBetween(self a, self b) const noexcept
	{
		static_assert(D == Dir::CW || D == Dir::CCW, "D must be either CW or CCW");
		if constexpr (D == Dir::CW)
			return isWideBetweenCW(a, b);
		else
			return isWideBetweenCCW(a, b);
	}
	template <Dir D>
	constexpr bool isWideBetween(self o, self a, self b) const noexcept
	{
		static_assert(D == Dir::CW || D == Dir::CCW, "D must be either CW or CCW");
		if constexpr (D == Dir::CW)
			return isWideBetweenCW(o, a, b);
		else
			return isWideBetweenCCW(o, a, b);
	}
	
	constexpr bool isBetweenCCW(self a, self b) const noexcept
	{
		if (!a.isCW(b))
			return isNarrowBetweenCCW(a, b);
		else
			return isWideBetweenCCW(a, b);
	}
	constexpr bool isBetweenCCW(self o, self a, self b) const noexcept
	{
		return isBetweenCCW(a - o, b - o);
	}
	constexpr bool isBetweenCW(self a, self b) const noexcept
	{
		if (!a.isCCW(b))
			return isNarrowBetweenCW(a, b);
		else
			return isWideBetweenCW(a, b);
	}
	constexpr bool isBetweenCW(self o, self a, self b) const noexcept
	{
		return isBetweenCW(a - o, b - o);
	}
	template <Between B, Dir D>
	constexpr bool isBetween(self a, self b) const noexcept
	{
		static_assert(D == Dir::CW || D == Dir::CCW, "D must be either CW or CCW");
		if constexpr (B == Between::WHOLE) {
			if constexpr (D == Dir::CW)
				return isBetweenCW(a, b);
			else
				return isBetweenCCW(a, b);
		} else if constexpr (B == Between::NARROW) {
			if constexpr (D == Dir::CW)
				return isNarrowBetweenCW(a, b);
			else
				return isNarrowBetweenCCW(a, b);
		} else {
			if constexpr (D == Dir::CW)
				return isWideBetweenCW(a, b);
			else
				return isWideBetweenCCW(a, b);
		}
	}
	template <Between B, Dir D>
	constexpr bool isBetween(self o, self a, self b) const noexcept
	{
		return isBetween<B, D>(a - o, b - o);
	}

	// if a and b are on opposite sides of the line from origin
	constexpr bool isOpposites(self a, self b) const noexcept
	{
		if constexpr (is_integral() && sizeof(CoordType) <= 2) {
			return cross(a) * cross(b) < 0;
		} else {
			if (auto c1 = cross(a); isColin(c1))
				return false;
			else if (auto c2 = cross(b); isCCW(c1))
				return isCW(c2);
			else
				return isCCW(c2);
		}
	}
	// if a and b are on opposite sides of the line from origin
	constexpr bool isNotOpposites(self a, self b) const noexcept
	{
		if constexpr (sizeof(CoordType) < 2) {
			return cross(a) * cross(b) > 0;
		} else {
			if (auto c1 = cross(a); isColin(c1))
				return false;
			else if (auto c2 = cross(b); isCCW(c1))
				return isCCW(c2);
			else
				return isCW(c2);
		}
	}
};

template <typename C1, typename C2>
constexpr Point<C1> operator+=(Point<C1>& a, Point<C2> b) noexcept
{
	a.x += static_cast<C1>(b.x); a.y += static_cast<C1>(b.y);
	return a;
}
template <typename CT>
constexpr Point<CT> operator+(Point<CT> a, Point<CT> b) noexcept
{
	return Point<CT>(a.x + b.x, a.y + b.y);
}

template <typename CT>
constexpr Point<CT> operator-(Point<CT> a) noexcept
{
	return Point<CT>(-a.x, -a.y);
}

template <typename C1, typename C2>
constexpr Point<C1> operator-=(Point<C1>& a, Point<C2> b) noexcept
{
	a.x -= static_cast<C1>(b.x); a.y -= static_cast<C1>(b.y);
	return a;
}
template <typename CT>
constexpr Point<CT> operator-(Point<CT> a, Point<CT> b) noexcept
{
	return Point<CT>(a.x - b.x, a.y - b.y);
}

template <typename CT>
constexpr bool operator==(Point<CT> a, Point<CT> b) noexcept
{
	if constexpr (Point<CT>::is_integral()) {
		if constexpr (inx::is_same_any_v<CT, int8, int16, int32>) {
			return details::PointAlign<CT>(a.x, a.y).p == details::PointAlign<CT>(b.x, b.y).p;
		} else {
			return ( (a.x-b.x) | (a.y-b.y) ) == 0;
		}
	} else {
		return (a-b).isZero();
	}
}
template <typename CT>
constexpr bool operator!=(Point<CT> a, Point<CT> b) noexcept
{
	return !(a == b);
}

template <typename CT>
constexpr bool operator<(Point<CT> a, Point<CT> b) noexcept
{
	return a.x != b.x ? a.x < b.x : a.y < b.y;
}
template <typename CT>
constexpr bool operator>(Point<CT> a, Point<CT> b) noexcept
{
	return b < a;
}
template <typename CT>
constexpr bool operator<=(Point<CT> a, Point<CT> b) noexcept
{
	return !(b < a);
}
template <typename CT>
constexpr bool operator>=(Point<CT> a, Point<CT> b) noexcept
{
	return !(a < b);
}

template <typename CT>
constexpr auto operator*(Point<CT> a, Point<CT> b) noexcept
{
	using result_type = typename Point<CT>::result_type;
	return static_cast<result_type>(static_cast<result_type>(a.x) * static_cast<result_type>(b.x) + static_cast<result_type>(a.y) * static_cast<result_type>(b.y));
}

template <typename T, typename CT>
constexpr auto operator*(T a, Point<CT> b) noexcept
{
	return Point<CT>(a * b.x, a * b.y);
}
template <typename T, typename CT>
constexpr auto operator*(Point<CT> a, T b) noexcept
{
	return Point<CT>(a.x * b, a.y * b);
}

template <typename CT>
constexpr Point<CT> multiply_scale(Point<CT> a, typename Point<CT>::scale_type scale)
{
	if constexpr (Point<CT>::is_integral()) {
		using result_type = typename Point<CT>::result_type;
		assert(!scale.isnan() && !scale.isinf());
		return Point<CT>(static_cast<CT>(scale.a * static_cast<result_type>(a.x) / scale.b), static_cast<CT>(scale.a * static_cast<result_type>(a.y) / scale.b));
	} else {
		return Point<CT>(scale * a.x, scale * a.y);
	}
}

template <bool XLess, bool YLess, typename CT>
constexpr bool strict_order(const Point<CT>& a, const Point<CT>& b) noexcept
{
	if constexpr (XLess && YLess) {
		return a.x < b.x + Point<CT>::neg_epsilon() ? true : a.x <= b.x + Point<CT>::pos_epsilon() ? a.y < b.y + Point<CT>::neg_epsilon() : false;
	} else if constexpr (XLess && !YLess) {
		return a.x < b.x + Point<CT>::neg_epsilon() ? true : a.x <= b.x + Point<CT>::pos_epsilon() ? a.y > b.y + Point<CT>::pos_epsilon() : false;
	} else if constexpr (!XLess && YLess) {
		return a.x > b.x + Point<CT>::pos_epsilon() ? true : a.x >= b.x + Point<CT>::neg_epsilon() ? a.y < b.y + Point<CT>::neg_epsilon() : false;
	} else {
		return a.x > b.x + Point<CT>::pos_epsilon() ? true : a.x >= b.x + Point<CT>::neg_epsilon() ? a.y > b.y + Point<CT>::pos_epsilon() : false;
	}
}

template <bool YFirst, bool XLess, bool YLess, typename CT>
constexpr bool strict_order_adv(const Point<CT>& a, const Point<CT>& b) noexcept
{
	if constexpr (!YFirst) {
		if constexpr (XLess && YLess) {
			return a.x < b.x + Point<CT>::neg_epsilon() ? true : a.x <= b.x + Point<CT>::pos_epsilon() ? a.y < b.y + Point<CT>::neg_epsilon() : false;
		} else if constexpr (XLess && !YLess) {
			return a.x < b.x + Point<CT>::neg_epsilon() ? true : a.x <= b.x + Point<CT>::pos_epsilon() ? a.y > b.y + Point<CT>::pos_epsilon() : false;
		} else if constexpr (!XLess && YLess) {
			return a.x > b.x + Point<CT>::pos_epsilon() ? true : a.x >= b.x + Point<CT>::neg_epsilon() ? a.y < b.y + Point<CT>::neg_epsilon() : false;
		} else {
			return a.x > b.x + Point<CT>::pos_epsilon() ? true : a.x >= b.x + Point<CT>::neg_epsilon() ? a.y > b.y + Point<CT>::pos_epsilon() : false;
		}
	} else {
		if constexpr (XLess && YLess) {
			return a.y < b.y + Point<CT>::neg_epsilon() ? true : a.y <= b.y + Point<CT>::pos_epsilon() ? a.x < b.x + Point<CT>::neg_epsilon() : false;
		} else if constexpr (!XLess && YLess) {
			return a.y < b.y + Point<CT>::neg_epsilon() ? true : a.y <= b.y + Point<CT>::pos_epsilon() ? a.x > b.x + Point<CT>::pos_epsilon() : false;
		} else if constexpr (XLess && !YLess) {
			return a.y > b.y + Point<CT>::pos_epsilon() ? true : a.y >= b.y + Point<CT>::neg_epsilon() ? a.x < b.x + Point<CT>::neg_epsilon() : false;
		} else {
			return a.y > b.y + Point<CT>::pos_epsilon() ? true : a.y >= b.y + Point<CT>::neg_epsilon() ? a.x > b.x + Point<CT>::pos_epsilon() : false;
		}
	}
}

enum class PRop : uint8
{
	eqZero = 1 << 3,
	ltZero = 2 << 3,
	gtZero = 3 << 3,
	eqOne = 4 << 3,
	ltOne = 5 << 3,
	gtOne = 6 << 3,
	rangeInc = 7 << 3,
	rangeIncExc = 8 << 3,

	neZero = eqZero | 1,
	leZero = gtZero | 1,
	geZero = ltZero | 1,
	neOne = eqOne | 1,
	leOne = gtOne | 1,
	geOne = ltOne | 1,
	rangeExc = rangeInc | 1,
	rangeExcInc = rangeIncExc | 1,
};
constexpr PRop pr_flip(PRop x) noexcept
{
	return static_cast<PRop>(static_cast<int>(x) ^ 1);
}
constexpr PRop pr_higheps(PRop x) noexcept
{
	return static_cast<PRop>( (static_cast<int>(x) & ~(0b0110)) | 0b0010 );
}
constexpr PRop pr_loweps(PRop x) noexcept
{
	return static_cast<PRop>( (static_cast<int>(x) & ~(0b0110)) | 0b0100 );
}
constexpr PRop pr_vloweps(PRop x) noexcept
{
	return static_cast<PRop>( static_cast<int>(x) | 0b0110 );
}
constexpr PRop pr_mideps(PRop x) noexcept
{
	return static_cast<PRop>( static_cast<int>(x) & ~(0b0110) );
}
template <typename T>
constexpr T pr_eps(PRop x) noexcept
{
	return (static_cast<int>(x) & 0b0100) != 0 ? 
		 ( (static_cast<int>(x) & 0b0010) != 0 ? inx::very_low_epsilon<T> : inx::low_epsilon<T> ) :
		 ( (static_cast<int>(x) & 0b0010) != 0 ? inx::high_epsilon<T> : inx::epsilon<T> );
}

template <PRop Op, typename T>
constexpr std::enable_if_t<std::is_floating_point_v<T>, bool> pr_op(T x) noexcept
{
	constexpr PRop ROp = pr_mideps(Op);
	constexpr T eps = pr_eps<T>(Op);
	if constexpr (ROp == PRop::eqZero) return std::abs(x) < eps;
	else if constexpr (ROp == PRop::ltZero) return x < -eps;
	else if constexpr (ROp == PRop::gtZero) return x > eps;
	else if constexpr (ROp == PRop::eqOne) return std::abs(x-1) < eps;
	else if constexpr (ROp == PRop::ltOne) return x < 1-eps;
	else if constexpr (ROp == PRop::gtOne) return x > 1+eps;
	else if constexpr (ROp == PRop::rangeInc) return -eps < x && x < 1+eps;
	else if constexpr (ROp == PRop::rangeExc) return eps < x && x < 1-eps;
	else if constexpr (ROp == PRop::rangeIncExc) return -eps < x && x < 1-eps;
	else if constexpr (ROp == PRop::rangeExcInc) return eps < x && x < 1+eps;
	else return !pr_op<pr_flip(Op)>(x);
}

template < PRop Op, typename T>
constexpr std::enable_if_t<std::is_integral_v<T>, bool> pr_op(T x, T y[[maybe_unused]]) noexcept
{
	assert(y != 0); // 0/0 is undefined
	constexpr PRop ROp = pr_mideps(Op);
	if constexpr (ROp == PRop::eqZero) return x == 0;
	else if constexpr (ROp == PRop::ltZero) return x < 0;
	else if constexpr (ROp == PRop::gtZero) return x > 0;
	else if constexpr (ROp == PRop::eqOne) return x == y;
	else if constexpr (ROp == PRop::ltOne) return x < y;
	else if constexpr (ROp == PRop::gtOne) return x > y;
	else if constexpr (ROp == PRop::rangeInc) return static_cast<std::make_unsigned_t<T>>(x) <= static_cast<std::make_unsigned_t<T>>(y);
	else if constexpr (ROp == PRop::rangeExc) return static_cast<std::make_unsigned_t<T>>(x-1) < static_cast<std::make_unsigned_t<T>>(y-1);
	else if constexpr (ROp == PRop::rangeIncExc) return static_cast<std::make_unsigned_t<T>>(x) < static_cast<std::make_unsigned_t<T>>(y);
	else if constexpr (ROp == PRop::rangeExcInc) return static_cast<std::make_unsigned_t<T>>(x-1) <= static_cast<std::make_unsigned_t<T>>(y-1);
	else return !pr_op<pr_flip(Op)>(x, y);
}

template < PRop Op, typename T>
constexpr std::enable_if_t<std::is_floating_point_v<T>, bool> pr_op(T x, T y) noexcept
{
	assert(std::abs(y) > Point<T>::pos_epsilon()); // /0 not accepted
	return pr_op<Op>(x / y);
}

template <PRop Op, typename T>
constexpr bool pr_op(frac<T> x) noexcept
{
	return pr_op<Op>(x.a, x.b);
}

enum class PCop : uint8
{
	lt,
	le,
	gt,
	ge,
	eq,
	ne,
};

template <PCop Op, typename T>
constexpr std::enable_if_t<std::is_floating_point_v<T>, bool> pc_op(T x, T y) noexcept
{
	using pt = Point<T>;
	if constexpr (Op == PCop::lt) return x - y < pt::neg_epsilon();
	else if constexpr (Op == PCop::le) return x - y <= pt::pos_epsilon();
	else if constexpr (Op == PCop::gt) return x - y > pt::pos_epsilon();
	else if constexpr (Op == PCop::ge) return x - y >= pt::neg_epsilon();
	else if constexpr (Op == PCop::eq) return std::abs(x - y) < pt::pos_epsilon();
	else if constexpr (Op == PCop::ne) return std::abs(x - y) >= pt::pos_epsilon();
}
template <PCop Op, typename T>
constexpr std::enable_if_t<!std::is_floating_point_v<T>, bool> pc_op(T x, T y) noexcept
{
	if constexpr (Op == PCop::lt) return x < y;
	else if constexpr (Op == PCop::le) return x <= y;
	else if constexpr (Op == PCop::gt) return x > y;
	else if constexpr (Op == PCop::ge) return x >= y;
	else if constexpr (Op == PCop::eq) return x == y;
	else if constexpr (Op == PCop::ne) return x != y;
}
// template <PCop Op, typename T>
// constexpr std::enable_if_t<!std::is_floating_point_v<T>, bool> pc_op(frac<T> x, frac<T> y) noexcept
// {
// 	if constexpr (Op == PCop::lt) return x < y;
// 	else if constexpr (Op == PCop::le) return x <= y;
// 	else if constexpr (Op == PCop::gt) return x > y;
// 	else if constexpr (Op == PCop::ge) return x >= y;
// 	else if constexpr (Op == PCop::eq) return x == y;
// 	else if constexpr (Op == PCop::ne) return x != y;
// }

template <typename T>
struct Intersect
{
	using pt = Point<T>;
	using result_type = std::conditional_t<pt::is_integral(), int32, double>;
	using scale_type = std::conditional_t<pt::is_integral(), frac<int32>, double>;
	result_type scale, a, b;
	Intersect() = default;
	Intersect(result_type s) : scale(s) { }
	Intersect(result_type s, result_type x, result_type y) : scale(s), a(x), b(y) { }
	bool colin() const noexcept { return scale == 0; }
	bool parallel() const noexcept { return a != 0; }
	bool intersect() const noexcept
	{
		if constexpr (pt::is_integral())
			return !colin() && pr_op<PRop::rangeInc>(a, scale) && pr_op<PRop::rangeInc>(b, scale);
		else
			return !colin() && pr_op<PRop::rangeInc>(a / scale) && pr_op<PRop::rangeInc>(b / scale);
	}
	// !colin assumed
	bool rangeAinc() const noexcept
	{
		assert(!colin());
		if constexpr (pt::is_integral())
			return pr_op<PRop::rangeInc>(a, scale);
		else
			return pr_op<PRop::rangeInc>(a / scale);
	}
	bool rangeAexc() const noexcept
	{
		assert(!colin());
		if constexpr (pt::is_integral())
			return pr_op<PRop::rangeExc>(a, scale);
		else
			return pr_op<PRop::rangeExc>(a / scale);
	}
	bool rangeBinc() const noexcept
	{
		assert(!colin());
		if constexpr (pt::is_integral())
			return pr_op<PRop::rangeInc>(b, scale);
		else
			return pr_op<PRop::rangeInc>(b / scale);
	}
	bool rangeBexc() const noexcept
	{
		assert(!colin());
		if constexpr (pt::is_integral())
			return pr_op<PRop::rangeExc>(b, scale);
		else
			return pr_op<PRop::rangeExc>(b / scale);
	}

	scale_type scaleA() const noexcept
	{
		if constexpr (pt::is_integral())
			return scale_type(a, scale);
		else
			return a / scale;
	}
	scale_type scaleB() const noexcept
	{
		if constexpr (pt::is_integral())
			return scale_type(b, scale);
		else
			return b / scale;
	}

	void norm() noexcept
	{
		if (scale == 0)
			a = b = 1;
		else {
			if (scale < 0) {
				scale = -scale;
				a = -a;
				b = -b;
			}
			if constexpr (pt::is_integral()) {
				int32 g = std::gcd(std::gcd(a, b), scale);
				scale /= g;
				a /= g;
				b /= g;
			}
		}
	}
};

template <typename T>
inline bool operator==(const Intersect<T>& a, const Intersect<T>& b) noexcept
{
	if constexpr (std::is_integral_v<T>) {
		auto ax = a;
		ax.norm();
		auto bx = b;
		bx.norm();
		return ax.scale == bx.scale && ax.a == bx.a && ax.b == bx.b;
	} else {
		return inx::is_zero(a.scale - b.scale) && inx::is_zero(a.a - b.a) && inx::is_zero(a.b - b.b);
	}
}
template <typename T>
inline bool operator!=(const Intersect<T>& a, const Intersect<T>& b) noexcept
{
	return !(a == b);
}

// https://stackoverflow.com/questions/563198/how-do-you-detect-where-two-line-segments-intersect
template <typename T>
Intersect<T> segment_intersect(Point<T> a, Point<T> av, Point<T> b, Point<T> bv) noexcept
{
	using pt = Point<T>;
	if constexpr (pt::is_integral()) {
		auto scale = av.cross(bv);
		if (scale == 0)
			return Intersect<T>(0, av.cross(b - a), 0);
		auto ab = b - a;
		if (scale < 0)
			return Intersect<T>(-scale, -ab.cross(bv), -ab.cross(av));
		else
			return Intersect<T>(scale, ab.cross(bv), ab.cross(av));
	} else {
		auto scale = av.cross(bv);
		if (is_zero(scale)) {
			auto par = av.cross(b - a);
			if (par < pt::neg_epsilon())
				par = -1;
			else if (par < pt::pos_epsilon())
				par = 0;
			else
				par = 1;
			return Intersect<T>(0, par, 0);
		} else {
			auto ab = b - a;
			return Intersect<T>(scale, ab.cross(bv), ab.cross(av));
		}
	}
}

template <typename T>
typename Point<T>::scale_type noncollinear_segment_intersect_dist_a(Point<T> a, Point<T> av, Point<T> b, Point<T> bv) noexcept
{
	static_assert(Point<T>::is_integral(), "T must be integral");
	assert(!av.isColin(bv));
	return frac<typename Point<T>::result_type>((b - a).cross(bv), av.cross(bv));
}

template <typename T>
typename Point<T>::scale_type collinear_point_on_segment(Point<T> x, Point<T> av) noexcept
{
	assert(!av.isZero() && av.isColin(x));
	if constexpr (Point<T>::is_integral()) {
		return typename Point<T>::scale_type(x * av, av * av);
	} else {
		return static_cast<T>((x * av) / (av * av));
	}
}
template <typename T>
typename Point<T>::scale_type collinear_point_on_segment(Point<T> x, Point<T> a, Point<T> av) noexcept
{
	return collinear_point_on_segment(x-a, av);
}

template <typename T>
typename Point<T>::scale_type near_collinear_point_on_segment(Point<T> x, Point<T> av) noexcept
{
	assert(!av.isZero());
	if constexpr (Point<T>::is_integral()) {
		return typename Point<T>::scale_type(x * av, av * av);
	} else {
		return static_cast<T>((x * av) / (av * av));
	}
}
template <typename T>
typename Point<T>::scale_type near_collinear_point_on_segment(Point<T> x, Point<T> a, Point<T> av) noexcept
{
	return near_collinear_point_on_segment(x-a, av);
}

template <typename T>
bool is_collinear_point_on_segment(Point<T> x, Point<T> av) noexcept
{
	assert(!av.isZero() && av.isColin(x));
	if constexpr (Point<T>::is_integral()) {
		return pr_op<PRop::rangeInc>(x*av, av*av);
	} else {
		return pr_op<PRop::rangeInc>(collinear_point_on_segment(x, av));
	}
}
template <typename T>
bool is_collinear_point_on_segment(Point<T> x, Point<T> a, Point<T> av) noexcept
{
	return is_collinear_point_on_segment(x-a, av);
}

template <typename T>
bool is_point_on_segment(Point<T> x, Point<T> a, Point<T> av) noexcept
{
	x -= a;
	return x.isColin(av) && is_collinear_point_on_segment(x, av);
}

template <bool EndTouch, typename T>
bool collinear_segment_overlap(Point<T> a, Point<T> ax, Point<T> b, Point<T> bx) noexcept
{
	assert(!ax.isZero() && !bx.isZero() && ax.isColin(bx) && ax.isColin(b-a)); // must be collinear
	if constexpr (EndTouch) {
		if (auto fx = collinear_point_on_segment(a, b, bx); pr_op<PRop::rangeInc>(fx))
			return true;
		if (auto fx = collinear_point_on_segment(a + ax, b, bx); pr_op<PRop::rangeInc>(fx))
			return true;
	} else {
		if (auto fx = collinear_point_on_segment(a, b, bx); pr_op<PRop::rangeExc>(fx))
			return true;
		else if (auto fy = collinear_point_on_segment(a + ax, b, bx); pr_op<PRop::rangeExc>(fy))
			return true;
		else if ( (pr_op<PRop::eqZero>(fx) && pr_op<PRop::eqOne>(fy)) ||
				(pr_op<PRop::eqZero>(fy) && pr_op<PRop::eqOne>(fx)) )
			return true;
	}
	return false;
}

template <typename T>
auto point_to_line_factor(Point<T> p, Point<T> a, Point<T> b) noexcept
{
	using pt = Point<T>;
	if constexpr (pt::is_integral()) {
		assert(std::abs(p.x) < (static_cast<int64>(1)<<14) && std::abs(p.y) < (static_cast<int64>(1)<<14));
		assert(std::abs(a.x) < (static_cast<int64>(1)<<14) && std::abs(a.y) < (static_cast<int64>(1)<<14));
		assert(std::abs(b.x) < (static_cast<int64>(1)<<14) && std::abs(b.y) < (static_cast<int64>(1)<<14));
		auto x = a - p, y = b - p;

		int64 n = x.cross(y);
		Point<int64> d(x); d -= y;
		
		return pt::scale_type(n*n, d*d);
	} else {
		auto x = a - p, y = b - p;
		auto n = x.cross(y);
		x -= y;
		return (n*n) / (x*x);
	}
}

template <typename T>
constexpr inline std::size_t hash_value(Point<T> val) noexcept
{
	static_assert(Point<T>::is_integral(), "T must be integral");
	static_assert(sizeof(std::size_t) >= 2*sizeof(T) || sizeof(std::size_t) == sizeof(T), "T must be either equal or half or less to std::size_t");
	constexpr std::size_t sizet = sizeof(T)*CHAR_BIT, sizet2 = sizet>>1;
	if constexpr (sizeof(std::size_t) >= 2*sizeof(T))
		return inx::bit_nshift_mask<0, sizet2, sizet>(static_cast<std::size_t>(val.y)) |
				inx::bit_nshift_mask<0, 0, sizet2>(static_cast<std::size_t>(val.x)) |
				inx::bit_nshift_mask<sizet2, sizet+sizet2, sizet2>(static_cast<std::size_t>(val.x));
	else if constexpr (sizeof(std::size_t) == sizeof(T))
		// boost::hash_combine implementation, manually here due to it not been constexpr
		return val.x ^ (val.y + 0x9e3779b9 + (val.x<<6) + (val.x>>2));
}

template <typename T>
std::istream& operator>>(std::istream& in, Point<T>& p)
{
	in >> p.x >> p.y;
	return in;
}
template <typename T>
std::ostream& operator<<(std::ostream& out, const Point<T>& p)
{
	out << p.x << ' ' << p.y;
	return out;
}

} // namespace inx

#endif // INX_POINT_HPP_INCLUDED
