module;

#include <format>
#include <functional>
#include <string>
#include <variant>

export module tpl.runtime:value;

import tpl.ast;
import tpl.util;

export namespace tpl::runtime {

struct Value;

using FunctionValue = std::function<Value(ast::Expr &, ast::Expr &)>;

using ValueT = std::variant<std::int64_t, std::string, FunctionValue>;

struct Value : ValueT {
	using ValueT::variant;
};

} // namespace tpl::runtime

template <>
struct std::formatter<tpl::runtime::Value> : std::formatter<std::string_view> {
	auto format(tpl::runtime::Value const &value, auto &ctx) const
	{
		return std::visit(
			tpl::overloaded{
				[&](std::int64_t const &number) { return std::format_to(ctx.out(), "{}", number); },
				[&](std::string const &str) { return std::format_to(ctx.out(), "{}", str); },
				[&](tpl::runtime::FunctionValue const &) { return std::format_to(ctx.out(), "function"); },
			},
			value);
	}
};