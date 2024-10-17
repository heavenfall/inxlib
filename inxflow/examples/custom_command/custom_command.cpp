#include <inxflow/framework.hpp>
#include <vector>

using inx::flow::command_args;
using inx::flow::Framework;

using namespace std::string_view_literals;

template <typename T>
void
vector_save(const std::vector<T>& vec, std::ostream& out)
{
	for (auto& i : vec) {
		out << i << '\n';
	}
	out << std::endl;
}
template <typename T>
void
vector_load(std::vector<T>& vec, std::istream& out)
{
	T val;
	vec.clear();
	while (out >> val)
		vec.push_back(val);
}

using vec_type = std::vector<double>;

int
add(Framework& fw, command_args args)
{
	if (args.size() != 2)
		return 1;
	auto v = fw.get(args[0], "vec"sv, inx::flow::vget_group);
	if (v == nullptr)
		return 2;
	vec_type vec = v->as<vec_type>();
	double mod_val = 0;
	std::string_view mod_val_str = args[1];
	auto vname = inx::flow::util::match_varname(mod_val_str, true);
	if (vname) {
		mod_val_str = fw.at(vname, "var"sv).as<inx::flow::var_string>().str();
	}
	mod_val = std::stod(std::string(mod_val_str));

	for (auto& i : vec) {
		i += mod_val;
	}

	fw["vec.output"].as<vec_type>() = std::move(vec);
	return 0;
}

int
main(int argc, char* argv[])
{
	inx::flow::Framework fw;
	inx::flow::framework_default(fw);
	using sig_type =
	  inx::flow::data::SerializeWrap<vec_type,
	                                 inx::flow::data::SerMode::Auto,
	                                 &vector_load<vec_type::value_type>,
	                                 &vector_save<vec_type::value_type>>;
	fw.emplace_signature<sig_type>("vecdouble");
	fw.emplace_scope("vec", "vecdouble");
	auto cmd = fw.emplace_command("add").first;
	cmd->set_cmd(&add);
	cmd->set_args_count(2);
	fw.register_general_command("add", std::move(cmd));

	fw.set_args_main(argc, argv);

	fw.set_help_print([]() {
		std::cout << "Usage: custom_command {commands}\n\n"
		             "{commands}:\n"
		             "Normal inxflow commands (-S,-L,ect)\n"
		             "add @vec:@ <value:double>: add value to whole vector, "
		             "then stores result in @vec:output@\n"
		          << std::flush;
	});

	return fw.exec();
}
