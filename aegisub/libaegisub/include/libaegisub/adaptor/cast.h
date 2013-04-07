// Copyright (c) 2013, Thomas Goyne <plorkyeran@aegisub.org>
//
// Permission to use, copy, modify, and distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//
// Aegisub Project http://www.aegisub.org/

#include <boost/range/adaptor/transformed.hpp>

namespace agi {
	namespace cast_detail {
		using namespace boost::adaptors;

#define AGI_CAST_H_CASTER(cast_type) \
		template<class T> struct cast_type ## _tag {};
		template<class Type> struct cast_type ## _to { \
			typedef Type *result_type; \
			\
			template<class InType> Type *operator()(InType &ptr) const { \
				return cast_type<Type *>(&ptr); \
			} \
			\
			template<class InType> Type *operator()(InType *ptr) const { \
				return cast_type<Type *>(ptr); \
			} \
		};

		AGI_CAST_H_CASTER(static_cast)
		AGI_CAST_H_CASTER(dynamic_cast)
#undef AGI_CAST_H_CASTER

#define AGI_CAST_H_OPERATOR(cls, type) \
		template<class Rng, class Type> \
		inline auto operator|(Rng& r, cls##_tag<Type>) \
			-> decltype(r | transformed(cls<type>())) \
		{ \
			return r | transformed(cls<type>()) \
		} \

		AGI_CAST_H_OPERATOR(static_cast_to, Type)
		AGI_CAST_H_OPERATOR(static_cast_to, const Type)
		AGI_CAST_H_OPERATOR(dynamic_cast_to, Type)
		AGI_CAST_H_OPERATOR(dynamic_cast_to, const Type)
#undef AGI_CAST_H_OPERATOR
	}

	template<class T>
	inline cast_detail::static_cast_tag<T> cast() {
		return cast_detail::static_cast_tag<T>();
	}

	template<class T>
	inline cast_detail::dynamic_cast_tag<T> cast_dynamic() {
		return cast_detail::dynamic_cast_tag<T>();
	}
}
/*
head()
head(default)
when(member)
when(member, value)
except(member)
except(member, value)
iwhen(member, value)
iexcept(member, value)
enumerate
*/
