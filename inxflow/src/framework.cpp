/*
MIT License

Copyright (c) 2024 Ryan Hechenberger

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <algorithm>
#include <inxflow/data/string_serialize.hpp>
#include <inxflow/framework.hpp>
#include <inxlib/util/functions.hpp>

namespace inx::flow {

namespace {
using namespace std::string_view_literals;
}

VarScope::VarScope(signature&& l_sig,
                   const std::pmr::polymorphic_allocator<>& alloc)
  : sig(std::move(l_sig))
  , global(alloc, signature(sig))
  , local(global)
{
}

bool
Framework::is_short_command_name(char c)
{
	return std::isalpha(static_cast<unsigned char>(c));
}
bool
Framework::is_short_command_name(std::string_view name)
{
	return name.size() >= 2 && name.starts_with('-') &&
	       is_short_command_name(name[1]);
}

bool
Framework::is_general_command_name(std::string_view name)
{
	if (name.empty()) // must not be empty
		return false;
	if (name[0] == '+' || name[0] == '-') // must not start with +-
		return false;
	if (std::ranges::any_of(name, [](unsigned char c) noexcept {
		    return c == '@' || std::iscntrl(c) || std::isspace(c);
	    })) {
		return false;
	}
	return true;
}

Framework::Framework()
{
#if !defined(__cplusplus) || __cplusplus < 202002L
	// should always be C++ 20 or higher, but just in case
	m_exec_run.clear();
#endif
}

int
Framework::exec()
{
	auto is_running = m_exec_run.test_and_set();
	if (is_running)
		throw std::logic_error(
		  "Framework::exec called when m_exec_run is locked.");
	inx::util::destruct_adaptor is_running_lock(
	  [&r = m_exec_run]() noexcept { r.clear(); });
	if (m_arguments.size() == 0)
		return 0;
	if (m_arguments.size() == 1 && m_arguments[0] == "--help") {
		print_help();
		return 0;
	}
	std::vector<std::string> arg_parsed;
	std::vector<std::string_view> arg_pass;
	for (int arg_i = 0, arg_n = m_arguments.size(); arg_i < arg_n;) {
		std::string_view arg = m_arguments[arg_i++];
		auto [cmd_name, cmd_type] = parse_ctrl(arg);
		std::optional<std::string_view> push_arg;
		CommandReg* cmd = nullptr;
		switch (cmd_type) {
		default:
			assert(false);
		case TokenCtrl::Invalid:
			std::cerr << "Invalid command: \"" << arg << "\"" << std::endl;
			return -3;
		case TokenCtrl::General:
			if (auto it = m_general_cmd.find(arg); it != m_general_cmd.end()) {
				cmd = &it->second;
			} else {
				std::cerr << "Invalid general command: \"" << arg << "\""
				          << std::endl;
				return -2;
			}
			break;
		case TokenCtrl::ShortArg:
			push_arg = arg.substr(2);
		case TokenCtrl::Short:
			if (auto it = m_short_cmd.find(arg[1]); it != m_short_cmd.end()) {
				cmd = &it->second;
			} else {
				std::cerr << "Invalid short command: \"" << arg << "\""
				          << std::endl;
				return -2;
			}
			break;
		case TokenCtrl::LocalSep:
			break;
		case TokenCtrl::GlobalSep:
			// clear all local variables
			for (auto& group : m_variables) {
				group.second.local.clear();
			}
			break;
		}

		if (cmd != nullptr) {
			int arg_count = 0;
			arg_parsed.clear();
			if (push_arg) {
				arg_count++;
				auto a = parse_argument(*push_arg);
				if (!a)
					return -3;
				arg_parsed.push_back(*a);
			}
			for (; arg_count != cmd->args_count_override &&
			       arg_i < m_arguments.size();
			     ++arg_i) {
				arg_count++;
				std::string_view arg = m_arguments[arg_i];
				if (arg == "+"sv || arg == "++"sv) {
					break;
				}
				if (arg.starts_with("++"sv)) {
					arg = arg.substr(2);
				}
				auto a = parse_argument(arg);
				if (!a)
					return -3;
				arg_parsed.push_back(*a);
			}
			arg_pass.assign(arg_parsed.begin(), arg_parsed.end());
			int ret = exec_command(*cmd, arg_pass);
			if (ret != 0) // if command unsuccessful, return its error code
				return ret;
		}
	}

	return 0;
}

void
Framework::print_help()
{
	if (m_help_print)
		m_help_print(*this);
}

std::pair<const signature*, bool>
Framework::push_signature(signature&& sig)
{
	auto it = m_signatures.try_emplace(*m_strings.insert(sig->m_name).first,
	                                   std::move(sig));
	return {&it.first->second, it.second};
}

void
Framework::emplace_scope(std::string_view name, signature&& sig)
{
	m_variables.try_emplace(*m_strings.insert(std::string(name)).first,
	                        std::move(sig),
	                        &get_mutable_resource());
}
void
Framework::emplace_scope(std::string_view name, std::string_view sig_name)
{
	emplace_scope(name, signature(m_signatures.at(sig_name)));
}

std::pair<command, bool>
Framework::emplace_command(std::string_view command)
{
	auto it =
	  m_commands.try_emplace(*m_strings.insert(std::string(command)).first);
	if (it.second) {
		it.first->second = std::allocate_shared<cmd::Command>(
		  std::pmr::polymorphic_allocator<>(&get_immutable_resource()));
	}
	return {it.first->second, it.second};
}

bool
Framework::register_short_command(char c,
                                  std::string_view command,
                                  int arguments)
{
	if (auto cmd = m_commands.find(command); cmd != m_commands.end()) {
		return register_short_command(
		  c, cmd->second->shared_from_this(), arguments);
	}
	return false;
}
bool
Framework::register_short_command(char c, command&& command, int arguments)
{
	if (command == nullptr)
		return false;
	if (!is_short_command_name(c))
		return false;
	if (arguments < 0 || arguments < command->get_args_min() ||
	    arguments > command->get_args_max())
		return false;
	CommandReg reg{std::move(command), arguments};
	auto e = m_short_cmd.try_emplace(c, std::move(reg));
	return e.second;
}

bool
Framework::register_general_command(std::string_view name,
                                    std::string_view command)
{
	if (auto cmd = m_commands.find(command); cmd != m_commands.end()) {
		return register_general_command(name, cmd->second->shared_from_this());
	}
	return false;
}
bool
Framework::register_general_command(std::string_view name, command&& command)
{
	if (command == nullptr)
		return false;
	if (!is_general_command_name(name))
		return false;
	CommandReg reg{std::move(command), -1};
	auto e = m_general_cmd.try_emplace(
	  *m_strings.insert(std::string(name)).first, std::move(reg));
	return e.second;
}

int
Framework::exec_command(CommandReg& reg, cmd::command_args args)
{
	if (reg.args_count_override >= 0 && args.size() != reg.args_count_override)
		return -1;
	return reg.cmd->exec(*this, args);
}

auto
Framework::parse_ctrl(std::string_view arg)
  -> std::pair<std::string_view, TokenCtrl>
{
	if (arg.empty())
		return {{}, TokenCtrl::Invalid};
	if (arg == "+"sv)
		return {{}, TokenCtrl::LocalSep};
	if (arg == "++"sv)
		return {{}, TokenCtrl::GlobalSep};
	if (is_short_command_name(arg))
		return {arg.substr(1, 1),
		        arg.size() > 2 ? TokenCtrl::ShortArg : TokenCtrl::Short};
	if (is_general_command_name(arg))
		return {arg, TokenCtrl::General};
	return {{}, TokenCtrl::Invalid};
}
std::optional<std::string>
Framework::parse_argument(std::string_view arg)
{
	auto p = arg.find('@');
	if (p == std::string_view::npos)
		return std::string(arg);
	auto& builder = m_arg_builder;
	builder.clear();
	do {
		builder << arg.substr(0, p);
		arg.remove_prefix(p);
		if (arg.size() == 1) { // ending in @
			builder << '@';
			arg.remove_prefix(1);
		} else if (arg.starts_with("@@"sv)) { // delimited
			builder << '@';
			arg.remove_prefix(2);
		} else {
			auto var = util::parse_varname(arg);
			if (!var.first) {
				std::cerr << "invalid parse of string at "
				          << arg.substr(0, std::max(arg.size(), size_t{20}))
				          << std::endl;
				return std::nullopt;
			}
			if (var.first.op() == util::VarOp::Print) {
				// print var
				if (auto group = var.first.group();
				    !(group.empty() || group == "var"sv)) {
					std::cerr << "print operator must be of group var"
					          << std::endl;
					return std::nullopt;
				}
				try {
					auto& print_var = at(var.first);
					builder << print_var.as<var_string>().str();
				} catch (std::exception&) {
					std::cerr
					  << "invalid print var: " << arg.substr(0, var.second)
					  << std::endl;
					return std::nullopt;
				}
			} else {
				builder << arg.substr(0, var.second);
			}
			arg.remove_prefix(var.second);
		}
		p = arg.find('@');
	} while (p != std::string_view::npos);
	builder << arg;
	return builder.str();
}

VarScope&
Framework::var_group(std::string_view var, std::string_view default_group)
{
	if (var.empty()) {
		var = default_group;
	}
	try {
		return m_variables.at(var);
	} catch (const std::exception&) {
		throw var_group_missing(std::string(var));
	}
}

data::Serialize&
Framework::operator[](util::VarName l_var)
{
	return var(l_var);
}
data::Serialize&
Framework::operator[](std::string_view l_var)
{
	return var(l_var);
}

data::Serialize&
Framework::var(util::VarName l_var, std::string_view default_group)
{
	if (!l_var)
		throw std::runtime_error("invalid variable");
	auto& group = var_group(l_var.group(), default_group);
	switch (l_var.cls()) {
	case util::VarClass::Global:
		return group.global.at_make(l_var.name());
	case util::VarClass::Local:
		return group.local.at_make(l_var.name());
	default:
		throw std::logic_error("l_var.cls()");
	}
}
data::Serialize&
Framework::var(std::string_view l_var, std::string_view default_group)
{
	return var(util::match_varname(l_var, false), default_group);
}

data::Serialize&
Framework::at(util::VarName l_var, std::string_view default_group)
{
	if (!l_var)
		throw std::runtime_error("invalid variable");
	auto& group = var_group(l_var.group(), default_group);
	switch (l_var.cls()) {
	case util::VarClass::Global:
		return group.global.at_chain(l_var.name());
	case util::VarClass::Local:
		return group.local.at_chain(l_var.name());
	default:
		throw std::logic_error("l_var.cls()");
	}
}
data::Serialize&
Framework::at(std::string_view l_var, std::string_view default_group)
{
	return at(util::match_varname(l_var, false), default_group);
}

serialize
Framework::get(util::VarName l_var, std::string_view default_group, int param)
{
	if (!l_var)
		throw std::runtime_error("invalid variable");
	if ((param & vget_group)) {
		if (default_group.empty() ||
		    !(l_var.group().empty() || l_var.group() == default_group))
			throw std::runtime_error(std::string(l_var.group()));
	}
	auto& group = var_group(l_var.group(), default_group);
	data::GroupTemplate* gt;
	switch (l_var.cls()) {
	case util::VarClass::Global:
		gt = &group.global;
		break;
	case util::VarClass::Local:
		gt = &group.local;
		break;
	default:
		throw std::logic_error("l_var.cls()");
	}
	switch (param & static_cast<int>(vget_create | vget_scope)) {
	case 0:
		return gt->get_chain(l_var.name());
	case vget_scope:
		return gt->get(l_var.name());
	case vget_create:
		return gt->get_chain_make(l_var.name());
	case vget_create | vget_scope:
		return gt->get_make(l_var.name());
	}
	assert(false);
#ifdef __GNUC__
	__builtin_unreachable();
#endif
}
serialize
Framework::get(std::string_view l_var,
               std::string_view default_group,
               int param)
{
	return get(util::match_varname(l_var, false), default_group, param);
}

void
framework_default(Framework& fw)
{
	auto* var_sig =
	  fw.emplace_signature<data::SerializeWrap<var_string>>("var"sv).first;
	assert(var_sig != nullptr);
	fw.emplace_scope("var"sv, signature(*var_sig));
	var_sig =
	  fw.emplace_signature<data::SerializeWrap<var_file>>("file"sv).first;
	assert(var_sig != nullptr);
	fw.emplace_scope("file"sv, signature(*var_sig));

	{
		auto var_cmd = fw.emplace_command("inxflow:serialize"sv).first;
		var_cmd->set_args_count(2);
		var_cmd->set_cmd(&command_serialize);
		fw.register_short_command('S', std::move(var_cmd), 2);
	}
	{
		auto var_cmd = fw.emplace_command("inxflow:deserialize"sv).first;
		var_cmd->set_args_count(2);
		var_cmd->set_cmd(&command_deserialize);
		fw.register_short_command('L', std::move(var_cmd), 2);
	}
}

int
command_serialize(Framework& fw, command_args args)
{
	using namespace std::string_view_literals;
	if (args.size() != 2)
		return 1;

	serialize p;
	try {
		p = fw.get(args[0], "file"sv, vget_scope);
	} catch (const std::exception& e) {
		std::cerr << "Failed to retrive variable: " << args[0] << std::endl
		          << e.what() << std::endl;
		return 1;
	}
	try {
		auto path = args[1];
		if (path == "/dev/stdout")
			p->save_stdout();
		else if (path == "/dev/null")
			p->save_null();
		else
			p->save(path);
	} catch (const std::exception& e) {
		std::cerr << "Failed to serialise" << std::endl
		          << e.what() << std::endl;
		return 2;
	} catch (...) {
		std::cerr << "Failed to serialise" << std::endl;
		return 2;
	}
	return 0;
}
int
command_deserialize(Framework& fw, command_args args)
{
	if (args.size() != 2)
		return 1;

	serialize p;
	try {
		p = fw.get(args[0], "file"sv, vget_create | vget_scope);
	} catch (const std::exception& e) {
		std::cerr << "Failed to create variable: " << args[0] << std::endl
		          << e.what() << std::endl;
		return 1;
	}
	try {
		auto path = args[1];
		if (path == "/dev/stdin")
			p->load_stdin();
		else if (path == "/dev/null")
			p->load_null();
		else
			p->load(path);
	} catch (const std::exception& e) {
		std::cerr << "Failed to deserialise" << std::endl
		          << e.what() << std::endl;
		return 2;
	} catch (...) {
		std::cerr << "Failed to deserialise" << std::endl;
		return 2;
	}
	return 0;
}

} // namespace inx::flow
