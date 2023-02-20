#ifndef INX_BRESENHAMRAYCAST_HPP_INCLUDED
#define INX_BRESENHAMRAYCAST_HPP_INCLUDED

#include "Point.hpp"
#include "Box.hpp"
#include "frac.hpp"
#include "bit_table.hpp"

namespace inx {

constexpr double MIN_SEGMENT_LENGTH = 0.01;

// how far behind long-axis should the line start
struct BresenhamDblLine
{
	using point = Point<double>;
	using crd = Point<int32>;
	constexpr static double axis_eps = 1e-8;
	constexpr static double axis_int_eps = 1e-6;

	frac<int32> prog;
	uint32 axis; // 0 = x, y = 1; represents longer axis
	int32 axisMod;
	int32 axisIMod;
	int32 startAxis;
	double startAxisI;
	double axisIscale;

	static bool is_int(double v) noexcept
	{
		return std::abs(v - std::round(v)) < axis_int_eps;
	}

	/// @param a The starting point
	/// @param ab The line segment
	/// @param adjStartUnits Adjusts a by number of major-axis units, pos is forward of ab, neg is backwards of ab (i.e. starts behind a)
	void setup(point a, point ab, int adjStartUnits = 0) noexcept
	{
		assert(!ab.isZero());
		// setup axis and axis mod
		if (std::abs(ab.y) < std::abs(ab.x)) {
			axis = 0;
		} else {
			axis = 1;
		}
		axisMod = ab[axis] < 0 ? -1 : 1;
		axisIMod = ab[axis^1] < 0 ? -1 : 1;
		// variable naming assumes axis = 0, thus x is major axis
		assert(std::abs(ab[axis]) >= std::abs(ab[axis^1]) && std::abs(ab[axis]) > axis_eps);
		bool Ix = is_int(a[axis]); // Ix/Iy are is integer
		axisIscale = ab[axis^1] / std::abs(ab[axis]);
		prog.a = static_cast<int32>(adjStartUnits) - (Ix ? 1 : 0);
		if (axisMod > 0) {
			startAxis = static_cast<int32>(std::floor(Ix ? a[axis] + 0.5 : a[axis]));
			prog.b = std::abs(static_cast<int32>(std::ceil(a[axis] + ab[axis] + axis_int_eps)) - startAxis);
		} else {
			startAxis = static_cast<int32>(std::ceil(Ix ? a[axis] - 0.5 : a[axis]));
			prog.b = std::abs(static_cast<int32>(std::floor(a[axis] + ab[axis] + axis_int_eps)) - startAxis);
		}
		startAxisI = a[axis^1] - axisIscale * std::abs(a[axis] - startAxis);
	}

	// returns coord drawn at cell start[axis]+i, second parameter [-1,1] to include cell above or below in axix^1
	std::pair<crd, int32> getCoord(int32 i) noexcept
	{
		std::pair<crd, int32> result{crd::zero(), 0};
		result.first[axis] = startAxis + i * axisMod;
		double lineY = startAxisI + i * axisIscale;
		if (is_int(lineY)) {
			result.first[axis^1] = static_cast<int32>(std::floor(lineY-0.5));
			result.second = 1;
		} else {
			result.first[axis^1] = static_cast<int32>(std::floor(lineY));
			lineY += axisIscale;
			if (is_int(lineY) || static_cast<int32>(std::floor(lineY)) != result.first[axis^1]) {
				result.second = axisIMod;
			}
		}
		return result;
	}

	operator bool() const noexcept
	{
		return prog.a < prog.b;
	}

	std::pair<crd, bool> getNextCoord() noexcept
	{
		return getCoord(prog.a++);
	}

#if 0
	// increment one axis-sep
	// if returns true, than pos[axis^1] += axisIMod
	// afterwards, pos[axis] += axisMod
	// prog and pos remain unchanged, up to user to track changes
	bool increment() noexcept
	{
		axisProg += axisProgInc;
		if (axisProg >= 1-axis_eps) {
			axisProg -= 1;
			return true;
		}
		return false;
	}

	bool intersection() const noexcept
	{
		return axisProg < axis_eps;
	}
#endif

	void make_ray() noexcept
	{
		prog.b = std::numeric_limits<int32>::max();
	}
};

template <typename BitTable, typename Function>
void bres_ray_loop(BresenhamDblLine& line, BitTable&, Function&& fn)
{
	do {
		auto [c,s] = line.getNextCoord();
		fn(c.x, c.y);
		if (s != 0) {
			c[line.axis^1] += s;
			fn(c.x, c.y);
		}
	}
	while (line);
}

class BresenhamRay
{
public:
	static constexpr size_t Padding = 4;
	using coord = double;
	using point = Point<coord>;
	using box = Box<coord>;

	static constexpr std::pair<point, point> getAngle(uint32 cells) noexcept
	{
		// 01
		// 23
		switch (cells & 0b1111) {
		case 0b0000:
		case 0b1111:
		case 0b0110:
		case 0b1001:
			return {point::zero(), point::zero()};
		case 0b0100:
			return {point(-1,0), point(0,1)};
		case 0b1000:
			return {point(0,1), point(1,0)};
		case 0b1100:
			return {point(-1,0), point(1,1)};
		case 0b0001:
			return {point(0,-1), point(-1,0)};
		case 0b0101:
			return {point(0,-1), point(0,1)};
		case 0b1101:
			return {point(0,-1), point(1,0)};
		case 0b0010:
			return {point(1,0), point(0,-1)};
		case 0b1010:
			return {point(0,1), point(0,-1)};
		case 0b1110:
			return {point(-1,0), point(0,-1)};
		case 0b0011:
			return {point(1,0), point(-1,0)};
		case 0b0111:
			return {point(1,0), point(0,1)};
		case 0b1011:
			return {point(0,1), point(-1,0)};
		}
		return {point::zero(), point::zero()};
	}

	template <auto TravValue, typename GridType>
	void setGrid(size_t width, size_t height, const GridType& grid)
	{
		m_gridHeight = static_cast<coord>(height);
		m_gridWidth = static_cast<coord>(width);
		m_grid.setup(width, height);
		for (size_t yo = 0, yn = height-1, index = 0; yo < height; ++yo, --yn) {
			for (size_t x = 0; x < width; ++x) {
				if (grid[index] == TravValue) { // traversable cell value
					m_grid.bit_or(x, yn, 1);
				}
				index += 1;
			}
		}
		// std::ofstream fout("debug.txt");
		// for (size_t y = 0; y < height; ++y) {
		// 	for (size_t x = 0; x < width; ++x) {
		// 		fout.put("#."[static_cast<int>(m_grid.bit_test<0>(x, y))]);
		// 	}
		// 	fout.put('\n');
		// }
	}

	point transform_point(point u) const noexcept
	{
		return point(u.x, m_gridHeight - u.y);
	}

	int rayShoot(point u, point v, point uvN)
	{
		// not cached or ray not fired
		point uv = v - u;
		std::array<int, 2> cellSegments;
		if (pr_op<PRop::gtZero>(uvN.x)) {
			if (pr_op<PRop::gtZero>(uvN.y)) {
				cellSegments = {3, 0};
			} else if (pr_op<PRop::ltZero>(uvN.y)) {
				cellSegments = {0, 1};
			} else {
				cellSegments = {0, -1};
			}
		} else if (pr_op<PRop::ltZero>(uvN.x)) {
			if (pr_op<PRop::gtZero>(uvN.y)) {
				cellSegments = {2, 3};
			} else if (pr_op<PRop::ltZero>(uvN.y)) {
				cellSegments = {1, 2};
			} else {
				cellSegments = {2, -1};
			}
		} else {
			if (pr_op<PRop::gtZero>(uvN.y)) {
				cellSegments = {3, -1};
			} else {
				cellSegments = {1, -1};
			}
		}

		// do line-scan
		BresenhamDblLine line;
		line.setup(u, uv, 0);
		// line.prog.b += 1;
		int res = -1; // <0 -> no intersection, =0 -> corner intersection, >0 -> cell intersection
		bres_ray_loop(line, m_grid, [u,v,uv,uvN,&res,&line,&grid=std::as_const(m_grid),cellSegments] (int32 x, int32 y) {
			point at(x, y);
			if (is_point_on_segment(at, u, uv)) {
				if (at == u || at == v) { // handled at line segment test section
					return;
				}
				auto cell = (~grid.region<1,1,2,2>(x, y)) & 0b1111;
				if (cell != 0) {
					auto [p0, p1] = getAngle(cell);
					if (p0.isZero() || uvN.isBetweenCW(p0, p1)) {
						res = 1;
						line.prog.b = -1;
					}
				}
				return;
			}
			if (grid.bit_test<0>(x, y)) // not filled, stop checking
				return;
			box box_cell(at, at + point(1,1));
			if (box_cell.strictly_within(u) || box_cell.strictly_within(v)) {
				res = 1;
				line.prog.b = -1;
				return;
			}
			auto [p0, p1] = box_cell.get_segment(cellSegments[0]);
			auto p01 = p1 - p0;
			// p1 = at
			// u-v not collin with p1
			if (p01.isCCW(u-p0) && p01.isCW(v-p0) && uvN.isBetweenCCW(u, p0, p1)) {
				res = 1;
				line.prog.b = -1;
				return;
			}
			if (cellSegments[1] >= 0) { // hori/vert line
				auto p2 = box_cell.get_segment(cellSegments[1]).second;
				auto p12 = p2 - p1;
				if (p12.isCCW(u-p1) && p12.isCW(v-p1) && uvN.isBetweenCCW(u, p1, p2)) {
					res = 1;
					line.prog.b = -1;
					return;
				}
			}
		});

		return res;
	}

	// returns -1 if valid path, otherwise id of segment where invalidness was detetcted
	template <typename Pts>
	int validPath(const Pts& pts) {
		int S = static_cast<int>(pts.size());
		if (S == 0)
			return -1;
		box B(point(0,0), point(m_gridWidth, m_gridHeight));
		P.resize(S);
		Pnorm.resize(S-1);
		// copy path to new point vector
		for (int i = 0; i < S; ++i) {
			P[i] = transform_point(point(pts[i].x, pts[i].y));
			if (!B.within(P[i]))
				return i;
			if (i > 0) {
				point norm = P[i] - P[i-1];
				if (norm.square() < MIN_SEGMENT_LENGTH*MIN_SEGMENT_LENGTH-point::pos_epsilon()) {
					return i-1;
				}
				Pnorm[i-1] = norm.normalise();
			}
		}
		// pre-check all points and then transform
		for (int i = 0; i < S; ++i) {
			bool good = true;
			bool xInt = P[i].isIntegerX(), yInt = P[i].isIntegerY();
			if (xInt && yInt) { // is integer
				int32 x = static_cast<int32>(std::round(P[i].x));
				int32 y = static_cast<int32>(std::round(P[i].y));
				auto cell = (~m_grid.region<1,1,2,2>(x, y)) & 0b1111;
				// 2 3
				// 0 1
				switch (cell) {
				case 0b0000:
					break;
				case 0b1111:
					good = false;
					break;
				case 0b1001:
					// #.
					// .#
					if (i == 0 || i == S-1) { // start or target
						point st2adj = i == 0 ? Pnorm.front() : -Pnorm.back();
						if (st2adj.isBetweenCCW(point(1,0), point(0,-1))) {
							good = false;
						}
					} else {
						point u2prev = -Pnorm[i-1];
						point u2next = Pnorm[i];
						if (!u2prev.isBetweenCCW(point(1,0), point(0,-1))) { // in SE quad
							if (u2next.isBetweenCCW(point(1,0), point(0,-1))) { // not in SE quad
								good = false;
							}
						} else if (!u2prev.isBetweenCCW(point(-1,0), point(0,1))) { // in NW quad
							if (u2next.isBetweenCCW(point(-1,0), point(0,1))) { // not in NW quad
								good = false;
							}
						} else {
							good = false;
						}
					}
					break;
				case 0b0110:
					// .#
					// #.
					if (i == 0 || i == S-1) { // start or target
						good = false;
					} else {
						point u2prev = -Pnorm[i-1];
						point u2next = Pnorm[i];
						if (!u2prev.isBetweenCCW(point(0,1), point(1,0))) { // in NE quad
							if (u2next.isBetweenCCW(point(0,1), point(1,0))) { // not in NE quad
								good = false;
							}
						} else if (!u2prev.isBetweenCCW(point(0,-1), point(-1,0))) { // in SW quad
							if (u2next.isBetweenCCW(point(0,-1), point(-1,0))) { // not in SW quad
								good = false;
							}
						} else {
							good = false;
						}
					}
					break;
				default:
					auto [p0,p1] = getAngle(cell);
					if (i != S-1) {
						if ((Pnorm[i]).isBetweenCW(p0, p1))
							good = false;
					}
					if (i != 0) {
						if ((-Pnorm[i-1]).isBetweenCW(p0, p1))
							good = false;
					}
				}
			} else if (xInt || yInt) { // point lies of vertical divide
				int32 x, y;
				size_t cell;
				if (xInt) { // x
					x = static_cast<int32>(std::round(P[i].x));
					y = static_cast<int32>(std::floor(P[i].y));
					cell = (~m_grid.region<1,0,2,1>(x, y)) & 0b11;
				} else { // y
					x = static_cast<int32>(std::floor(P[i].x));
					y = static_cast<int32>(std::round(P[i].y));
					cell = (~m_grid.region<0,1,1,2>(x, y)) & 0b11;
				}
				// x
				// 0 1
				// y
				// 1
				// 0
				switch (cell) {
				case 0b11:
					good = false;
					break;
				case 0b01:
				case 0b10:
				{
					point wall = xInt ?
						((cell & 0b01) ? point(0, -1) : point(0, 1)) :
						((cell & 0b01) ? point(1, 0) : point(-1, 0));
					for (int j = i-1; ; j += 2) {
						if (static_cast<uint32>(j) < static_cast<uint32>(S)) {
							point i2j = j < i ? -Pnorm[i-1] : Pnorm[i];
							if (wall.isCW(i2j)) {
								good = false;
								break;
							}
						}
						if (j > i)
							break;
					}
				}
					break;
				default:
					break;
				}
			}
			if (!good) {
				return i;
			}
		}
		// visibility test for each line segment
		for (int i = 0; i < S-1; ++i) {
			if (rayShoot(P[i], P[i+1], Pnorm[i]) >= 0) {
				return i;
			}
		}
		return -1;
	}

private:
	double m_gridHeight, m_gridWidth;
	bit_table<1, Padding> m_grid;
	std::vector<point> P;
	std::vector<point> Pnorm;
};

} // namespace inx

#endif // INX_BRESENHAMRAYCAST_HPP_INCLUDED
