#include <inxflow/framework.hpp>

int
main(int argc, char* argv[])
{
	constexpr auto npos = std::string_view::npos;
	inx::flow::Framework fw;
	inx::flow::framework_default(fw);
	for (int i = 1; i < argc; ++i) {
		auto arg = std::string_view(argv[i]);
		if (auto pos = arg.find('='); pos != npos) {
			auto& var = fw.var(arg.substr(0, pos), "var"sv);
		}
	}
}
