module;

#include <format>
#include <memory>
#include <stdexcept>
#include <utility>
#include <variant>

export module tpl.parser;

import tpl.tokenizer;
import tpl.ast;
import tpl.util;

export namespace tpl::ast {

class Parser {
  public:
	Parser(lex::Tokenizer tokenizer) : _tokenizer{std::move(tokenizer)} {}

	std::unique_ptr<Expr> parse() { return parse_expr(); }

  private:
	std::unique_ptr<Expr> parse_expr()
	{
		auto lhs = parse_atom();
		if (_tokenizer != std::default_sentinel and not match<lex::RParen>()) {
			auto func = parse_atom();
			auto rhs = parse_expr();
			return std::make_unique<BinaryOp>(std::move(func), std::move(lhs), std::move(rhs));
		}
		return lhs;
	}

	std::unique_ptr<Expr> parse_atom()
	{
		return std::visit(
			overloaded{
				[&](lex::Identifier const &ident) -> std::unique_ptr<Expr> {
					auto ret = std::make_unique<Variable>(ident.name);
					++_tokenizer;
					return ret;
				},
				[&](lex::Number const &number) -> std::unique_ptr<Expr> {
					auto ret = std::make_unique<Number>(number.value);
					++_tokenizer;
					return ret;
				},
				[&](lex::LParen const &) -> std::unique_ptr<Expr> {
					++_tokenizer;
					auto ret = parse_expr();
					eat<lex::RParen>("closing parenthesis");
					return ret;
				},
				[](auto const &) -> std::unique_ptr<Expr> { throw std::runtime_error{std::format("Invalid")}; },
			},
			current());
	}

	lex::Token const &current()
	{
		if (_tokenizer == std::default_sentinel) {
			throw std::runtime_error{std::format("Unexpected end of input")};
		}
		return *_tokenizer;
	}

	template <class T>
	bool match()
	{
		return std::holds_alternative<T>(current());
	}

	template <class T>
	void eat(char const *token_names)
	{
		if (match<T>()) {
			++_tokenizer;
			return;
		}
		throw std::runtime_error{std::format("Expected {}", token_names)};
	}

	lex::Tokenizer _tokenizer;
};

} // namespace tpl::ast