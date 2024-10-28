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

#ifndef INXFLOW_FRAMEWORK_HPP
#define INXFLOW_FRAMEWORK_HPP

#include "cmd/command.hpp"
#include "data/group_template.hpp"
#include "data/string_serialize.hpp"
#include "types.hpp"
#include "util/string.hpp"
#include <atomic>
#include <inxlib/inx.hpp>
#include <memory_resource>
#include <ranges>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace inx::flow {

using cmd::command_args;

struct VarScope
{
	VarScope(signature&& l_sig, const std::pmr::polymorphic_allocator<>& alloc);
	signature sig;
	data::GroupTemplate global;
	data::GroupTemplate local;
};

struct CommandReg
{
	command cmd;
	int args_count_override; /// overrides the amount of arguments command takes
	                         /// to this, -1 to use command default.
};

using var_string = data::StringSerialize;
using var_file = data::StringSerialize;

enum VarGet
{
	vget_get = 0,
	vget_scope =
	  1 << 0, /// do not chain variable, get at correct scope global/local
	vget_create = 1 << 1, /// if var does not exists, create it
	vget_group =
	  1 << 2, /// make default_group the required group, throws if invalidated
};

class Framework
{
public:
	// program run

	enum class TokenCtrl
	{
		Invalid,   // invalid control token
		Short,     // short command i.e. "-L"
		ShortArg,  // short command including arg1, i.e. "-Lfile.txt"
		General,   // general command name
		LocalSep,  // "+"
		GlobalSep, // "++"
	};

	static bool is_short_command_name(char c);
	static bool is_short_command_name(std::string_view);
	static bool is_general_command_name(std::string_view);

	Framework();

	void set_args_main(int argc, char* argv[])
	{
		auto is_running = m_exec_run.test_and_set();
		if (is_running)
			throw std::logic_error(
			  "Framework::set_args_main called when m_exec_run is locked.");
		inx::util::destruct_adaptor is_running_lock(
		  [&r = m_exec_run]() noexcept { r.clear(); });
		if (argc < 1)
			throw std::out_of_range("argc");
		m_arguments.assign(argv + 1, argv + argc);
	}
	template <std::ranges::input_range G>
	void set_args_range(G&& args)
	{
		auto is_running = m_exec_run.test_and_set();
		if (is_running)
			throw std::logic_error(
			  "Framework::set_args_range called when m_exec_run is locked.");
		inx::util::destruct_adaptor is_running_lock(
		  [&r = m_exec_run]() noexcept { r.clear(); });
		m_arguments.assign(args.begin(), args.end());
	}
	template <typename HelpFn>
	void set_help_print(HelpFn&& func)
	{
		if constexpr (std::is_invocable_v<HelpFn, Framework&>) {
			m_help_print = std::forward<HelpFn>(func);
		} else {
			m_help_print = std::bind(std::forward<HelpFn>(func));
		}
	}

	void print_help();

	/// @brief Run command from arguments.  Args from set_args_main or
	/// set_args_range
	/// @return exit code
	int exec();

	/// @brief A memory_resource that is never freed, not thread safe
	/// @return monotonic_buffer_resource
	std::pmr::memory_resource& get_immutable_resource() noexcept
	{
		return m_immRes;
	}
	/// @brief A memory_resource that is never freed, thread safe
	/// @return synchronized_pool_resource
	std::pmr::memory_resource& get_mutable_resource() noexcept
	{
		return m_mutRes;
	}

	// program variables

	template <data::concepts::serializable T, typename... Args>
	std::pair<const signature*, bool> emplace_signature(std::string_view name,
	                                                    Args&&... args)
	{
		if (auto it = m_signatures.find(name); it != m_signatures.end())
			return {&it->second, false};
		auto base_obj = std::allocate_shared<T>(
		  std::pmr::polymorphic_allocator<>(&get_mutable_resource()),
		  std::forward<Args>(args)...);
		signature sig = std::allocate_shared<signature::element_type>(
		  std::pmr::polymorphic_allocator<>(&get_immutable_resource()),
		  std::string(name),
		  std::move(base_obj),
		  &get_mutable_resource());
		return push_signature(std::move(sig));
	}
	std::pair<const signature*, bool> push_signature(signature&& sig);
	const signature* get_signature(std::string_view name)
	{
		if (auto it = m_signatures.find(name); it != m_signatures.end())
			return &it->second;
		return nullptr;
	}

	void emplace_scope(std::string_view name, signature&& sig);
	void emplace_scope(std::string_view name, std::string_view sig_name);

	std::pair<command, bool> emplace_command(std::string_view name);

	/**
	 * Register command with the short syntax, e.g. -c arg1 arg2
	 * @param c Control name, constrain must be std::isalpha, case sensitive.
	 * @param command Get command by name
	 * @param arguments Number of arguments, must be >= 0. This command only
	 * works with given exact number of arguments.
	 * @return true on successful registration, false if failed as either `c`
	 * exists, command does not exist or arguments does not fit within command
	 * arguments
	 */
	bool register_short_command(char c,
	                            std::string_view command,
	                            int arguments);
	bool register_short_command(char c, command&& command, int arguments);

	/**
	 * Register general command.  Runs as name arg1 arg2...
	 * @param name Exec name, must not contain std::iscntrl or std::isspace or
	 * '@' characters, can not start with any characters "-+".
	 * @param command Get command by name
	 * @return true on successful registration, false if name is invalid or
	 * taken, or command does not exist
	 */
	bool register_general_command(std::string_view name,
	                              std::string_view command);
	bool register_general_command(std::string_view name, command&& command);

	/**
	 * Returns varable var.
	 * Requires an explicit group that exists.
	 * Will create variable name if not exists.
	 * Calls var(l_var)
	 */
	data::Serialize& operator[](util::VarName l_var);
	/**
	 * Calls var(l_var)
	 */
	data::Serialize& operator[](std::string_view l_var);

	/**
	 * Returns varable l_var at specified local/global scope.
	 * If no group exists and default_group is specified, use that group,
	 * Requires group to exists. Will create variable name if not exists.
	 */
	data::Serialize& var(util::VarName l_var,
	                     std::string_view default_group = std::string_view());
	data::Serialize& var(std::string_view l_var,
	                     std::string_view default_group = std::string_view());

	/**
	 * Returns varable l_var. If local scope, try local first, then try global.
	 * If no group exists and default_group is specified, use that group,
	 * Requires group to exists. Will not create variable name.
	 */
	data::Serialize& at(util::VarName l_var,
	                    std::string_view default_group = std::string_view());
	data::Serialize& at(std::string_view l_var,
	                    std::string_view default_group = std::string_view());

	/**
	 * Returns varable l_var. If local scope, try local first, then try global.
	 * If no group exists and default_group is specified, use that group,
	 * Requires group to exists. Will not create variable name (returns nullptr
	 * instead). These rules are changed based on param. vget_create will create
	 * the variable if not exists. vget_group enforces default_group (must be
	 * set). vget_scope will explicitly use varable at local/global scope
	 * specified.
	 */
	serialize get(util::VarName l_var,
	              std::string_view default_group = std::string_view(),
	              int param = vget_get);
	serialize get(std::string_view l_var,
	              std::string_view default_group = std::string_view(),
	              int param = vget_get);

	VarScope& var_group(std::string_view l_var,
	                    std::string_view default_group = std::string_view());

protected:
	int exec_command(CommandReg& reg, cmd::command_args);

	/// @brief Parse an argument as a ctrl token, e.g. short/general command and
	/// command seperators
	/// @param arg argument to parse
	/// @return pair of parsed value, string_view if a command containing the
	/// name, token for type
	std::pair<std::string_view, TokenCtrl> parse_ctrl(std::string_view arg);

	/// @brief Parse an argument, performs @ print statements
	/// @param arg argument to parse
	/// @return the formatted string
	std::optional<std::string> parse_argument(std::string_view arg);

protected:
	std::pmr::monotonic_buffer_resource m_immRes;
	std::pmr::synchronized_pool_resource m_mutRes;
	std::vector<std::string> m_arguments;
	std::pmr::unordered_set<std::string> m_strings;
	std::pmr::unordered_map<std::string_view, signature> m_signatures;
	std::pmr::unordered_map<std::string_view, VarScope> m_variables;
	std::pmr::unordered_map<std::string_view, command> m_commands;
	std::pmr::unordered_map<char, CommandReg> m_short_cmd;
	std::pmr::unordered_map<std::string_view, CommandReg> m_general_cmd;
	std::atomic_flag m_exec_run;
	std::ostringstream m_arg_builder;
	std::function<void(Framework&)> m_help_print;
};

/**
 * Setup the default layout of framework.
 */
void
framework_default(Framework& fw);

int
command_serialize(Framework& fw, command_args args);
int
command_deserialize(Framework& fw, command_args args);
int
command_define(Framework& fw, command_args args);
int
command_var(Framework& fw, command_args args);

} // namespace inx::flow

#endif // INXFLOW_FRAMEWORK_HPP
