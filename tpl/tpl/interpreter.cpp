module;

#include <format>
#include <functional>
#include <map>
#include <memory>
#include <stack>
#include <string>
#include <variant>

export module tpl.runtime;

import tpl.ast;
import tpl.util;

export import :value;

export namespace tpl::runtime {

class Scope {
  public:
	Scope(std::unique_ptr<Scope> parent, std::map<std::string, Value, std::less<>> values = {}) :
		_parent{std::move(parent)},
		_values{std::move(values)}
	{
	}

	Value &lookup(std::string_view name)
	{
		auto iter = _values.find(name);
		if (iter != _values.end()) {
			return iter->second;
		}
		if (_parent != nullptr) {
			return _parent->lookup(name);
		}
		return _values[std::string{name}];
	}

	void insert(std::string name, Value value) { _values.emplace(name, value); }

	std::unique_ptr<Scope> &parent() { return _parent; }

  private:
	std::unique_ptr<Scope> _parent;
	std::map<std::string, Value, std::less<>> _values;
};

class Interpreter : public ast::ExprVisitor {
  public:
	Value interpret(ast::Expr &ast)
	{
		ast.accept(*this);
		return values.top();
	}

	void visit(ast::Number &number) override { values.emplace(number.value); }
	void visit(ast::Variable &var) override { values.emplace(std::reference_wrapper<Value>{scope->lookup(var.name)}); }
	void visit(ast::BinaryOp &expr) override
	{
		Value center = eval(*expr.func).unwrap();

		if (FunctionValue *ptr = std::get_if<FunctionValue>(&center)) {
			FunctionValue func = std::move(*ptr);
			values.push(func(expr.lhs, expr.rhs));
		}
		else {
			throw std::runtime_error{std::format("Attempted to call a non-function: {}", center)};
		}
	}

  private:
	template <class Op>
	class AutoOp {
	  public:
		AutoOp(Interpreter *interpreter, char const *name) : _interpreter{interpreter}, _name{name} {}
		auto operator()(std::unique_ptr<ast::Expr> &lhs, std::unique_ptr<ast::Expr> &rhs) -> Value
		{
			return std::visit(overloaded{
				[](auto lhs, auto rhs) -> Value
					requires requires { Op{}(lhs, rhs); }
				{ return Op{}(lhs, rhs); },
				[this](Value lhs, Value rhs) -> Value {
					throw std::runtime_error{std::format("Cannot call {} with {} and {}", _name, lhs, rhs)};
				},
				},
				_interpreter->eval(*lhs).unwrap(),
				_interpreter->eval(*rhs).unwrap());
		};

	  private:
		Interpreter *_interpreter;
		char const *_name;
	};

	std::unique_ptr<Scope> scope = std::make_unique<Scope>(
		nullptr,
		std::map<std::string, Value, std::less<>>{
			{"+", AutoOp<std::plus<>>(this, "plus")},
			{"-", AutoOp<std::minus<>>{this, "minus"}},
			{"/", AutoOp<std::divides<>>{this, "divide"}},
			{"*", AutoOp<std::multiplies<>>{this, "multiply"}},
			{"object", std::map<std::string, Value>{}},
			{"of",
			 [&](std::unique_ptr<ast::Expr> &lhs, std::unique_ptr<ast::Expr> &rhs) -> Value {
				 auto *member = dynamic_cast<ast::Variable *>(lhs.get());
				 if (member == nullptr) {
					 throw std::runtime_error{std::format("The LHS is not a variable")};
				 }
				 Value object = eval(*rhs);
				 auto *ref = std::get_if<std::reference_wrapper<Value>>(&object);
				 if (ref == nullptr) {
					 throw std::runtime_error{std::format("The RHS is not a reference (RHS = {})", object)};
				 }
				 auto *map = std::get_if<std::map<std::string, Value>>(&ref->get().unwrap());
				 if (map == nullptr) {
					 throw std::runtime_error{std::format("The RHS is not an object (RHS = {})", ref->get())};
				 }
				 return std::reference_wrapper<Value>{(*map)[member->name]};
			 }},
			{"=",
			 [&](std::unique_ptr<ast::Expr> &lhs, std::unique_ptr<ast::Expr> &rhs) -> Value {
				 Value ref = eval(*lhs);
				 if (auto *var = std::get_if<std::reference_wrapper<Value>>(&ref)) {
					 return var->get() = eval(*rhs);
				 }
				 else {
					 throw std::runtime_error{std::format("The LHS is not assignable")};
				 }
			 }},
			{"func",
			 [&](std::unique_ptr<ast::Expr> &lhs, std::unique_ptr<ast::Expr> &rhs) -> Value {
				 std::shared_ptr<ast::Expr> body = std::move(rhs);
				 return std::function(
					 [this, body](std::unique_ptr<ast::Expr> &lhs, std::unique_ptr<ast::Expr> &rhs) -> Value {
						 scope = std::make_unique<Scope>(std::move(scope));
						 scope->insert("LHS", eval(*lhs).unwrap());
						 scope->insert("RHS", eval(*rhs).unwrap());
						 auto value = eval(*body);
						 scope = std::move(scope->parent());
						 return value;
					 });
			 }},
		});

	Value eval(ast::Expr &expr)
	{
		expr.accept(*this);
		Value value = values.top();
		values.pop();
		return value;
	}

	std::stack<Value> values;
};

} // namespace tpl::runtime
