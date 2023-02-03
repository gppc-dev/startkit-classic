#ifndef INX_BOX_HPP_INCLUDED
#define INX_BOX_HPP_INCLUDED

#include "inx.hpp"
#include "frac.hpp"

namespace inx {

template <typename CoordType>
class Box : public std::pair<Point<CoordType>, Point<CoordType>>
{
private:
	using self = Box<CoordType>;
	using super = std::pair<Point<CoordType>, Point<CoordType>>;
public:
	using point_type = typename super::first_type;
	using coord_type = typename point_type::coord_type;
	using result_type = typename point_type::result_type;
	using unsigned_type = typename point_type::unsigned_type;

	// defaults
	Box() = default;
	constexpr Box(const self&) = default;
	constexpr Box(self&&) = default;
	using super::super;
	~Box() = default;

	constexpr self& operator=(const self&) = default;
	constexpr self& operator=(self&&) = default;

	constexpr Box(const point_type& lb, const point_type& ub) noexcept(noexcept(super(lb, ub)))
		: super(lb, ub)
	{ }

	template <typename OT>
	operator Box<OT>() const noexcept { return Box<OT>(static_cast<Point<OT>>(this->first), static_cast<Point<OT>>(this->second)); }

	static constexpr self zeroBox() noexcept { return Box(point_type(0, 0), point_type(0, 0)); }

	constexpr operator bool() const noexcept { return this->first.x <= this->second.x && this->first.y <= this->second.y; }

	constexpr unsigned_type width() const noexcept { return static_cast<unsigned_type>(static_cast<result_type>(this->second.x) - static_cast<result_type>(this->first.x)); }
	constexpr unsigned_type height() const noexcept { return static_cast<unsigned_type>(static_cast<result_type>(this->second.y) - static_cast<result_type>(this->first.y)); }
	constexpr const point_type& lower() const noexcept { return this->first; }
	constexpr point_type& lower() noexcept { return this->first; }
	constexpr const point_type& upper() const noexcept { return this->second; }
	constexpr point_type& upper() noexcept { return this->second; }
	constexpr point_type lower_norm() const noexcept { return point_type(std::min(this->first.x, this->second.x), std::min(this->first.y, this->second.y)); }
	constexpr point_type upper_norm() const noexcept { return point_type(std::max(this->first.x, this->second.x), std::max(this->first.y, this->second.y)); }

	constexpr point_type lowerLeft() const noexcept { return this->first; }
	constexpr point_type lowerRight() const noexcept { return {this->second.x, this->first.y}; }
	constexpr point_type upperRight() const noexcept { return this->second; }
	constexpr point_type upperLeft() const noexcept { return {this->first.x, this->second.y}; }

	constexpr bool is_norm() const noexcept
	{
		return this->first.x <= this->second.x && this->first.y <= this->second.y;
	}

	constexpr void normalise() noexcept
	{
		if (this->first.x > this->second.x)
			std::swap(this->first.x, this->second.x);
		if (this->first.y > this->second.y)
			std::swap(this->first.y, this->second.y);
	}

	constexpr self box_norm() const noexcept
	{
		self tmp(*this);
		tmp.normalise();
		return tmp;
	}

	constexpr result_type area2() const noexcept
	{
		return (static_cast<result_type>(this->second.x) - static_cast<result_type>(this->first.x)) *
			   (static_cast<result_type>(this->second.y) - static_cast<result_type>(this->first.y));
	}

	constexpr self& operator=(const point_type& pt) noexcept(noexcept(std::declval<point_type>() = pt))
	{
		this->first = this->second = pt;
		return *this;
	}
	constexpr self& operator<<(const point_type& pt) noexcept
	{
		if (pt.x < this->first.x) this->first.x = pt.x;
		else if (pt.x > this->second.x) this->second.x = pt.x;
		if (pt.y < this->first.y) this->first.y = pt.y;
		else if (pt.y > this->second.y) this->second.y = pt.y;
		return *this;
	}
	constexpr self& operator<<(const self& box) noexcept
	{
		if (box.first.x < this->first.x) this->first.x = box.first.x;
		if (box.first.y < this->first.y) this->first.y = box.first.y;
		if (box.second.x > this->second.x) this->second.x = box.second.x;
		if (box.second.y > this->second.y) this->second.y = box.second.y;
		return *this;
	}

	constexpr bool strictly_within(point_type pt) const noexcept
	{
		assert(is_norm());
		if constexpr (point_type::is_integral()) {
			if (int64 x = static_cast<int64>(pt.x) - static_cast<int64>(this->first.x) - 1; static_cast<uint64>(x) < static_cast<uint64>(this->second.x - this->first.x - 1)) {
				if (int64 y = static_cast<int64>(pt.y) - static_cast<int64>(this->first.y) - 1; static_cast<uint64>(y) < static_cast<uint64>(this->second.y - this->first.y - 1))
					return true;
			}
			return false;
		} else {
			return this->first.x + point_type::pos_epsilon() < pt.x && pt.x < this->second.x - point_type::pos_epsilon()
			    && this->first.y + point_type::pos_epsilon() < pt.y && pt.y < this->second.y - point_type::pos_epsilon();
		}
	}
	constexpr bool strictly_within(const self& box) const noexcept
	{
		assert(is_norm() && box.is_norm());
		return this->first.x + point_type::pos_epsilon() < box.first.x && this->second.x - point_type::pos_epsilon() > box.second.x
			&& this->first.y + point_type::pos_epsilon() < box.first.y && this->second.y - point_type::pos_epsilon() > box.second.y;
	}

	constexpr bool within(point_type pt) const noexcept
	{
		assert(is_norm());
		if constexpr (point_type::is_integral()) {
			if (int64 x = static_cast<int64>(pt.x) - static_cast<int64>(this->first.x); static_cast<uint64>(x) <= static_cast<uint64>(this->second.x - this->first.x)) {
				if (int64 y = static_cast<int64>(pt.y) - static_cast<int64>(this->first.y); static_cast<uint64>(y) <= static_cast<uint64>(this->second.y - this->first.y))
					return true;
			}
			return false;
		} else {
			return this->first.x - point_type::pos_epsilon() <= pt.x && pt.x <= this->second.x + point_type::pos_epsilon()
			    && this->first.y - point_type::pos_epsilon() <= pt.y && pt.y <= this->second.y + point_type::pos_epsilon();
		}
	}
	constexpr bool within(const self& box) const noexcept
	{
		assert(is_norm() && box.is_norm());
		return this->first.x - point_type::pos_epsilon() <= box.first.x && this->second.x + point_type::pos_epsilon() >= box.second.x
			&& this->first.y - point_type::pos_epsilon() <= box.first.y && this->second.y + point_type::pos_epsilon() >= box.second.y;
	}

	constexpr bool overlap(const self& box) const noexcept
	{
		assert(is_norm() && box.is_norm());
		bool xdis = this->second.x + point_type::pos_epsilon() < box.first.x || box.second.x + point_type::pos_epsilon() < this->first.x;
		bool ydis = this->second.y + point_type::pos_epsilon() < box.first.y || box.second.y + point_type::pos_epsilon() < this->first.y;
		return xdis && ydis;
	}

	constexpr bool strictly_overlap(const self& box) const noexcept
	{
		assert(is_norm() && box.is_norm());
		bool xdis = this->second.x - point_type::pos_epsilon() <= box.first.x || box.second.x - point_type::pos_epsilon() <= this->first.x;
		bool ydis = this->second.y - point_type::pos_epsilon() <= box.first.y || box.second.y - point_type::pos_epsilon() <= this->first.y;
		return !xdis && !ydis;
	}

	// 0=SW, 1=SE, 2=NW, 3=NE
	point_type get_point(int id) const noexcept
	{
		assert(0 <= id && id < 4);
		return point_type((id & 1) == 0 ? this->first.x : this->second.x, (id & 2) == 0 ? this->first.y : this->second.y);
	}
	std::pair<point_type, point_type> get_segment(int id) const noexcept
	{
		assert(0 <= id && id < 4);
		switch (id) {
		case 0:
			return {get_point(0), get_point(2)};
		case 1:
			return {get_point(2), get_point(3)};
		case 2:
			return {get_point(3), get_point(1)};
		case 3:
			return {get_point(1), get_point(0)};
		}
		// non-reachable
		return {point_type::zero(), point_type::zero()};
	}
};

} // namespace inx

#endif // INX_BOX_HPP_INCLUDED
