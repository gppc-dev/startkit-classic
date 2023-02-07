#ifndef INX_BRESENHAMRAYCAST_HPP_INCLUDED
#define INX_BRESENHAMRAYCAST_HPP_INCLUDED

#include "Point.hpp"
#include "Box.hpp"
#include "frac.hpp"
#include "bit_table.hpp"

namespace inx {

// how far behind long-axis should the line start
struct BresenhamDblLine
{
	using point = Point<double>;
	using crd = Point<int32>;
	constexpr static double axis_eps = 1e-8;
	constexpr static double axis_int_eps = 1e-6;

	double axisProg;
	double axisProgInc;
	uint32 axis; // 0 = x, y = 1; represents longer axis
	int32 axisMod;
	int32 axisIMod;
	frac<int32> prog;
	crd pos;
	point start;
	point adj;

	/// @param a The starting point
	/// @param ab The line segment
	/// @param adjStartUnits Adjusts a by number of major-axis units, pos is forward of ab, neg is backwards of ab (i.e. starts behind a)
	void setup(point a, point ab, int adjStartUnits = 0) noexcept
	{
		assert(!ab.isZero());
		// setup axis and axis mod
		if (std::abs(ab.y) < std::abs(ab.x)) {
			axis = 0;
			axisMod = ab.x < 0 ? -1 : 1;
			std::swap(a.x, a.y);
			std::swap(ab.x, ab.y);
		} else {
			axis = 1;
			axisMod = ab.y < 0 ? -1 : 1;
		}
		assert(std::abs(ab.y) >= std::abs(ab.x) && std::abs(ab.y) > axis_eps);
		prog.a = static_cast<int32>(adjStartUnits);
		prog.b = static_cast<int32>(std::ceil(std::abs(ab.y)));
		ab.x /= std::abs(ab.y);
		ab.y = axisMod;
		if (ab.x < -axis_eps)
			axisIMod = -1;
		else if (ab.x > axis_eps)
			axisIMod = 1;
		else
			axisIMod = 0;
		a += static_cast<double>(adjStartUnits) * ab;
		// align to edge of axis
		double axisF, axisI;
		if (axisMod >= 0) {
			axisF = std::floor(a.y);
			assert(axisF <= a.y);
			if (a.y - axisF > 1-axis_int_eps) {
				axisF += 1;
				axisI = a.x;
			} else {
				axisI = a.x - ab.x * (a.y - axisF);
			}
			pos[axis] = static_cast<int32>(axisF);
		} else {
			axisF = std::ceil(a.y);
			assert(axisF >= a.y);
			if (axisF - a.y > 1-axis_int_eps) {
				axisF -= 1;
				axisI = a.x;
			} else {
				axisI = a.x - ab.x * (axisF - a.y);
			}
			pos[axis] = static_cast<int32>(axisF)-1;
		}
		start[axis] = axisF;
		start[axis^1] = axisI;
		adj[axis^1] = ab.x;
		adj[axis] = ab.y;
		double modp, modi;
		modp = std::modf(axisI, &modi);
		assert(axisI >= 0);
		if (axisIMod > 0) {
			if (modp < axis_eps) {
				modi -= axisIMod;
				axisProg = 1+modp;
			} else if (modp > 1-axis_eps) {
			//	modi -= axisIMod;
				axisProg = modp;
			} else {
				axisProg = modp;
			}
			axisProgInc = ab.x;
		} else if (axisIMod < 0) {
			if (modp < axis_eps) {
			//	modi -= axisIMod;
				axisProg = 1 - modp;
			} else if (modp > 1-axis_eps) {
				modi -= axisIMod;
				axisProg = modp;
			} else {
				axisProg = 1 - modp;
			}
			axisProgInc = -ab.x;
		} else { // hori/vert line
			if (modp > 1-axis_eps) {
				modi += 1;
			}
			axisProg = 0.5;
			axisProgInc = 0;
		}
		pos[axis^1] = static_cast<int32>(std::floor(modi));
	}

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

	void make_ray() noexcept
	{
		prog.b = std::numeric_limits<int32>::max();
	}
};

template <typename BitTable, typename Function>
void bres_ray_loop(BresenhamDblLine& line, BitTable& bt, Function&& fn)
{
	// setup line detail
	auto bitId = bt.bit_index(line.pos.x, line.pos.y);
	// setup adjustment detail
	int32 bitRowAdj = static_cast<int32>(bt.getRowWords());
	int32 bitColAdj;
	int32 bitColReset;
	bool xpos = line.axisMod >= 0, ypos = line.axisIMod >= 0;
	if (line.axis != 0)
		std::swap(xpos, ypos);
	if (xpos) {
		bitColAdj = 1;
		bitColReset = 0;
	} else {
		bitColAdj = -1;
		bitColReset = BitTable::bit_id_end - BitTable::bit_id_step;
	}
	if (!ypos) {
		bitRowAdj = -bitRowAdj;
	}
	// loop through
	auto&& action = [&]() {
		if (!bt.template bit_test<0>(bitId)) {
			auto [x,y] = bt.index_get(bitId);
			fn(x, y);
		}
	};
	if (line.axis == 0) {
		// x-axis is longer
		do {
			action();
			if (line.increment()) { // y-axis inc
				if (line.intersection()) { // on corner, fill whole square
					auto tmpBitId = bitId;
					bitId.bit = static_cast<uint32>(static_cast<int32>(bitId.bit) + bitColAdj);
					if (bitId.bit >= BitTable::bit_id_end) {
						bitId.bit = bitColReset;
						bitId.word = static_cast<uint32>(static_cast<int32>(bitId.word) + bitColAdj);
					}
					action();
					bitId = tmpBitId;
				}
				bitId.word = static_cast<uint32>(static_cast<int32>(bitId.word) + bitRowAdj);
				action();
			}
			bitId.bit = static_cast<uint32>(static_cast<int32>(bitId.bit) + bitColAdj);
			if (bitId.bit >= BitTable::bit_id_end) {
				bitId.bit = bitColReset;
				bitId.word = static_cast<uint32>(static_cast<int32>(bitId.word) + bitColAdj);
			}
		}
		while (++line.prog.a <= line.prog.b);
	} else {
		// y-axis is longer
		do {
			action();
			if (line.increment()) { // y-axis inc
				if (line.intersection()) { // on corner, fill whole square
					auto tmpBitId = bitId;
					bitId.word = static_cast<uint32>(static_cast<int32>(bitId.word) + bitRowAdj);
					action();
					bitId = tmpBitId;
				}
				bitId.bit = static_cast<uint32>(static_cast<int32>(bitId.bit) + bitColAdj);
				if (bitId.bit >= BitTable::bit_id_end) {
					bitId.bit = bitColReset;
					bitId.word = static_cast<uint32>(static_cast<int32>(bitId.word) + bitColAdj);
				}
				action();
			}
			bitId.word = static_cast<uint32>(static_cast<int32>(bitId.word) + bitRowAdj);
		}
		while (++line.prog.a <= line.prog.b);
	}
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

	int rayShoot(point u, point v)
	{
		// not cached or ray not fired
		point uv = v - u;
		std::array<int, 2> cellSegments;
		if (pr_op<PRop::gtZero>(uv.x)) {
			if (pr_op<PRop::gtZero>(uv.y)) {
				cellSegments = {3, 0};
			} else if (pr_op<PRop::ltZero>(uv.y)) {
				cellSegments = {0, 1};
			} else {
				cellSegments = {0, -1};
			}
		} else if (pr_op<PRop::ltZero>(uv.x)) {
			if (pr_op<PRop::gtZero>(uv.y)) {
				cellSegments = {2, 3};
			} else if (pr_op<PRop::ltZero>(uv.y)) {
				cellSegments = {1, 2};
			} else {
				cellSegments = {2, -1};
			}
		} else {
			if (pr_op<PRop::gtZero>(uv.y)) {
				cellSegments = {3, -1};
			} else {
				cellSegments = {1, -1};
			}
		}

		// do line-scan
		BresenhamDblLine line;
		line.setup(u, uv, -1);
		line.prog.b += 1;
		int res = -1; // <0 -> no intersection, =0 -> corner intersection, >0 -> cell intersection
		bres_ray_loop(line, m_grid, [u,v,uv,&res,&line,&grid=m_grid,cellSegments] (int32 x, int32 y) {
			point at(x, y);
			box box_cell(at, at + point(1,1));
			if (box_cell.strictly_within(u) || box_cell.strictly_within(v)) {
				res = 1;
				line.prog.b = -1;
				return;
			}
			if (is_point_on_segment(at, u, uv)) {
				if (at == u || at == v) { // handled at line segment test section
					return;
				}
				auto cell = (~grid.region<1,1,2,2>(x, y)) & 0b1111;
				auto [p0, p1] = getAngle(cell);
				if (p0.isZero() || uv.isBetweenCW(p0, p1)) {
					res = 1;
					line.prog.b = -1;
					return;
				}
				return;
			}
			auto [p0, p1] = box_cell.get_segment(cellSegments[0]);
			auto p01 = p1 - p0;
			// p1 = at
			// u-v not collin with p1
			if (p01.isCCW(u-p0) && p01.isCW(v-p0) && uv.isBetweenCCW(u, p0, p1)) {
				res = 1;
				line.prog.b = -1;
				return;
			}
			if (cellSegments[1] >= 0) { // hori/vert line
				auto p2 = box_cell.get_segment(cellSegments[1]).second;
				auto p12 = p2 - p1;
				if (p12.isCCW(u-p1) && p12.isCW(v-p1) && uv.isBetweenCCW(u, p1, p2)) {
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
		box B(point(0,0), point(m_gridWidth, m_gridHeight));
		std::vector<point> P;
		P.resize(S);
		// copy path to new point vector
		for (int i = 0; i < S; ++i) {
			P[i] = transform_point(point(pts[i].x, pts[i].y));
			if (!B.within(P[i]))
				return i;
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
				switch (cell & 0b11) {
				case 0b0000:
					break;
				case 0b1111:
					good = false;
					break;
				case 0b0110:
					// #.
					// .#
					if (i == 0 || i == S-1) { // start or target
						point st2adj = i == 0 ? P[1] - P[0] : P[i-1] - P[i];
						if (st2adj.isBetweenCCW(point(1,0), point(0,1))) {
							good = false;
						}
					} else {
						point u2prev = P[i-1] - P[i];
						point u2next = P[i+1] - P[i];
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
				case 0b1001:
					// .#
					// #.
					if (i == 0 || i == S-1) { // start or target
						good = false;
					} else {
						point u2prev = P[i-1] - P[i];
						point u2next = P[i+1] - P[i];
						if (!u2prev.isBetweenCCW(point(1,0), point(0,-1))) { // in NE quad
							if (u2next.isBetweenCCW(point(1,0), point(0,-1))) { // not in NE quad
								good = false;
							}
						} else if (!u2prev.isBetweenCCW(point(-1,0), point(0,1))) { // in SW quad
							if (u2next.isBetweenCCW(point(-1,0), point(0,1))) { // not in SW quad
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
						if ((P[i+1] - P[i]).isBetweenCW(p0, p1))
							good = false;
					}
					if (i != 0) {
						if ((P[i-1] - P[i]).isBetweenCW(p0, p1))
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
							point i2j = P[j] - P[i];
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
			if (rayShoot(P[i], P[i+1]) >= 0) {
				return i;
			}
		}
		return -1;
	}

private:
	double m_gridHeight, m_gridWidth;
	bit_table<1, Padding> m_grid;
};

} // namespace inx

#endif // INX_BRESENHAMRAYCAST_HPP_INCLUDED
