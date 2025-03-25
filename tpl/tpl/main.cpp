module;

#include <iostream>
#include <print>
#include <string>
#include <string_view>
#include <variant>

export module tpl;

import tpl.tokenizer;
import tpl.parser;
import tpl.interpreter;
import tpl.util;

namespace {

int run(std::string_view filename)
{
	return EXIT_SUCCESS;
}

int repl()
{
	std::string line;
	tpl::runtime::Interpreter interpreter;
	while (std::print("> "), std::getline(std::cin, line)) {
		try {
			tpl::lex::Tokenizer tokenizer{line.data(), line.data() + line.size()};
			tpl::ast::Parser parser{tokenizer};

			auto ast = parser.parse();

			std::visit(
				tpl::overloaded{
					[](std::int64_t const &number) { std::println("{}", number); },
					[](std::string const &str) { std::println("{}", str); },
				},
				interpreter.interpret(*ast));
		}
		catch (std::exception &ex) {
			std::println("Error: {}", ex.what());
		}
	}
	return EXIT_SUCCESS;
}

} // namespace

int main(int argc, char const *argv[])
{
	switch (argc) {
	case 1: return repl();
	case 2: return run(argv[1]);
	case 3: std::println(stderr, "Usage: tpl [filename]"); return EXIT_FAILURE;
	}
}