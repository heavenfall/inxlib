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

#ifndef INXLIB_DATA_BINARY_TREE_HPP
#define INXLIB_DATA_BINARY_TREE_HPP

#include <inxlib/inx.hpp>

#include "mary_tree.hpp"

namespace inx::data {

class binary_tree_base;
struct binary_tree_node;

struct binary_tree_node : mary_tree_node<2>
{
	using self = binary_tree_node;
	using super = mary_tree_node<2>;
	using binary_node_type = self;

	binary_node_type* left() noexcept { return static_cast<binary_node_type*>(this->m_nData.children[0]); }
	const binary_node_type* left() const noexcept { return static_cast<binary_node_type*>(this->m_nData.children[0]); }
	binary_node_type* right() noexcept { return static_cast<binary_node_type*>(this->m_nData.children[1]); }
	const binary_node_type* right() const noexcept { return static_cast<binary_node_type*>(this->m_nData.children[1]); }
	binary_node_type* first() noexcept
	{
		return static_cast<binary_node_type*>(this->m_nData.children[0] ? this->m_nData.children[0]
		                                                                : this->m_nData.children[1]);
	}
	const binary_node_type* first() const noexcept
	{
		return static_cast<binary_node_type*>(this->m_nData.children[0] ? this->m_nData.children[0]
		                                                                : this->m_nData.children[1]);
	}
	binary_node_type* second() noexcept
	{
		return static_cast<binary_node_type*>(this->m_nData.children[0] ? this->m_nData.children[1] : nullptr);
	}
	const binary_node_type* second() const noexcept
	{
		return static_cast<binary_node_type*>(this->m_nData.children[0] ? this->m_nData.children[1] : nullptr);
	}
	binary_node_type* single() noexcept
	{
		assert(!is_deg2());
		return static_cast<binary_node_type*>(
		  reinterpret_cast<node_type*>(reinterpret_cast<intptr_t>(this->m_nData.children[0]) |
		                               reinterpret_cast<intptr_t>(this->m_nData.children[1])));
	}
	const binary_node_type* single() const noexcept
	{
		assert(!is_deg2());
		return static_cast<binary_node_type*>(
		  reinterpret_cast<node_type*>(reinterpret_cast<intptr_t>(this->m_nData.children[0]) |
		                               reinterpret_cast<intptr_t>(this->m_nData.children[1])));
	}

	binary_node_type& rotate_id(size_t i) noexcept // 0 == left, 1 == right
	{
		assert(i < 2);
		assert(child(i ^ 1) != nullptr);
		binary_node_type& r = static_cast<self&>(*child(i ^ 1));
		this->connect_child_auto(r.child(i),
		                         i ^ 1); // node.right() = r.left();
		r.connect_child(*this, i);       // r.left() = &node;
		assert(children_connected());
		assert(r.children_connected());
		return r;
	}

	binary_node_type& rotate_left() noexcept { return rotate_id(0); }

	binary_node_type& rotate_right() noexcept { return rotate_id(1); }

	void replace_child(binary_node_type& node1, binary_node_type* node2) noexcept
	{
		this->m_nData.children[get_child_id(node1)] = node2;
	}

	size_t get_child_id(binary_node_type& child) noexcept
	{
		assert(this->m_nData.children[0] == &child || this->m_nData.children[1] == &child);
		return static_cast<size_t>(this->m_nData.children[0] != &child);
	}

	binary_node_type* find_inorder_id(size_t id) noexcept // find next inorder succ (0) or pred(1)
	{
		return const_cast<binary_node_type*>(std::as_const(*this).find_inorder_id(id));
	}
	const binary_node_type* find_inorder_id(size_t id) const noexcept // find next inorder succ (0) or pred(1)
	{
		if (this->m_nData.children[id ^ 1] != nullptr) {
			return &trace_inorder_id(id);
		} else {
			const binary_node_type* C = this;
			const binary_node_type* P = static_cast<const binary_node_type*>(this->parent());
			while (P != nullptr) {
				if (P->child(id) == C)
					break;
				C = P;
				P = static_cast<const binary_node_type*>(P->parent());
			}
			return P;
		}
	}

	binary_node_type& trace_inorder_id(size_t id) noexcept // 0 = in-order succ, 1 = in-order pred
	{
		return const_cast<binary_node_type&>(std::as_const(*this).trace_inorder_id(id));
	}
	const binary_node_type& trace_inorder_id(size_t id) const noexcept // 0 = in-order succ, 1 = in-order pred
	{
		assert(this->m_nData.children[id ^ 1] != nullptr);
		const binary_node_type* ans = static_cast<const binary_node_type*>(this->m_nData.children[id ^ 1]);
		while (ans->m_nData.children[id] != nullptr) {
			ans = static_cast<const binary_node_type*>(ans->m_nData.children[id]);
		}
		return *ans;
	}

	bool is_deg1() const noexcept { return (left() != nullptr) != (right() != nullptr); }
	bool is_deg2() const noexcept { return left() != nullptr && right() != nullptr; }
};

class binary_tree_base : public mary_tree_base<2>
{
public:
	using self = binary_tree_base;
	using super = mary_tree_base<2>;
	using value_type = binary_tree_node;

	value_type& front() noexcept { return const_cast<value_type&>(std::as_const(*this).front()); }
	const value_type& front() const noexcept
	{
		assert(this->m_root != nullptr);
		const value_type* ans = static_cast<const value_type*>(this->m_root);
		while (ans->left() != nullptr) {
			ans = ans->left();
		}
		return static_cast<const value_type&>(*ans);
	}
	value_type& back() noexcept { return const_cast<value_type&>(std::as_const(*this).back()); }
	const value_type& back() const noexcept
	{
		assert(this->m_root != nullptr);
		const value_type* ans = static_cast<const value_type*>(this->m_root);
		while (ans->right() != nullptr) {
			ans = ans->right();
		}
		return static_cast<const value_type&>(*ans);
	}

	void rotate_id(value_type& node, size_t i)
	{
		assert(i < 2);
		auto* parent = node.m_nData.parent;
		auto& n = node.rotate_id(i);
		n.m_nData.parent = parent;
		if (parent != nullptr) {
			static_cast<value_type&>(*parent).replace_child(node, &n);
			assert(parent->children_connected());
		} else {
			this->m_root = &n;
			assert(n.is_root());
		}
	}

	void rotate_left(value_type& node) { rotate_id(node, 0); }

	void rotate_right(value_type& node) { rotate_id(node, 1); }
};

} // namespace inx::data

#endif // INXLIB_DATA_BINARY_TREE_HPP
