#ifndef INX_HPP_INCLUDED
#define INX_HPP_INCLUDED

#include <climits>
#include <cfloat>
#include <cstdint>
#include <cerrno>
#include <cassert>
#include <cstddef>
#include <exception>
#include <limits>
#include <type_traits>
#include <memory>
#include <utility>
#include <tuple>
#include <functional>
#include <variant>

namespace inx {

#define INX_COMMA ,

#ifndef __FAST_MATH__
template <typename T>
inline constexpr T inf = std::numeric_limits<T>::infinity();
#else
template <typename T>
inline constexpr T inf = std::numeric_limits<T>::max();
#endif
template <typename T>
inline constexpr T pi = static_cast<T>(3.1415926535897932384626433);
#if 0
template <typename T>
inline constexpr T epsilon = static_cast<T>(std::is_same_v<T, float> ? (1.0 / 4096) : (1.0 / 4096 / 2048));
template <typename T>
inline constexpr T high_epsilon = static_cast<T>(std::is_same_v<T, float> ? (1.0 / 4096 / 64) : (1.0 / 4096 / 4096 / 16));
template <typename T>
inline constexpr T low_epsilon = static_cast<T>(std::is_same_v<T, float> ? (1.0 / 1024) : (1.0 / 4096 / 16));
template <typename T>
inline constexpr T very_low_epsilon = static_cast<T>(std::is_same_v<T, float> ? (1.0 / 128) : (1.0 / 4096 / 2));
#else
template <typename T>
inline constexpr T epsilon = static_cast<T>(std::is_same_v<T, float> ? 1e-4 : 1e-8);
template <typename T>
inline constexpr T high_epsilon = epsilon<T>;
template <typename T>
inline constexpr T low_epsilon = epsilon<T>;
template <typename T>
inline constexpr T very_low_epsilon = epsilon<T>;
#endif

using int8 = std::int8_t;
using int16 = std::int16_t;
using int32 = std::int32_t;
using int64 = std::int64_t;
using intptr = std::intptr_t;

using uint8 = std::uint8_t;
using uint16 = std::uint16_t;
using uint32 = std::uint32_t;
using uint64 = std::uint64_t;
using uintptr = std::intptr_t;

using size_t = std::size_t;
using ssize_t = std::make_signed_t<size_t>;
using ptrdiff_t = std::ptrdiff_t;

template <bool B, auto T, auto F>
struct conditional_value;
template <auto T, auto F>
struct conditional_value<true, T, F> : std::integral_constant<decltype(T), T>
{ };
template <auto T, auto F>
struct conditional_value<false, T, F> : std::integral_constant<decltype(F), F>
{ };
template <bool B, auto T, auto F>
inline constexpr auto conditional_value_v = conditional_value<B, T, F>::value;

template <typename T>
struct remove_cvref
{
	using type = std::remove_cv_t<std::remove_reference_t<T>>;
};
template <typename T>
using remove_cvref_t = typename remove_cvref<T>::type;

template <typename T, size_t I>
struct count_pointer_aux : std::integral_constant<size_t, I>
{ };
template <typename T, size_t I>
struct count_pointer_aux<T*, I> : count_pointer_aux<T, I+1>
{ };
template <typename T>
struct count_pointer : count_pointer_aux<T, 0>
{ };
template <typename T>
inline constexpr std::size_t count_pointer_v = count_pointer<T>::value;

template <typename T, size_t I>
struct add_const_pointer_aux;
template <typename T, size_t I>
struct add_const_pointer_aux<T*, I>
{
	using type = typename add_const_pointer_aux<T, I - 1>::type*;
};
template <typename T, size_t I>
struct add_const_pointer_aux<T*const, I>
{
	using type = typename add_const_pointer_aux<T, I - 1>::type* const;
};
template <typename T, size_t I>
struct add_const_pointer_aux<T*volatile, I>
{
	using type = typename add_const_pointer_aux<T, I - 1>::type* volatile;
};
template <typename T, size_t I>
struct add_const_pointer_aux<T*const volatile, I>
{
	using type = typename add_const_pointer_aux<T, I - 1>::type* const volatile;
};
template <typename T>
struct add_const_pointer_aux<T, 0>
{
	using type = std::add_const_t<T>;
};
template <typename T, size_t I>
struct add_const_pointer
{
	static_assert(I <= count_pointer_v<T>, "I must fall within count_poiner");
	using type = typename add_const_pointer_aux<T, count_pointer_v<T> - I>::type;
};
template <typename T, size_t I>
using add_const_pointer_t = typename add_const_pointer<T, I>::type;

template <typename L, typename T>
struct _Apply_Each;

template <typename T, typename L>
void apply_each(L&& la)
{
	_Apply_Each<L, T>::apply(std::forward<L>(la));
}

template <typename L, typename T, T... Ints>
struct _Apply_Each<L, std::integer_sequence<T, Ints...>>
{
	static void apply(L&& la)
	{
		apply<Ints...>(std::forward<L>(la));
	}
	template <T I, T... IS>
	static void apply(L&& la)
	{
		la(I);
		if constexpr (sizeof...(IS) > 0)
			apply<IS...>(std::forward<L>(la));
	}
};

constexpr size_t byte_size = CHAR_BIT;

template <typename T> struct raise_integral_level;
template <> struct raise_integral_level<int8> { using type = int16; };
template <> struct raise_integral_level<int16> { using type = int32; };
template <> struct raise_integral_level<int32> { using type = int64; };
template <> struct raise_integral_level<int64> { using type = int64; };
template <> struct raise_integral_level<uint8> { using type = uint16; };
template <> struct raise_integral_level<uint16> { using type = uint32; };
template <> struct raise_integral_level<uint32> { using type = uint64; };
template <> struct raise_integral_level<uint64> { using type = uint64; };
template <typename T>
using raise_integral_level_t = typename raise_integral_level<T>::type;

template <typename T>
struct raise_numeric_level : raise_integral_level<T>
{ };
template <>
struct raise_numeric_level<float> { using type = float; };
template <>
struct raise_numeric_level<double> { using type = double; };
template <>
struct raise_numeric_level<long double> { using type = long double; };
template <typename T>
using raise_numeric_level_t = typename raise_numeric_level<T>::type;

template <typename T>
std::enable_if_t<std::is_integral_v<T>, bool> is_zero(T x) noexcept
{
	return x == 0;
}
template <typename T>
std::enable_if_t<std::is_floating_point_v<T>, bool> is_zero(T x) noexcept
{
	return std::abs(x) < epsilon<T>;
}
template <typename T, typename... Ts>
std::enable_if_t<std:: conjunction_v<std::is_same<T, Ts>...>, bool> is_all_zero(T x, Ts... xs) noexcept
{
	if constexpr (std::is_integral_v<T>) {
		return ( (x | ... | xs) == 0 );
	} else {
		return (is_zero(x) && ... && is_zero(xs));
	}
}
template <typename T, typename... Ts>
std::enable_if_t<std::conjunction_v<std::is_same<T, Ts>...>, bool> is_any_zero(T x, Ts... xs) noexcept
{
	if constexpr (std::is_integral_v<T>) {
		return ( (x & ... & xs) == 0 );
	} else {
		return (is_zero(x) || ... || is_zero(xs));
	}
}


template <typename... T>
struct empty_type
{ };

template <typename... T>
inline constexpr bool false_value = false;
template <auto... T>
inline constexpr bool false_valuea = false;

template <typename... T>
inline constexpr bool true_value = true;
template <auto... T>
inline constexpr bool true_valuea = true;

template<typename Array, std::size_t... I>
auto to_tuple_aux_(const Array& a, std::index_sequence<I...>)
{
    return std::make_tuple(a[I]...);
}

template<typename T, std::size_t N, typename Indices = std::make_index_sequence<N>>
auto to_tuple(const std::array<T, N>& a)
{
    return to_tuple_aux_(a, Indices{});
}

template <typename T, typename Enable = void>
struct is_complete : std::false_type
{ };

template <typename T>
struct is_complete<T, std::void_t<decltype(sizeof(T) != 0)>> : std::true_type
{ };

template <typename... Types>
struct type_list
{ };

template <typename TypeList, typename... Types>
struct append_type_list;
template <typename... TypeListTypes>
struct append_type_list<type_list<TypeListTypes...>>
{
	using type = type_list<TypeListTypes...>;
};
template <typename Type1, typename... TypeListTypes>
struct append_type_list<type_list<TypeListTypes...>, Type1>
{
	using type = type_list<TypeListTypes..., Type1>;
};
template <typename TypeList, typename Type1, typename Type2, typename... Types>
struct append_type_list<TypeList, Type1, Type2, Types...> :
	append_type_list<typename append_type_list<TypeList, Type1>::type, Type2, Types...>
{ };
template <typename TypeList, typename... Types>
using append_type_list_t = typename append_type_list<TypeList, Types...>::type;

template <typename... TypeLists>
struct merge_type_lists;
template <typename TypeList1>
struct merge_type_lists<TypeList1>
{
	using type = TypeList1;
};
template <typename TypeList1, typename... TypeListTypes>
struct merge_type_lists<TypeList1, type_list<TypeListTypes...>> : append_type_list<TypeList1, TypeListTypes...>
{ };
template <typename TypeList1, typename TypeList2, typename TypeList3, typename... TypeLists>
struct merge_type_lists<TypeList1, TypeList2, TypeList3, TypeLists...> :
	merge_type_lists<typename merge_type_lists<TypeList1, TypeList2>::type, TypeList3, TypeLists...>
{ };
template <typename... TypeLists>
using merge_type_lists_t = typename merge_type_lists<TypeLists...>::type;

template <size_t Start, size_t End, typename TypeListMade, typename... Types>
struct sub_type_list_impl_;
template <size_t Start, size_t End, typename TypeListMade, typename Type1, typename... Types>
struct sub_type_list_impl_<Start, End, TypeListMade, Type1, Types...> : sub_type_list_impl_<Start-1, End-1, TypeListMade, Types...>
{ };
template <size_t End, typename TypeListMade, typename Type1, typename... Types>
struct sub_type_list_impl_<0, End, TypeListMade, Type1, Types...> : sub_type_list_impl_<0, End-1, typename append_type_list<TypeListMade, Type1>::type, Types...>
{ };
template <typename TypeListMade, typename Type1, typename... Types>
struct sub_type_list_impl_<0, 0, TypeListMade, Type1, Types...>
{
	using type = TypeListMade;
};
template <typename TypeListMade>
struct sub_type_list_impl_<0, 0, TypeListMade>
{
	using type = TypeListMade;
};
template <typename TypeList, size_t Start, ssize_t Count = -1>
struct sub_type_list;
template <size_t Start, ssize_t Count, typename... Types>
struct sub_type_list<type_list<Types...>, Start, Count> : sub_type_list_impl_<Start, Start+Count, type_list<>, Types...>
{ };
template <size_t Start, typename... Types>
struct sub_type_list<type_list<Types...>, Start, -1> : sub_type_list_impl_<Start, sizeof...(Types), type_list<>, Types...>
{ };
template <typename TypeList, size_t Start, ssize_t Count = -1>
using sub_type_list_t = typename sub_type_list<TypeList, Start, Count>::type;

template <typename T>
struct is_tuple : std::false_type
{ };
template <typename... Types>
struct is_tuple<std::tuple<Types...>> : std::true_type
{ };
template <typename T>
inline constexpr bool is_tuple_v = is_tuple<T>::value;

template <typename T>
struct is_variant : std::false_type
{ };
template <typename... Types>
struct is_variant<std::variant<Types...>> : std::true_type
{ };
template <typename T>
inline constexpr bool is_variant_v = is_variant<T>::value;

template <typename T, typename... Types>
struct is_same_any : std::disjunction<std::is_same<T, Types>...>
{ };
template <typename T, typename... Types>
inline constexpr bool is_same_any_v = is_same_any<T, Types...>::value;

template <typename T, typename Template>
struct template_has_type;
template <typename T, template<typename...> typename Template, typename... Types>
struct template_has_type<T, Template<Types...>> : is_same_any<T, Types...>
{ };
template <typename T, typename Template>
inline constexpr bool template_has_type_v = template_has_type<T, Template>::value;

template <typename T, typename Tuple>
struct tuple_has_type;
template <typename T, typename... Types>
struct tuple_has_type<T, std::tuple<Types...>> : is_same_any<T, Types...>
{ };
template <typename T, typename Tuple>
inline constexpr bool tuple_has_type_v = tuple_has_type<T, Tuple>::value;

template <typename T, typename Tuple>
struct variant_has_type;
template <typename T, typename... Types>
struct variant_has_type<T, std::variant<Types...>> : is_same_any<T, Types...>
{ };
template <typename T, typename Tuple>
inline constexpr bool variant_has_type_v = variant_has_type<T, Tuple>::value;

template <typename T, typename Tuple>
struct tuple_count;
template <typename T, template<typename...> typename Tuple, typename... Types>
struct tuple_count<T, Tuple<Types...>> : std::integral_constant<size_t,
	((std::is_same_v<T, Types> ? static_cast<size_t>(1) : static_cast<size_t>(0)) + ...)
> { };
template <typename T, typename Tuple>
inline constexpr size_t tuple_count_v = tuple_count<T, Tuple>::value;

template <size_t I, typename T, typename... Types>
struct type_index_found_;
template <size_t I, typename T, typename TypeAt, typename... Types>
struct type_index_found_<I, T, TypeAt, Types...> : type_index_found_<I, T, Types...>
{ };
template <size_t I, typename T, typename... Types>
struct type_index_found_<I, T, T, Types...>
{
	static_assert(false_valuea<I>, "type T appears more than once");
};
template <size_t I, typename T>
struct type_index_found_<I, T> : std::integral_constant<size_t, I>
{ };
template <size_t I, typename T, typename... Types>
struct type_index_search_;
template <size_t I, typename T, typename TypeAt, typename... Types>
struct type_index_search_<I, T, TypeAt, Types...> : type_index_search_<I+1, T, Types...>
{ };
template <size_t I, typename T, typename... Types>
struct type_index_search_<I, T, T, Types...> : type_index_found_<I, T, Types...>
{ };
template <size_t I, typename T>
struct type_index_search_<I, T>
{
	static_assert(false_valuea<I>, "type T not found");
};

template <typename T, typename... Types>
struct type_index : type_index_search_<0, T, Types...>
{ };
template <typename T, typename... Types>
inline constexpr size_t type_index_v = type_index<T, Types...>::value;

template <size_t Index, typename... Types>
struct type_select_loop_;
template <size_t Index, typename TypeAt, typename... Types>
struct type_select_loop_<Index, TypeAt, Types...> : type_select_loop_<Index-1, Types...>
{ };
template <typename TypeAt, typename... Types>
struct type_select_loop_<0, TypeAt, Types...>
{
	using type = TypeAt;
};
template <size_t Index>
struct type_select_loop_<Index>
{
	static_assert(false_valuea<Index>, "Index exceeds number of types");
};
template <size_t Index, typename... Types>
struct type_select : type_select_loop_<Index, Types...>
{ };
template <size_t Index, typename... Types>
using type_select_t = typename type_select<Index, Types...>::type;

template <typename T, typename Tuple>
struct template_index;
template <typename T, template<typename...> typename Template, typename... Types>
struct template_index<T, Template<Types...>> : type_index<T, Types...>
{ };
template <typename T, typename... Types>
inline constexpr size_t template_index_v = template_index<T, Types...>::value;

template <typename T, typename Tuple>
struct tuple_index;
template <typename T, typename... Types>
struct tuple_index<T, std::tuple<Types...>> : type_index<T, Types...>
{ };
template <typename T, typename... Types>
inline constexpr size_t tuple_index_v = tuple_index<T, Types...>::value;

template <typename T, typename Tuple>
struct variant_index;
template <typename T, typename... Types>
struct variant_index<T, std::variant<Types...>> : type_index<T, Types...>
{ };
template <typename T, typename... Types>
inline constexpr size_t variant_index_v = variant_index<T, Types...>::value;

template <typename Variant, typename Tuple>
struct holds_alternatives_;
template <typename Variant, class... Types>
struct holds_alternatives_<Variant, std::tuple<Types...>>
{
	static_assert(is_variant_v<Variant>, "Variant is not std::variant");
	static constexpr bool test(const Variant& v)
	{
		size_t id = v.index();
		return ((variant_index_v<Types, Variant> == id) || ...);
	}
};

template <typename Tuple, class... Types>
inline constexpr std::enable_if_t<is_tuple_v<Tuple>, bool> holds_alternatives(const std::variant<Types...>& v)
{
	return holds_alternatives_<std::variant<Types...>, Tuple>::test(v);
}

template <typename Tuple>
struct variant_from_tuple;
template <typename... Types>
struct variant_from_tuple<std::tuple<Types...>>
{
	using type = std::variant<Types...>;
};
template <typename Tuple>
using variant_from_tuple_t = typename variant_from_tuple<Tuple>::type;

template <typename Tuple>
struct tuple_from_template;
template <template <typename...> typename Template, typename... Types>
struct tuple_from_template<Template<Types...>>
{
	using type = std::tuple<Types...>;
};
template <typename Tuple>
using tuple_from_template_t = typename tuple_from_template<Tuple>::type;

template <typename Type1, typename... Type>
struct repeated_typename
{
	static_assert(( std::is_same_v<Type1, Type> && ... ), "Type must be the same as Type1");
};
template <size_t N, typename Type, typename... Types>
struct make_repeated_typename_impl
{
	using type = typename make_repeated_typename_impl<N-1, Type, Type, Types...>::type;
};
template <typename Type, typename... Types>
struct make_repeated_typename_impl<0, Type, Types...>
{
	using type = repeated_typename<Types...>;
};
template <size_t N, typename Type>
using make_repeated_typename = typename make_repeated_typename_impl<N, Type>::type;

template <typename T, T... I>
constexpr auto make_tuple_from_sequence_impl(std::integer_sequence<T, I...>) noexcept
{
	return std::tuple(I...);
}

template <typename T>
inline constexpr auto make_tuple_from_sequence = make_tuple_from_sequence_impl(T());

template <typename T, T... I>
constexpr auto make_array_from_sequence_impl(std::integer_sequence<T, I...>) noexcept
{
	return std::array{I...};
}

template <typename T>
inline constexpr auto make_array_from_sequence = make_array_from_sequence_impl(T());

template <typename TypeList, typename Find, typename... Replace>
struct replace_type_list
{
	constexpr static size_t index = template_index<Find, TypeList>::value;
	using type = typename merge_type_lists<
			typename sub_type_list<TypeList, 0, index>::type,
	        type_list<Replace...>,
			typename sub_type_list<TypeList, index+1>::type
		>::type;
};
template <typename TypeList, typename Find, typename... Replace>
using replace_type_list_t = typename replace_type_list<TypeList, Find, Replace...>::type;

template <bool Enable, typename Type>
struct optional_type
{
	using type = Type;
};
template <typename Type>
struct optional_type<false, Type>
{
	using type = empty_type<>;
};
template <bool Enable, typename Type>
using optional_type_t = typename optional_type<Enable, Type>::type;

template <typename... VTypes>
struct manual_variant;

template <typename T, typename MV>
struct manual_variant_can_hold : std::false_type { };
template <typename T, typename... VTypes>
struct manual_variant_can_hold<T, manual_variant<VTypes...>> : std::bool_constant<is_same_any<T, VTypes...>::value> { };

template <typename... VTypes>
struct manual_variant
{
	static_assert(!(std::is_const_v<VTypes> || ...), "VTypes can not be const");
	using self = manual_variant<VTypes...>;
	static constexpr std::size_t size() noexcept { return std::max({sizeof(VTypes)...}); }
	alignas(VTypes...) std::array<std::byte, size()> m_data;

	manual_variant() = default;
	manual_variant(const self& other ) = delete;
	manual_variant(self&& other) = delete;
	template <typename T, typename... Args>
	constexpr explicit manual_variant(std::in_place_type_t<T>, Args&&... args)
	{
		emplace<T>(std::forward<Args>(args)...);
	}

	template <typename T, typename... Args>
	constexpr std::enable_if_t<manual_variant_can_hold<T, self>::value, T&> emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args&&...>)
	{
		return *(::new (m_data.data()) T(std::forward<Args>(args)...));
	}

	template <typename T>
	constexpr std::enable_if_t<manual_variant_can_hold<T, self>::value> release() noexcept
	{
		std::launder(reinterpret_cast<T*>(m_data.data()))->~T();
	}

	template <typename T>
	constexpr std::enable_if_t<manual_variant_can_hold<T, self>::value, T&> get() noexcept { return *std::launder(reinterpret_cast<T*>(m_data.data())); }
	template <typename T>
	constexpr std::enable_if_t<manual_variant_can_hold<T, self>::value, const T&> get() const noexcept { return *std::launder(reinterpret_cast<const T*>(m_data.data())); }
};

template <typename MV, typename T>
[[nodiscard]] std::enable_if_t<manual_variant_can_hold<T, MV>::value, MV*> manual_variant_cast(T* type) noexcept
{
	if constexpr (offsetof(MV, m_data) == 0) {
		return std::launder(reinterpret_cast<MV*>(type));
	} else {
		return std::launder(reinterpret_cast<MV*>(reinterpret_cast<std::byte*>(type) - offsetof(MV, m_data)));
	}
}
template <typename MV, typename T>
[[nodiscard]] std::enable_if_t<manual_variant_can_hold<T, MV>::value, const MV*> manual_variant_cast(const T* type) noexcept
{
	if constexpr (offsetof(MV, m_data) == 0) {
		return std::launder(reinterpret_cast<const MV*>(type));
	} else {
		return std::launder(reinterpret_cast<const MV*>(reinterpret_cast<std::byte*>(type) - offsetof(MV, m_data)));
	}
}
template <typename MV, typename T>
[[nodiscard]] std::enable_if_t<manual_variant_can_hold<T, MV>::value, MV&> manual_variant_cast(T& type) noexcept
{
	return *manual_variant_cast<MV*>(&type);
}
template <typename MV, typename T>
[[nodiscard]] std::enable_if_t<manual_variant_can_hold<T, MV>::value, const MV&> manual_variant_cast(const T& type) noexcept
{
	return *manual_variant_cast<const MV*>(&type);
}

template <typename T1, typename... Tn>
struct first_tuple : std::tuple<T1, Tn...>
{
	using std::tuple<T1, Tn...>::tuple;
};
template <typename... Tn>
first_tuple(Tn&&... args) -> first_tuple<Tn...>;

template <typename... Tn>
bool operator<(const first_tuple<Tn...> a, const first_tuple<Tn...> b) noexcept(noexcept(std::get<0>(a) < std::get<0>(b)))
{
	return std::get<0>(a) < std::get<0>(b);
}
template <typename... Tn>
bool operator<=(const first_tuple<Tn...> a, const first_tuple<Tn...> b) noexcept(noexcept(std::get<0>(a) <= std::get<0>(b)))
{
	return std::get<0>(a) <= std::get<0>(b);
}
template <typename... Tn>
bool operator>(const first_tuple<Tn...> a, const first_tuple<Tn...> b) noexcept(noexcept(std::get<0>(a) > std::get<0>(b)))
{
	return std::get<0>(a) > std::get<0>(b);
}
template <typename... Tn>
bool operator>=(const first_tuple<Tn...> a, const first_tuple<Tn...> b) noexcept(noexcept(std::get<0>(a) >= std::get<0>(b)))
{
	return std::get<0>(a) >= std::get<0>(b);
}
template <typename... Tn>
bool operator==(const first_tuple<Tn...> a, const first_tuple<Tn...> b) noexcept(noexcept(std::get<0>(a) == std::get<0>(b)))
{
	return std::get<0>(a) == std::get<0>(b);
}
template <typename... Tn>
bool operator!=(const first_tuple<Tn...> a, const first_tuple<Tn...> b) noexcept(noexcept(std::get<0>(a) != std::get<0>(b)))
{
	return std::get<0>(a) != std::get<0>(b);
}

} // namespace inx

namespace std
{

template <typename T, typename... VTypes>
T& get(inx::manual_variant<VTypes...>& v)
{
	return v.template get<T>();
}
template <typename T, typename... VTypes>
const T& get(const inx::manual_variant<VTypes...>& v)
{
	return v.template get<T>();
}

} // namespace std

namespace inx
{

///
/// make_mask: bit mask
///   size_t Count: mask bit count
///   size_t Offset: mask offset from lsb
///
template <typename Type>
constexpr Type make_mask(size_t Count, size_t Offset = 0) noexcept
{
	static_assert(std::is_integral<Type>(), "Type must be integral");
	if (Count == sizeof(Type) * byte_size)
		return static_cast<Type>(~static_cast<std::make_unsigned_t<Type>>(0));
	return static_cast<Type>(~(~static_cast<std::make_unsigned_t<Type>>(0) << Count) << Offset);
}
template <typename Type, size_t Count, size_t Offset = 0>
constexpr Type make_mask() noexcept
{
	static_assert(Count + Offset <= sizeof(Type) * byte_size, "Mask exceeds Type bit count");
	return make_mask<Type>(Count, Offset);
}
template <typename Type, size_t Count, size_t Offset = 0>
struct make_mask_c : std::integral_constant<Type, make_mask<Type, Count, Offset>()>
{ };
template <typename Type, size_t Count, size_t Offset = 0>
inline constexpr Type make_mask_v = make_mask<Type, Count, Offset>();

///
/// bit_left_shift: left shift wrapper, equiv Value << Shift
///   Value:
///   Shift:
///
template <typename Type>
constexpr Type bit_left_shift(Type Value, size_t Shift)
{
	static_assert(std::is_integral_v<Type>, "Type must be integral");
	assert(Shift < sizeof(Type) * byte_size);
	if constexpr (std::is_unsigned_v<Type>)
		return static_cast<Type>(Value << Shift);
	else
		return static_cast<Type>(static_cast<std::make_unsigned_t<Type>>(Value) << Shift);
}
template <size_t Shift, typename Type>
constexpr Type bit_left_shift(Type Value)
{
	static_assert(Shift < sizeof(Type) * byte_size, "Shift exceeds Type bit count");
	if constexpr (Shift == 0)
		return Value;
	else
		return bit_left_shift(Value, Shift);
}
template <size_t Shift, auto Value>
constexpr decltype(Value) bit_left_shift()
{
	return bit_left_shift<Shift>(Value);
}
template <size_t Shift, auto Value>
inline constexpr decltype(Value) bit_left_shift_v = bit_left_shift<Shift, Value>();
template <size_t Shift, auto Value>
struct bit_left_shift_c : std::integral_constant<decltype(Value), bit_left_shift_v<Shift, Value>>
{ };

///
/// bit_left_shift: right shift wrapper, equiv Value >> Shift
///   Value:
///   Shift:
///
template <typename Type>
constexpr Type bit_right_shift(Type Value, size_t Shift)
{
	static_assert(std::is_integral_v<Type>, "Type must be integral");
	assert(Shift < sizeof(Type) * byte_size);
	return static_cast<Type>(Value >> Shift);
}
template <size_t Shift, typename Type>
constexpr Type bit_right_shift(Type Value)
{
	static_assert(Shift < sizeof(Type) * byte_size, "Shift exceeds Type bit count");
	if constexpr (Shift == 0)
		return Value;
	else
		return bit_right_shift(Value, Shift);
}
template <size_t Shift, auto Value>
constexpr decltype(Value) bit_right_shift()
{
	return bit_right_shift<Shift>(Value);
}
template <size_t Shift, auto Value>
inline constexpr decltype(Value) bit_right_shift_v = bit_right_shift<Shift, Value>();
template <size_t Shift, auto Value>
struct bit_right_shift_c : std::integral_constant<decltype(Value), bit_right_shift_v<Shift, Value>>
{ };

///
/// bit_left_nshift: right neutral shift, signed values always inserts 0, even for negative numbers
///   Value:
///   Shift:
///
template <typename Type>
constexpr Type bit_right_nshift(Type Value, size_t Shift)
{
	static_assert(std::is_integral_v<Type>, "Type must be integral");
	assert(Shift < sizeof(Type) * byte_size);
	if constexpr (std::is_unsigned_v<Type>)
		return static_cast<Type>(Value >> Shift);
	else
		return static_cast<Type>(static_cast<std::make_unsigned_t<Type>>(Value) >> Shift);
}
template <size_t Shift, typename Type>
constexpr Type bit_right_nshift(Type Value)
{
	static_assert(Shift < sizeof(Type) * byte_size, "Shift exceeds Type bit count");
	if constexpr (Shift == 0)
		return Value;
	else
		return bit_right_nshift(Value, Shift);
}
template <size_t Shift, auto Value>
constexpr decltype(Value) bit_right_nshift()
{
	return bit_right_nshift<Shift>(Value);
}
template <size_t Shift, auto Value>
inline constexpr decltype(Value) bit_right_nshift_v = bit_right_nshift<Shift, Value>();
template <size_t Shift, auto Value>
struct bit_right_nshift_c : std::integral_constant<decltype(Value), bit_right_nshift_v<Shift, Value>>
{ };

///
/// bit_shift: shifts left or right
///   Value:
///   Shift:
///
template <typename Type>
constexpr Type bit_shift(Type Value, ssize_t Shift) noexcept
{
	static_assert(std::is_integral_v<Type>, "Type must be integral");
	assert((Shift < 0 ? -Shift : Shift) < sizeof(Type) * byte_size);
	if (Shift < 0)
		return bit_right_shift(Value, static_cast<size_t>(-Shift));
	else
		return bit_left_shift(Value, static_cast<size_t>(Shift));
}
template <ssize_t Shift, typename Type>
constexpr Type bit_shift(Type Value) noexcept
{
	static_assert(std::is_integral_v<Type>, "Type must be integral");
	static_assert((Shift < 0 ? -Shift : Shift) < sizeof(Type) * byte_size, "Shift exceeds Type bit count");
	if constexpr (Shift < 0)
		return bit_right_shift<static_cast<size_t>(-Shift)>(Value);
	else if constexpr (Shift > 0)
		return bit_left_shift<static_cast<size_t>(Shift)>(Value);
	else
		return Value;
}
template <ssize_t Shift, auto Value>
constexpr decltype(Value) bit_shift() noexcept
{
	return bit_shift<Shift>(Value);
}
template <ssize_t Shift, auto Value>
inline constexpr decltype(Value) bit_shift_v = bit_shift<Shift, Value>();
template <ssize_t Shift, auto Value>
struct bit_shift_c : std::integral_constant<decltype(Value), bit_shift_v<Shift, Value>>
{ };

///
/// bit_nshift: neutral shifts left or right
///   Value:
///   Shift:
///
template <typename Type>
constexpr Type bit_nshift(Type Value, ssize_t Shift) noexcept
{
	static_assert(std::is_integral_v<Type>, "Type must be integral");
	assert((Shift < 0 ? -Shift : Shift) < sizeof(Type) * byte_size);
	if (Shift < 0)
		return bit_right_nshift(Value, static_cast<size_t>(-Shift));
	else
		return bit_left_shift(Value, static_cast<size_t>(Shift));
}
template <ssize_t Shift, typename Type>
constexpr Type bit_nshift(Type Value) noexcept
{
	static_assert(std::is_integral_v<Type>, "Type must be integral");
	static_assert((Shift < 0 ? -Shift : Shift) < sizeof(Type) * byte_size, "Shift exceeds Type bit count");
	if constexpr (Shift < 0)
		return bit_right_nshift<static_cast<size_t>(-Shift)>(Value);
	else if constexpr (Shift > 0)
		return bit_left_shift<static_cast<size_t>(Shift)>(Value);
	else
		return Value;
}
template <ssize_t Shift, auto Value>
constexpr decltype(Value) bit_nshift() noexcept
{
	return bit_shift<Shift>(Value);
}
template <ssize_t Shift, auto Value>
inline constexpr decltype(Value) bit_nshift_v = bit_nshift<Shift, Value>();
template <ssize_t Shift, auto Value>
struct bit_nshift_c : std::integral_constant<decltype(Value), bit_nshift_v<Shift, Value>>
{ };

///
/// bit_shift_set: shift from point to point in a single shift
///   From: bit shift from
///   To: bit shift to
///
template <size_t From, size_t To, typename Type>
constexpr Type bit_shift_set(Type Value) noexcept
{
	static_assert(std::is_integral<Type>(), "Type must be integral");
	static_assert(From < sizeof(Type) * byte_size, "From exceeds Type bit count");
	static_assert(To < sizeof(Type) * byte_size, "To exceeds Type bit count");
	return bit_shift<static_cast<ssize_t>(To) - static_cast<ssize_t>(From)>(Value);
}
template <size_t From, size_t To, auto Value>
constexpr decltype(Value) bit_shift_set() noexcept
{
	return bit_shift_set<From, To>(Value);
}
template <size_t From, size_t To, auto Value>
inline constexpr decltype(Value) bit_shift_set_v = bit_shift_set<From, To, Value>();
template <size_t From, size_t To, auto Value>
struct bit_shift_set_c : std::integral_constant<decltype(Value), bit_shift_set_v<From, To, Value>>
{ };

///
/// bit_nshift_set: neutral shift from point to point in a single shift
///   From: bit shift from
///   To: bit shift to
///
template <size_t From, size_t To, typename Type>
constexpr Type bit_nshift_set(Type Value) noexcept
{
	static_assert(std::is_integral<Type>(), "Type must be integral");
	static_assert(From < sizeof(Type) * byte_size, "From exceeds Type bit count");
	static_assert(To < sizeof(Type) * byte_size, "To exceeds Type bit count");
	return bit_nshift<static_cast<ssize_t>(To) - static_cast<ssize_t>(From)>(Value);
}
template <size_t From, size_t To, auto Value>
constexpr decltype(Value) bit_nshift_set() noexcept
{
	return bit_nshift_set<From, To>(Value);
}
template <size_t From, size_t To, auto Value>
inline constexpr decltype(Value) bit_nshift_set_v = bit_nshift_set<From, To, Value>();
template <size_t From, size_t To, auto Value>
struct bit_nshift_set_c : std::integral_constant<decltype(Value), bit_nshift_set_v<From, To, Value>>
{ };

///
/// bit_shift_to: shift bit to from variable from, all bits before from are cleared
///   From: bit shift from
///   To: bit shift to
///
template <size_t To, typename Type>
constexpr Type bit_shift_to(Type Value, size_t From) noexcept
{
	assert(From < sizeof(Type) * byte_size);
	static_assert(To < sizeof(Type) * byte_size, "To exceeds Type bit count");
	return bit_left_shift<To>(bit_right_shift(Value, From));
}

///
/// bit_shift_to: neutral shift bit to from variable from, all bits before from are cleared
///   From: bit shift from
///   To: bit shift to
///
template <size_t To, typename Type>
constexpr Type bit_nshift_to(Type Value, size_t From) noexcept
{
	assert(From < sizeof(Type) * byte_size);
	static_assert(To < sizeof(Type) * byte_size, "To exceeds Type bit count");
	return bit_left_shift<To>(bit_right_nshift(Value, From));
}

///
/// bit_shift_from: shift bit to from variable from, all bits before from are cleared
///   From: bit shift from
///   To: bit shift to
///
template <size_t From, typename Type>
constexpr Type bit_shift_from(Type Value, size_t To) noexcept
{
	static_assert(From < sizeof(Type) * byte_size, "From exceeds Type bit count");
	assert(To < sizeof(Type) * byte_size);
	return bit_left_shift(bit_right_shift<From>(Value), To);
}

///
/// bit_shift_from: shift bit to from variable from, all bits before from are cleared
///   From: bit shift from
///   To: bit shift to
///
template <size_t From, typename Type>
constexpr Type bit_nshift_from(Type Value, size_t To) noexcept
{
	static_assert(From < sizeof(Type) * byte_size, "From exceeds Type bit count");
	assert(To < sizeof(Type) * byte_size);
	return bit_left_shift(bit_right_nshift<From>(Value), To);
}

template <typename Type, size_t Segment, typename... Args>
constexpr std::enable_if_t<std::conjunction_v<std::bool_constant<std::is_convertible_v<Args, Type>>...>, Type> // Type
bit_pack_lsb(Args... args) noexcept
{
	static_assert(std::is_integral<Type>(), "Type must be integral");
	static_assert(Segment <= sizeof(Type) * byte_size, "Segment exceeds number of available bits of Type");
	static_assert(Segment * sizeof...(Args) <= sizeof(Type) * byte_size, "Number of slotted segments exceeds availble bit count");
	struct helper {
		Type out;
		constexpr helper(Type a) : out(a & make_mask<Type, Segment>()) { }
		constexpr helper(Type a, Type b) : out(a | (b << Segment)) { }
		constexpr helper operator<<(helper x) { return helper(out, x.out); }
	};
	//return (helper(args) << ... << helper(0)).out;
	return (helper(args) << ...).out;
}
template <typename Type, size_t Segment, Type... Args>
constexpr std::enable_if_t<(sizeof...(Args) > 0), Type> // Type
bit_pack_lsb() noexcept
{
	return bit_pack_lsb<Type, Segment>(Args...);
}
template <typename Type, size_t Segment, Type... Args>
inline constexpr Type bit_pack_lsb_v = bit_pack_lsb<Type, Segment, Args...>();
template <typename Type, size_t Segment, Type... Args>
struct bit_pack_lsb_c : std::integral_constant<Type, bit_pack_lsb_v<Type, Segment, Args...>>
{ };

template <size_t Segment, typename Type>
constexpr Type bit_unpack_lsb(size_t i, Type pack) noexcept
{
	static_assert(std::is_integral<Type>(), "Type must be integral");
	static_assert(Segment <= sizeof(Type) * byte_size, "Segment exceeds number of available bits of Type");
	assert(i <= sizeof(Type) * byte_size && i * Segment <= sizeof(Type) * byte_size);
	if constexpr (std::is_signed_v<Type>)
		return bit_shift_set<sizeof(Type) * byte_size - Segment, 0>(bit_left_shift(pack, sizeof(Type) * byte_size - (i+1)*Segment));
	else
		return bit_right_shift(pack, i*Segment) & make_mask_v<Type, Segment>;
}
template <size_t Segment, size_t I, typename Type>
constexpr Type bit_unpack_lsb(Type pack) noexcept
{
	static_assert(I <= sizeof(Type) * byte_size && I * Segment <= sizeof(Type) * byte_size, "Number of slotted segments exceeds availble bit count");
	return bit_unpack_lsb<Segment>(I, pack);
}
template <size_t Segment, size_t I, auto Pack>
constexpr decltype(Pack) bit_unpack_lsb() noexcept
{
	return bit_unpack_lsb<Segment, I>(Pack);
}
template <size_t Segment, size_t I, auto Pack>
inline constexpr decltype(Pack) bit_unpack_lsb_v = bit_unpack_lsb<Segment, I, Pack>();
template <size_t Segment, size_t I, auto Pack>
struct bit_unpack_lsb_c : std::integral_constant<decltype(Pack), bit_unpack_lsb_v<Segment, I, Pack>>
{ };

template <typename Type, size_t Segment, typename... Args>
constexpr std::enable_if_t<std::conjunction_v<std::bool_constant<std::is_convertible_v<Args, Type>>...>, Type> // Type
bit_pack_msb(Args... args) noexcept
{
	static_assert(Segment <= sizeof(Type) * byte_size, "Segment exceeds number of available bits of Type");
	static_assert(Segment * sizeof...(Args) <= sizeof(Type) * byte_size, "Number of slotted segments exceeds availble bit count");
	struct helper {
		Type out;
		constexpr helper(Type a) : out(a & make_mask<Type, Segment>()) { }
		constexpr helper(Type a, Type b) : out((a << Segment) | b) { }
		constexpr helper operator<<(helper x) { return helper(out, x.out); }
	};
	return (helper(0) << ... << helper(args)).out;
}
template <typename Type, size_t Segment, Type... Args>
constexpr std::enable_if_t<(sizeof...(Args) > 0), Type> // Type
bit_pack_msb() noexcept
{
	return bit_pack_msb<Type, Segment>(Args...);
}
template <typename Type, size_t Segment, Type... Args>
struct bit_pack_msb_c : std::integral_constant<Type, bit_pack_msb<Type, Segment, Args...>()>
{ };
template <typename Type, size_t Segment, Type... Args>
inline constexpr Type bit_pack_msb_v = bit_pack_msb<Type, Segment, Args...>();

template <size_t From, size_t To, size_t Count, typename Type>
constexpr Type bit_shift_mask(Type Value) noexcept
{
	static_assert(std::is_integral<Type>(), "Type must be integral");
	static_assert(From < sizeof(Type) * byte_size, "From exceeds Type bit count");
	static_assert(To < sizeof(Type) * byte_size, "To exceeds Type bit count");
	static_assert(From + Count <= sizeof(Type) * byte_size && To + Count <= sizeof(Type) * byte_size, "Count must make a valid mask");
	return bit_shift_set<From, To>(Value) & make_mask<Type, Count, To>();
}
template <size_t From, size_t To, size_t Count, auto Value>
constexpr decltype(Value) bit_shift_mask() noexcept
{
	return bit_shift_mask(Value);
}
template <size_t From, size_t To, size_t Count, auto Value>
struct bit_shift_mask_c : std::integral_constant<decltype(Value), bit_shift_mask<From, To, Count, Value>()>
{ };
template <size_t From, size_t To, size_t Count, auto Value>
inline constexpr decltype(Value) bit_shift_mask_v = bit_shift_mask_c<From, To, Count, Value>::value;

template <size_t From, size_t To, size_t Count, typename Type>
constexpr Type bit_nshift_mask(Type Value) noexcept
{
	return static_cast<Type>(bit_shift_mask<From, To, Count>(static_cast<std::make_unsigned_t<Type>>(Value)));
}
template <size_t From, size_t To, size_t Count, auto Value>
constexpr decltype(Value) bit_nshift_mask() noexcept
{
	return bit_nshift_mask(Value);
}
template <size_t From, size_t To, size_t Count, auto Value>
struct bit_nshift_mask_c : std::integral_constant<decltype(Value), bit_nshift_mask<From, To, Count, Value>()>
{ };
template <size_t From, size_t To, size_t Count, auto Value>
inline constexpr decltype(Value) bit_nshift_mask_v = bit_nshift_mask_c<From, To, Count, Value>::value;

#if defined(__GNUC__) || defined(__clang__)

template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
constexpr int clz(T val) noexcept
{
	assert(val > 0);
	if constexpr (sizeof(T) <= 4) {
		return __builtin_clz(static_cast<unsigned int>(val));
	} else {
		return __builtin_clzll(static_cast<unsigned long long>(val));
	}
}

template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
constexpr int ctz(T val) noexcept
{
	assert(val > 0);
	if constexpr (sizeof(T) <= 4) {
		return __builtin_ctz(static_cast<unsigned int>(val));
	} else {
		return __builtin_ctzll(static_cast<unsigned long long>(val));
	}
}

template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
constexpr int popcount(T val) noexcept
{
	if constexpr (sizeof(T) <= 4) {
		return __builtin_popcount(static_cast<unsigned int>(val));
	} else {
		return __builtin_popcountll(static_cast<unsigned long long>(val));
	}
}

template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
constexpr int clz_index(T val) noexcept
{
	assert(val > 0);
	if constexpr (sizeof(T) <= 4) {
		return (sizeof(uint32) * byte_size - 1) - __builtin_clz(static_cast<unsigned int>(val));
	} else {
		return (sizeof(uint64) * byte_size - 1) - __builtin_clzll(static_cast<unsigned long long>(val));
	}
}

#endif

} // namespace inx

#endif // INX_HPP_INCLUDED
