#include <inxflow/framework.hpp>

int
main(int argc, char* argv[])
{
	using namespace std::string_view_literals;
	constexpr auto npos = std::string_view::npos;
	inx::flow::Framework fw;
	inx::flow::framework_default(fw);
	for (int i = 1; i < argc; ++i) {
		auto arg = std::string_view(argv[i]);
		if (auto pos = arg.find('='); pos != npos) {
			auto& var = fw.var(arg.substr(0, pos), "var"sv);
			auto& str = var.as<inx::flow::var_string>();
			str = arg.substr(pos + 1);
			std::cout << "SET " << arg.substr(0, pos) << " = \"" << str.view()
			          << '\"' << std::endl;
		} else {
			auto [var_name, var_end] = inx::flow::util::parse_varname(arg);
			if (!var_name) {
				std::cerr << "UKNOWN VAR " << arg << std::endl;
				continue;
			}
			auto& var = fw.at(var_name, "var"sv);
			auto& str = var.as<inx::flow::var_string>();
			std::cout << (var_name.op() == inx::flow::util::VarOp::Name
			                ? "NAME "
			                : "PRINT ")
			          << var_name.name() << " \"" << str.view() << "\""
			          << std::endl;
			if (var_end != arg.length()) {
				std::cout << "UNPARSED STRING " << arg.substr(var_end)
				          << std::endl;
			}
		}
	}

	return 0;
}
