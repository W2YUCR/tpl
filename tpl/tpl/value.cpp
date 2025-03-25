module;

#include <format>
#include <functional>
#include <string>
#include <variant>

export module tpl.runtime:value;

import tpl.util;

export namespace tpl::runtime {

struct Value;

using ValueT = std::variant<std::int64_t, std::string, std::function<Value(Value, Value)>>;

struct Value : ValueT {
	using ValueT::variant;
};

} // namespace tpl::runtime

template <>
struct std::formatter<tpl::runtime::Value> : std::formatter<std::string_view> {
	auto format(tpl::runtime::Value const& value, auto &ctx) const
	{
		return std::visit(tpl::overloaded{
			[&](std::int64_t const &number) { return std::format_to(ctx.out(), "{}", number); },
			[&](std::string const &str) { return std::format_to(ctx.out(), "{}", str); },
			[&](std::function<tpl::runtime::Value(tpl::runtime::Value, tpl::runtime::Value)>) {
				return std::format_to(ctx.out(), "function");
			},
		}, value);
	}
};