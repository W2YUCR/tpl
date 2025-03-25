module;

#include <cassert>
#include <charconv>
#include <cstddef>
#include <iterator>
#include <string>
#include <variant>

export module tpl.tokenizer;

export namespace tpl::lex {

struct Identifier {
	std::string name;
};

struct Number {
	double value;
};

struct LParen {};
struct RParen {};

using Token = std::variant<Identifier, Number, LParen, RParen>;

class Tokenizer {
  public:
	using difference_type = std::ptrdiff_t;
	using value_type = Token;

	Tokenizer(char const *begin, char const *end) : _iter{begin}, _end{end} { ++*this; }

	Tokenizer &operator++()
	{
		while (_iter != _end) {
			if (std::isspace(*_iter) != 0) {
				++_iter;
				continue;
			}
			if (*_iter == '(') {
				++_iter;
				_token = LParen{};
				return *this;
			}
			if (*_iter == ')') {
				++_iter;
				_token = RParen{};
				return *this;
			}
			if ('0' <= *_iter and *_iter <= '9') {
				get_number();
				return *this;
			}
			get_identifier();
			return *this;
		}
		_at_end = true;
		return *this;
	}

	void get_identifier()
	{
		char const *start = _iter;
		while (_iter != _end and not std::isspace(*_iter) and *_iter != '(' and *_iter != ')') {
			++_iter;
		}
		_token = Identifier{std::string{start, _iter}};
	}

	void get_number()
	{
		char const *start = _iter;
		while (_iter != _end and '0' <= *_iter and *_iter <= '9') {
			++_iter;
		}
		if (_iter != _end and *_iter == '.') {
			++_iter;
			while (_iter != _end and '0' <= *_iter and *_iter <= '9') {
				++_iter;
			}
		}
		double value;
		auto [_, ec] = std::from_chars(start, _iter, value);
		assert(ec == std::errc{});
		_token = Number{value};
	}

	void operator++(int) { ++*this; }

	Token const &operator*() const { return _token; }

	bool operator==(std::default_sentinel_t const &) const { return _at_end; }

  private:
	Token _token;
	char const *_iter;
	char const *_end;
	bool _at_end{false};
};

static_assert(std::input_iterator<Tokenizer>);
}; // namespace tpl::lex