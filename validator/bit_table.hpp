#ifndef INX_BIT_TABLE_HPP_INCLUDED
#define INX_BIT_TABLE_HPP_INCLUDED

#include "inx.hpp"

namespace inx {

template <size_t BitCount = 1, size_t BufferSize = 0, typename PackType = size_t>
class bit_table
{
public:
	static_assert(std::is_integral<PackType>::value && std::is_unsigned<PackType>::value, "PackType must be an unsigned integer type");
	static_assert(0 < BitCount && BitCount <= 8, "BitCount must fall within [1,8]");
	static constexpr size_t bit_count = BitCount;
	static constexpr size_t buffer_size = BufferSize;
	using pack_type = PackType;
	struct index_t { uint32 word, bit; };

protected:
	static constexpr pack_type bit_mask = make_mask<pack_type,bit_count>();
	static constexpr size_t bit_adj = bit_count <= 1 ? 0 : bit_count <= 2 ? 1 : bit_count <= 4 ? 2 : 3;
	static constexpr size_t pack_bits = sizeof(pack_type) * CHAR_BIT;
	static_assert(8 <= pack_bits && pack_bits <= 64, "pack_bits must be between 8 and 64");
	static constexpr size_t pack_size = (pack_bits <= 8 ? 3 : pack_bits <= 16 ? 4 : pack_bits <= 32 ? 5 : pack_bits <= 64 ? 6 : 0) - bit_adj;
	static constexpr pack_type pack_mask = make_mask<pack_type,pack_size>();

public:
	static constexpr size_t bit_id_end = (1 << pack_size);
	static constexpr size_t bit_id_step = 1 << bit_adj;

public:
	bit_table() noexcept : mWidth(0), mHeight(0), mRowWords(0) { }
	bit_table(uint32 width, uint32 height)
	{
		setup(width, height);
	}

	void setup(uint32 width, uint32 height)
	{
		assert(width > 0);
		assert(height > 0);
		mWidth = width;
		mHeight = height;
		mRowWords = static_cast<uint32>(-(-static_cast<int32>(static_cast<pack_type>(width + 2 * buffer_size)) >> pack_size));
		mCells.clear();
		mCells.resize((mHeight + 2 * buffer_size) * mRowWords);
	}

	pack_type bit_get(int32 x, int32 y) const noexcept
	{
		return bit_get(bit_index(x, y));
	}
	template <size_t I = 0>
	bool bit_test(int32 x, int32 y) const noexcept
	{
		return bit_test<I>(bit_index(x, y));
	}
	void bit_set(int32 x, int32 y, pack_type value) noexcept
	{
		return bit_set(bit_index(x, y), value);
	}
	void bit_clear(int32 x, int32 y) noexcept
	{
		return bit_clear(bit_index(x, y));
	}
	void bit_and(int32 x, int32 y, pack_type value) noexcept
	{
		return bit_and(bit_index(x, y), value);
	}
	void bit_or(int32 x, int32 y, pack_type value) noexcept
	{
		return bit_or(bit_index(x, y), value);
	}
	void bit_xor(int32 x, int32 y, pack_type value) noexcept
	{
		return bit_xor(bit_index(x, y), value);
	}

	void set_buffer(pack_type value) noexcept
	{
		if constexpr (BufferSize != 0) {
			for (int32 i = -static_cast<int32>(BufferSize), j = static_cast<int32>(mHeight); i < 0; i++, j++)
			for (int32 x  = -static_cast<int32>(BufferSize),
					   xe = x + static_cast<int32>(mWidth) + static_cast<int32>(2*BufferSize);
					x < xe; ++x) {
				bit_set(x, i, value);
				bit_set(x, j, value);
			}

			for (int32 i = -static_cast<int32>(BufferSize), j = static_cast<int32>(mWidth); i < 0; i++, j++)
			for (int32 y  = 0,
					   ye = static_cast<int32>(mHeight);
					y < ye; ++y) {
				bit_set(i, y, value);
				bit_set(j, y, value);
			}
		}
	}

	template <int32 X, int32 Y, int32 W, int32 H>
	pack_type region(int32 x, int32 y) const noexcept
	{
		static_assert(X >= 0 && W > 0 && X < W, "x must lie within region");
		static_assert(Y >= 0 && H > 0 && Y < H, "y must lie within region");
		static_assert(W * H <= static_cast<int32>(1 << pack_size), "must be packable in a single pack_type");
		assert(-static_cast<int32>(buffer_size) <= x-X && x-X+W <= static_cast<int32>(mWidth + buffer_size));
		assert(-static_cast<int32>(buffer_size) <= y-Y && y-Y+H <= static_cast<int32>(mHeight + buffer_size));

		if constexpr (bit_count == 1 || bit_count == 2 || bit_count == 4 || bit_count == 8) { // tight packing of bits
			auto id = bit_index(x - X, y - Y);
			if (id.bit + (W<<bit_adj) > pack_bits) { // split bits
				uint32 w1count = pack_bits - id.bit;
				pack_type w2mask = make_mask<pack_type>((W << bit_adj) - w1count);
				pack_type ans = bit_right_shift<pack_type>(mCells[id.word], id.bit) | bit_left_shift<pack_type>(mCells[id.word+1] & w2mask, w1count);
				for (uint32 i = W << bit_adj; i < static_cast<uint32>(H * (W<<bit_adj)); i += W << bit_adj) {
					id.word += mRowWords;
					ans |= bit_left_shift<pack_type>(bit_right_shift<pack_type>(mCells[id.word], id.bit) | bit_left_shift<pack_type>(mCells[id.word+1] & w2mask, w1count), i);
				}

				return ans;
			} else {
				pack_type w1mask = make_mask<pack_type>(W << bit_adj);
				pack_type ans = bit_right_shift<pack_type>(mCells[id.word], id.bit) & w1mask;
				for (uint32 i = W << bit_adj; i < static_cast<uint32>(H * (W<<bit_adj)); i += W << bit_adj) {
					id.word += mRowWords;
					ans |= bit_left_shift<pack_type>(bit_right_shift<pack_type>(mCells[id.word], id.bit) & w1mask, i);
				}

				return ans;
			}
		} else {
			pack_type ans = 0;
			for (int32 i = 0, j = -Y; j < H-Y; j++)
			for (int32 k = -X; k < W-X; k++, i+=bit_count)
				ans |= bit_get(x+k, y+j) << i;
			return ans;
		}
	}

	uint32 getWidth() const noexcept { return mWidth; }
	uint32 getHeight() const noexcept { return mHeight; }
	uint32 getRowWords() const noexcept { return mRowWords; }

	bool empty() const noexcept { return mWidth == 0; }
	void clear()
	{
		mWidth = 0;
		mHeight = 0;
		mRowWords = 0;
		mCells.clear();
	}
	void shrink_to_fit()
	{
		mCells.shrink_to_fit();
	}

	index_t bit_index(int32 x, int32 y) const noexcept
	{
		assert(-static_cast<int32>(buffer_size) <= x && x < static_cast<int32>(mWidth + buffer_size));
		assert(-static_cast<int32>(buffer_size) <= y && y < static_cast<int32>(mHeight + buffer_size));
		x += buffer_size;
		return {
			static_cast<uint32>( (y + buffer_size) * mRowWords + (x >> pack_size) ),
			static_cast<uint32>( (x & pack_mask) << bit_adj )
		};
	}
	std::pair<int32, int32> index_get(index_t id) const noexcept
	{
		uint32 y = id.word / mRowWords;
		uint32 x = (id.word % mRowWords) << pack_size;
		x += id.bit >> bit_adj;
		return { static_cast<int32>(x) - static_cast<int32>(buffer_size), static_cast<int32>(y) - static_cast<int32>(buffer_size) };
	}
	pack_type bit_get(index_t id) const noexcept
	{
		return bit_right_shift<pack_type>(mCells[id.word], id.bit) & bit_mask;
	}
	template <size_t I>
	bool bit_test(index_t id) const noexcept
	{
		return static_cast<bool>(bit_right_shift<pack_type>(mCells[id.word], id.bit+I) & 1);
	}
	void bit_set(index_t id, pack_type value) noexcept
	{
		assert(value <= bit_mask);
		mCells[id.word] = (mCells[id.word] & ~bit_left_shift<pack_type>(bit_mask, id.bit))
			| bit_left_shift<pack_type>(value & bit_mask, id.bit);
	}
	void bit_clear(index_t id) noexcept
	{
		mCells[id.word] &= ~bit_left_shift<pack_type>(bit_mask, id.bit);
	}
	void bit_or(index_t id, pack_type value) noexcept
	{
		assert(value <= bit_mask);
		mCells[id.word] |= bit_left_shift<pack_type>(value & bit_mask, id.bit);
	}
	void bit_and(index_t id, pack_type value) noexcept
	{
		assert(value <= bit_mask);
		mCells[id.word] &= ~bit_left_shift<pack_type>((~value) & bit_mask, id.bit);
	}
	void bit_xor(index_t id, pack_type value) noexcept
	{
		assert(value <= bit_mask);
		mCells[id.word] ^= bit_left_shift<pack_type>(value & bit_mask, id.bit);
	}
	void bit_not(index_t id) noexcept
	{
		mCells[id.word] ^= bit_left_shift<pack_type>(bit_mask, id.bit);
	}
	
private:
	uint32 mWidth, mHeight, mRowWords;
	std::vector<pack_type> mCells;

};

} // namespace inx::alg

#endif // INX_BIT_TABLE_HPP_INCLUDED
