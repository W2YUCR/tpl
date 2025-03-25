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
struct BinaryOp;

struct ExprVisitor {
	virtual ~ExprVisitor() = default;
	virtual void visit(Number &) = 0;
	virtual void visit(Variable &) = 0;
	virtual void visit(BinaryOp &) = 0;
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

struct BinaryOp : ExprCRTP<BinaryOp> {
	BinaryOp(std::unique_ptr<Expr> func, std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> rhs) :
		func{std::move(func)},
		lhs{std::move(lhs)},
		rhs{std::move(rhs)}
	{
	}
	std::unique_ptr<Expr> func;
	std::unique_ptr<Expr> lhs;
	std::unique_ptr<Expr> rhs;
};

class Parser {
  public:
	Parser(lex::Tokenizer tokenizer) : _tokenizer{std::move(tokenizer)} {}

	std::unique_ptr<Expr> parse() { return parse_expr(); }

  private:
	std::unique_ptr<Expr> parse_expr()
	{
		auto lhs = parse_atom();
		if (_tokenizer != std::default_sentinel) {
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