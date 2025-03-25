module;

#include <format>
#include <memory>
#include <stdexcept>
#include <utility>
#include <variant>

export module tpl.parser;

import tpl.tokenizer;
import tpl.util;

export namespace tpl::ast {

struct Number;
struct Variable;

struct ExprVisitor {
	virtual ~ExprVisitor() = default;
	virtual void visit(Number &) = 0;
	virtual void visit(Variable &) = 0;
};

struct Expr {
  public:
	virtual ~Expr() = default;
	virtual void accept(ExprVisitor &) = 0;
};

template <class T>
struct ExprCRTP : Expr {
  public:
	void accept(ExprVisitor &visitor) override { visitor.visit(static_cast<T &>(*this)); }
};

struct Number : ExprCRTP<Number> {
	Number(std::int64_t value) : value{value} {}
	std::int64_t value;
};

struct Variable : ExprCRTP<Variable> {
	Variable(std::string name) : name{std::move(name)} {}
	std::string name;
};

class Parser {
  public:
	Parser(lex::Tokenizer tokenizer) : _tokenizer{std::move(tokenizer)} {}

	std::unique_ptr<Expr> parse() { return parse_atom(); }

  private:
	std::unique_ptr<Expr> parse_atom()
	{
		return std::visit(
			overloaded{
				[&](lex::Identifier const &ident) -> std::unique_ptr<Expr> {
					++_tokenizer;
					return std::make_unique<Variable>(ident.name);
				},
				[&](lex::Number const &number) -> std::unique_ptr<Expr> {
					++_tokenizer;
					return std::make_unique<Number>(number.value);
				},
				[](auto const &) -> std::unique_ptr<Expr> { throw std::runtime_error{std::format("Invalid")}; },
			},
			*_tokenizer);
	}

	template <class T>
	void eat(char const *token_names)
	{
		if (T *value = std::get_if<T>(*_tokenizer)) {
			++_tokenizer;
		}
		throw std::runtime_error{std::format("Expected {}", token_names)};
	}

	lex::Tokenizer _tokenizer;
};

} // namespace tpl::ast