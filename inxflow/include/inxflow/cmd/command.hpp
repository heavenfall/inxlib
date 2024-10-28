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

#ifndef INXFLOW_CMD_COMMAND_HPP
#define INXFLOW_CMD_COMMAND_HPP

#include "types.hpp"
#include <functional>
#include <limits>
#include <memory>

namespace inx::flow {

namespace cmd {

class Command : public std::enable_shared_from_this<Command>
{
public:
	Command()
	  : m_args_min(0)
	  , m_args_max(0)
	{
	}

	int exec(Framework& fw, command_args args)
	{
		if (args.size() < m_args_min || args.size() > m_args_max)
			return -1;
		return m_cmd(fw, args);
	}

	template <typename... Args>
	void set_cmd(Args&&... args)
	{
		m_cmd = std::function<command_exec>(std::forward<Args>(args)...);
	}

	void set_args_count(int args_exact) noexcept
	{
		if (args_exact < 0)
			return;
		m_args_min = m_args_max = args_exact;
	}
	void set_args_range(int args_min, int args_max = std::numeric_limits<int>::max()) noexcept
	{
		if (args_min < 0 || args_max < args_min) {
			return; // TODO: figure out user error system
		}
		m_args_min = args_min;
		m_args_max = args_max;
	}

	int get_args_min() const noexcept { return m_args_min; }
	int get_args_max() const noexcept { return m_args_max; }

protected:
	std::function<command_exec> m_cmd;
	int m_args_min, m_args_max;
};

} // namespace cmd

using command = std::shared_ptr<cmd::Command>;

} // namespace inx::flow

#endif // INXFLOW_CMD_COMMAND_HPP
