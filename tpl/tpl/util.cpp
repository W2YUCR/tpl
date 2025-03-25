export module tpl.util;

export namespace tpl {

template <class... Ts>
struct overloaded : Ts... {
	using Ts::operator()...;
};

} // namespace tpl