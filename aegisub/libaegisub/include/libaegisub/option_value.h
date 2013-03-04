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

#include <libaegisub/color.h>
#include <libaegisub/exception.h>
#include <libaegisub/signal.h>

#include <boost/variant.hpp>
#include <cstdint>
#include <vector>

namespace agi {
DEFINE_BASE_EXCEPTION_NOINNER(OptionValueError, Exception)
DEFINE_SIMPLE_EXCEPTION_NOINNER(OptionValueErrorNotFound, OptionValueError, "options/not_found")
DEFINE_SIMPLE_EXCEPTION_NOINNER(OptionValueErrorInvalidType, OptionValueError, "options/invalid_type")

/// @class OptionValue
/// Holds an actual option.
class OptionValue {
	typedef boost::variant<std::string, int, double, agi::Color, bool, std::vector<std::string>, std::vector<int>, std::vector<double>, std::vector<agi::Color>, std::vector<bool>> type;
	agi::signal::Signal<OptionValue const&> ValueChanged;
	type value;
	type def;
	std::string name;

	template<typename T>
	struct CastVisitor {
		T *value;
		void operator()(T& v) const { value = &v; }
		template<typename Other> void operator()(Other& v) { value = nullptr; }
	};

	void NotifyChanged() { ValueChanged(*this); }

public:
	template<typename ValueType>
	OptionValue(std::string const& name, ValueType const& initial_value)
	: value(initial_value), def(initial_value), name(name) { }
	~OptionValue() { }

	template<typename T>
	T Get() const {
		CastVisitor<const T> visitor;
		boost::apply_visitor(visitor, value);
		if (!visitor.value)
			throw OptionValueErrorInvalidType("Bad type for option value");
		return *visitor.value;
	}

	template<typename T>
	void Set(T const& new_value) {
		CastVisitor<T> visitor;
		boost::apply_visitor(visitor, value);
		if (!visitor.value)
			throw OptionValueErrorInvalidType("Bad type for option value");
		*visitor.value = new_value;
	}

	void Set(const OptionValue *new_value) {
		value = new_value->value;
	}

	std::string GetName() const { return name; }
	void Reset() { value = def; NotifyChanged(); }
	bool IsDefault() const { return value == def; }

	DEFINE_SIGNAL_ADDERS(ValueChanged, Subscribe)

	template <typename Visitor>
	void apply_visitor(Visitor& visitor) {
		return value.apply_visitor(visitor);
	}
};

} // namespace agi
