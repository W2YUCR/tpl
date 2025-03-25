module;

#include <utility>

export module tpl.parser;

import tpl.tokenizer;

export namespace tpl::ast {

class Expr {};

class Number : Expr {};

class Parser {
  public:
	Parser(Tokenizer tokenizer) : _tokenizer{std::move(tokenizer)} {}

  private:
	Tokenizer _tokenizer;
};

} // namespace tpl