module;

#include <memory>
#include <string>

export module tpl.ast;

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

} // namespace tpl::ast