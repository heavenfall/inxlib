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

#ifndef INXLIB_DATA_REDBLACK_TREE_HPP
#define INXLIB_DATA_REDBLACK_TREE_HPP

#include <inxlib/inx.hpp>

#include "binary_tree.hpp"

namespace inx::data {

template <typename Node, typename Tag = void>
class redblack_tree;
class redblack_tree_base;
struct redblack_tree_node;
template <typename Tag = void>
struct redblack_tree_tag;
template <typename Tree, typename Node>
class redblack_tree_iterator;

struct redblack_tree_node : binary_tree_node
{
	using self = redblack_tree_node;
	using super = binary_tree_node;
	using redblack_node_type = self;

	bool is_black() const noexcept { return !m_rbData.red; }
	bool is_red() const noexcept { return m_rbData.red; }

	bool child_is_black(size_t i) const noexcept
	{
		assert(i < 2);
		return this->m_nData.children[i] == nullptr ||
		       static_cast<self&>(*this->m_nData.children[i]).is_black();
	}

	struct RBData
	{
		bool red;
	} m_rbData;
};

template <typename Tag>
struct redblack_tree_tag : redblack_tree_node
{
	using self = redblack_tree_tag<Tag>;
	using super = redblack_tree_node;
	using tag_node = self;

	// mary_tree wrapper functions
	tag_node* connect_child(tag_node& child, size_t i) noexcept
	{
		return static_cast<tag_node*>(super::connect_child(child, i));
	}
	tag_node* connect_none(size_t i) noexcept
	{
		return static_cast<tag_node*>(super::connect_none(i));
	}
	tag_node* connect_child_auto(tag_node* child, size_t i) noexcept
	{
		return static_cast<tag_node*>(super::connect_child_auto(child, i));
	}
	tag_node* make_root() noexcept
	{
		return static_cast<tag_node*>(super::make_root());
	}

	tag_node* parent() noexcept
	{
		return static_cast<tag_node*>(super::parent());
	}
	const tag_node* parent() const noexcept
	{
		return static_cast<const tag_node*>(super::parent());
	}

	tag_node* child(size_t i) noexcept
	{
		return static_cast<tag_node*>(super::child(i));
	}
	const tag_node* child(size_t i) const noexcept
	{
		return static_cast<const tag_node*>(super::child(i));
	}

	// binary_tree wrapper functions
	tag_node* left() noexcept { return static_cast<tag_node*>(super::left()); }
	const tag_node* left() const noexcept
	{
		return static_cast<const tag_node*>(super::left());
	}
	tag_node* right() noexcept
	{
		return static_cast<tag_node*>(super::right());
	}
	const tag_node* right() const noexcept
	{
		return static_cast<const tag_node*>(super::right());
	}
	tag_node* first() noexcept
	{
		return static_cast<tag_node*>(super::first());
	}
	const tag_node* first() const noexcept
	{
		return static_cast<const tag_node*>(super::first());
	}
	tag_node* second() noexcept
	{
		return static_cast<tag_node*>(super::second());
	}
	const tag_node* second() const noexcept
	{
		return static_cast<const tag_node*>(super::second());
	}
	tag_node* single() noexcept
	{
		return static_cast<tag_node*>(super::single());
	}
	const tag_node* single() const noexcept
	{
		return static_cast<const tag_node*>(super::single());
	}

	tag_node* find_inorder_id(size_t id) noexcept
	{
		return static_cast<tag_node*>(super::find_inorder_id(id));
	}
	const tag_node* find_inorder_id(size_t id) const noexcept
	{
		return static_cast<const tag_node*>(super::find_inorder_id(id));
	}

	tag_node& trace_inorder_id(size_t id) noexcept
	{
		return static_cast<tag_node&>(super::trace_inorder_id(id));
	}
	const tag_node& trace_inorder_id(size_t id) const noexcept
	{
		return static_cast<const tag_node&>(super::trace_inorder_id(id));
	}
};

class redblack_tree_base : public binary_tree_base
{
public:
	using self = redblack_tree_base;
	using super = binary_tree_base;
	using value_type = redblack_tree_node;

	void insert_root(value_type& node) noexcept { insertRootNode(node); }

	void insert_leaf_node(value_type& leaf, size_t i, value_type& ins) noexcept
	{
		insertUnderLeafNode(leaf, i, ins);
	}

	void insert_before(value_type& before, value_type& ins)
	{
		if (before.child(0) == nullptr) {
			insertUnderLeafNode(before, 0, ins);
		} else {
			insertUnderLeafNode(
			  static_cast<value_type&>(before.trace_inorder_id(1)), 1, ins);
		}
	}

	void insert_after(value_type& after, value_type& ins)
	{
		if (after.child(1) == nullptr) {
			insertUnderLeafNode(after, 1, ins);
		} else {
			insertUnderLeafNode(
			  static_cast<value_type&>(after.trace_inorder_id(0)), 0, ins);
		}
	}

	void erase(value_type& node) noexcept { eraseNode(node); }

	void clear() noexcept
	{
		this->m_root = nullptr;
		this->m_size = 0;
	}

	static bool is_node_black(const value_type* node) noexcept
	{
		return node == nullptr ? true : node->is_black();
	}
	static bool is_node_red(const value_type* node) noexcept
	{
		return node == nullptr ? false : node->is_red();
	}

protected:
	void swap_node(value_type& node1, value_type& node2) noexcept
	{
		assert(&node1 != &node2);
		for (int i = 0; i < 2; i++) {
			auto tmp = node1.m_nData.children[i];
			node1.connect_child_auto(node2.m_nData.children[i], i);
			node2.connect_child_auto(tmp, i);
		}
		std::swap(node1.m_rbData.red, node2.m_rbData.red);
		auto p1 = static_cast<value_type*>(node1.m_nData.parent);
		auto p2 = static_cast<value_type*>(node2.m_nData.parent);
		node1.m_nData.parent = p2;
		node2.m_nData.parent = p1;
		// handle p2 parent
		if (p1 == nullptr) {
			this->m_root = &node2;
		} else {
			p1->replace_child(node1, &node2);
		}
		// handle p1 parent
		if (p2 == nullptr) {
			this->m_root = &node1;
		} else {
			p2->replace_child(node2, &node1);
		}
		assert(this->m_root != &node1 || node1.parent() == nullptr);
		assert(this->m_root != &node2 || node2.parent() == nullptr);
		assert(node1.parent() == nullptr
		         ? this->m_root == &node1
		         : node1.parent()->children_connected());
		assert(node2.parent() == nullptr
		         ? this->m_root == &node2
		         : node2.parent()->children_connected());
		assert(node1.children_connected());
		assert(node2.children_connected());
	}

	void insertRootNode(value_type& node) noexcept
	{
		assert(this->m_size == 0 && this->m_root == nullptr);
		this->m_root = &node;
		this->m_size += 1;
		node.m_nData.parent = node.m_nData.children[0] =
		  node.m_nData.children[1] = nullptr;
		node.m_rbData.red = false;
	}
	void insertUnderLeafNode(value_type& leaf,
	                         size_t i,
	                         value_type& node) noexcept
	{
		assert(i < 2);
		assert(leaf.m_nData.children[i] == nullptr);
		this->m_size += 1;
		leaf.connect_child(node, i);
		node.m_nData.children[0] = node.m_nData.children[1] = nullptr;
		node.m_rbData.red = true;
		insertNormalise(&node);
	}
	void insertNormalise(value_type* node) noexcept
	{
		while (true) {
			if (node->is_root()) {
				node->m_rbData.red = false;
				break;
			} else if (auto p = static_cast<value_type*>(node->parent());
			           p->is_black()) {
				break;
			} else {
				auto gp = static_cast<value_type*>(p->parent());
				assert(gp !=
				       nullptr); // only root parent can be nullptr, but root
				                 // must be black so parent cannot be root
				size_t pid = gp->get_child_id(
				  *p); // the parent children id of grand parent
				auto u = static_cast<value_type*>(
				  gp->child(pid ^ 1)); // the uncle node
				if (is_node_red(u)) {  // both parent and uncle are red
					p->m_rbData.red = u->m_rbData.red = false;
					gp->m_rbData.red = true;
					node = gp;
					// break; // the only case we do not break
				} else { // parent is red and uncle is black
					size_t nid = p->get_child_id(*node);
					if (nid != pid) { // if rotating grand parent will result
						              // in invalid tree, do inital rotate
						gp->connect_child(p->rotate_id(pid), pid);
						p = node;
					}
					this->rotate_id(*gp, pid ^ 1);
					p->m_rbData.red = false;
					gp->m_rbData.red = true;
					break;
				}
			}
		}
	}

	void eraseNode(value_type& node) noexcept
	{
		m_size -= 1;
		if (node.is_deg2()) { // ensure node has at most one child
			swap_node(node,
			          static_cast<value_type&>(node.trace_inorder_id(
			            1))); // swap with in-order pred, which must
			                  // have at most 1 child
		}
		assert(!node.is_deg2());
		auto c = static_cast<value_type*>(node.single());
		auto p = static_cast<value_type*>(node.parent());
		if (p == nullptr) { // node to remove is root
			assert(m_root == &node && node.is_black());
			assert(!node.is_leaf() || m_size == 0);
			assert(!node.is_deg1() ||
			       (m_size == 1 && c->is_leaf() && c->is_red()));
			// make child the new root
			m_root = c;
			if (c != nullptr) { // if no child, then root was only element in
				                // tree, otherwise ensure new root is black
				c->make_root();
				c->m_rbData.red = false;
			}
			assert(m_root == nullptr ? m_size == 0
			                         : m_size > 0); // ensure that removing root
			                                        // did not break anything
			return;                                 // special case, end here
		}
		size_t nid = p->get_child_id(node);
		if (c == nullptr) {           // node is leaf
			if (node.is_red()) {      // node is a red leaf, can just delete
				p->connect_none(nid); // remove node
				return;
			}
			// otherwise, more complicated delete required
		} else { // node has 1 child
			if (node.is_black() !=
			    c->is_black()) { // red-black or black-red, replace child with
				                 // parent, make it black
				p->connect_child(*c, nid);
				c->m_rbData.red = false;
				return;
			}
		}
		assert(node.is_leaf()); // double black of degree 1 can only occur with
		                        // a leaf
		// since node is leaf, just delete it for now, keep track of parent
		p->connect_none(nid);
		// now rebalance the tree
		eraseNodeNormalise(p, nid);
	}

	void eraseNodeNormalise(value_type* P, size_t side) noexcept
	{
		// P is inbalanced, where the number of blacks on side is 1 less than on
		// the other
		while (true) {
			// case 1 handled in case 3
			assert(P != nullptr);
			// if (P == nullptr) // N is root
			//	return; // done
			auto S = static_cast<value_type*>(P->child(side ^ 1));
			assert(S != nullptr); // S cannot be a leaf
			// case 2
			if (S->is_red()) {
				assert(P->is_black());
				rotate_id(*P, side);
				P->m_rbData.red = true;
				S->m_rbData.red = false;
				S = static_cast<value_type*>(
				  P->child(side ^ 1)); // update S to new sibling
			}
			// case 3&4
			assert(S->is_black());
			if (is_node_black(static_cast<value_type*>(S->left())) &&
			    is_node_black(static_cast<value_type*>(S->right()))) {
				// case 3
				if (P->is_black()) {
					S->m_rbData.red = true;
					if (auto gp = static_cast<value_type*>(P->parent());
					    gp == nullptr) {
						break; // gp is root, thus next iteration will also be
						       // root
					} else {
						side = gp->get_child_id(*P);
						P = gp;
						continue; // go back to case 2
					}
				} else { // case 4
					// P is red
					P->m_rbData.red = false;
					S->m_rbData.red = true;
					break;
				}
				assert(
				  S->is_red()); // S is red here, thus case 5&6 cannot occur
			} else {
				if (auto* Sl = static_cast<value_type*>(S->child(side));
				    is_node_red(Sl)) { // case 5
					rotate_id(*S, side ^ 1);
					assert(Sl->parent() == P);
					S->m_rbData.red = true;
					Sl->m_rbData.red = false;
					S = Sl;
					assert(is_node_red(
					  static_cast<value_type*>(S->child(side ^ 1))));
				}
				if (auto* Sr = static_cast<value_type*>(S->child(side ^ 1));
				    is_node_red(Sr)) { // case 6
					rotate_id(*P, side);
					assert(P->parent() == S);
					S->m_rbData.red = P->m_rbData.red;
					P->m_rbData.red = false;
					Sr->m_rbData.red = false;
					break; // done
				}
			}
		}
	}
};

template <typename Tree, typename Node>
class redblack_tree_iterator
{
public:
	using self = redblack_tree_iterator<Tree, Node>;
	using tree_type = Tree;
	static_assert(
	  std::is_same_v<typename Tree::value_type, std::remove_cv_t<Node>>,
	  "Node must match Tree::value_type");
	using value_type = Node;
	using tag = typename tree_type::tag;
	using node_tag = redblack_tree_tag<tag>;
	using difference_type = ptrdiff_t;
	using reference = value_type&;
	using const_reference = const value_type&;
	using pointer = value_type*;
	using const_pointer = const value_type*;
	using iterator_category = std::bidirectional_iterator_tag;

	redblack_tree_iterator() noexcept
	  : m_tree(nullptr)
	  , m_node(nullptr)
	{
	}
	redblack_tree_iterator(tree_type& tree, value_type& node) noexcept
	  : m_tree(&tree)
	  , m_node(&node)
	{
	}
	redblack_tree_iterator(tree_type& tree, value_type* node) noexcept
	  : m_tree(&tree)
	  , m_node(node)
	{
	}
	redblack_tree_iterator(const self&) noexcept = default;
	redblack_tree_iterator(self&&) noexcept = default;

	~redblack_tree_iterator() noexcept = default;

	self& operator=(const self&) noexcept = default;
	self& operator=(self&&) noexcept = default;

	bool operator==(const self& rhs) const noexcept
	{
		assert(m_tree != nullptr && m_tree == rhs.m_tree);
		return m_node == rhs.m_node;
	}
	bool operator!=(const self& rhs) const noexcept
	{
		assert(m_tree != nullptr && m_tree == rhs.m_tree);
		return m_node != rhs.m_node;
	}
	bool operator==(std::nullptr_t) const noexcept { return m_node == nullptr; }
	bool operator!=(std::nullptr_t) const noexcept { return m_node != nullptr; }

	reference operator*() noexcept { return *m_node; }
	const_reference operator*() const noexcept { return *m_node; }
	pointer operator->() noexcept { return m_node; }
	const_pointer operator->() const noexcept { return m_node; }

	self& operator++() noexcept
	{
		assert(m_node != nullptr);
		m_node = static_cast<value_type*>(m_node->node_tag::find_inorder_id(0));
		return *this;
	}
	self operator++(int) noexcept
	{
		auto cp = *this;
		++*this;
		return cp;
	}
	self& operator--() noexcept
	{
		if (m_node == nullptr) {
			assert(m_tree != nullptr);
			m_node = &m_tree->back();
		} else {
			m_node =
			  static_cast<value_type*>(m_node->node_tag::find_inorder_id(1));
		}
		return *this;
	}
	self operator--(int) noexcept
	{
		auto cp = *this;
		--*this;
		return cp;
	}

	tree_type* tree() noexcept { return m_tree; }
	const tree_type* tree() const noexcept { return m_tree; }
	pointer node() noexcept { return m_node; }
	const_pointer node() const noexcept { return m_node; }

private:
	tree_type* m_tree;
	value_type* m_node;
};

template <typename Node, typename Tag>
class redblack_tree : public redblack_tree_base
{
public:
	using self = redblack_tree<Node, Tag>;
	using super = redblack_tree_base;
	using value_type = Node;
	using tag = Tag;
	using node_tag = redblack_tree_tag<tag>;
	using iterator = redblack_tree_iterator<self, value_type>;
	using const_iterator = redblack_tree_iterator<std::add_const_t<self>,
	                                              std::add_const_t<value_type>>;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	// mary_tree interface
	value_type& root() noexcept
	{
		return static_cast<value_type&>(static_cast<node_tag&>(super::root()));
	}
	const value_type& root() const noexcept
	{
		return static_cast<const value_type&>(
		  static_cast<const node_tag&>(super::root()));
	}

	// binary_tree interface
	value_type& front() noexcept
	{
		return static_cast<value_type&>(static_cast<node_tag&>(super::front()));
	}
	const value_type& front() const noexcept
	{
		return static_cast<const value_type&>(
		  static_cast<const node_tag&>(super::front()));
	}
	value_type& back() noexcept
	{
		return static_cast<value_type&>(static_cast<node_tag&>(super::back()));
	}
	const value_type& back() const noexcept
	{
		return static_cast<const value_type&>(
		  static_cast<const node_tag&>(super::back()));
	}

	// redblack_tree interface and iterator features
	template <typename Key, typename LessThan = std::less<void>>
	iterator lower_bound(Key key, LessThan&& lt = LessThan())
	{
		value_type* node =
		  static_cast<value_type*>(static_cast<node_tag*>(this->m_root));
		value_type* lb = nullptr;
		while (node != nullptr) {
			if (lt(*node, key)) {
				node = static_cast<value_type*>(node->node_tag::right());
			} else {
				lb = node;
				node = static_cast<value_type*>(node->node_tag::left());
			}
		}
		return iterator(*this, lb);
	}
	template <typename Key, typename LessThan = std::less<void>>
	const_iterator lower_bound(Key key, LessThan&& lt = LessThan()) const
	{
		const value_type* node =
		  static_cast<value_type*>(static_cast<node_tag*>(this->m_root));
		const value_type* lb = nullptr;
		while (node != nullptr) {
			if (lt(*node, key)) {
				node = static_cast<const value_type*>(node->node_tag::right());
			} else {
				lb = node;
				node = static_cast<const value_type*>(node->node_tag::left());
			}
		}
		return const_iterator(*this, lb);
	}

	// redblack_tree interface and iterator features
	template <typename Key, typename LessThan = std::less<void>>
	iterator upper_bound(Key key, LessThan&& lt = LessThan())
	{
		value_type* node =
		  static_cast<value_type*>(static_cast<node_tag*>(this->m_root));
		value_type* lb = nullptr;
		while (node != nullptr) {
			if (!lt(key, *node)) {
				node = static_cast<value_type*>(node->node_tag::right());
			} else {
				lb = node;
				node = static_cast<value_type*>(node->node_tag::left());
			}
		}
		return iterator(*this, lb);
	}
	template <typename Key, typename LessThan = std::less<void>>
	const_iterator upper_bound(Key key, LessThan&& lt = LessThan()) const
	{
		const value_type* node =
		  static_cast<value_type*>(static_cast<node_tag*>(this->m_root));
		const value_type* lb = nullptr;
		while (node != nullptr) {
			if (!lt(key, *node)) {
				node = static_cast<const value_type*>(node->node_tag::right());
			} else {
				lb = node;
				node = static_cast<const value_type*>(node->node_tag::left());
			}
		}
		return const_iterator(*this, lb);
	}

	template <typename Pred>
	iterator partition_point(Pred&& pred)
	{
		value_type* node =
		  static_cast<value_type*>(static_cast<node_tag*>(this->m_root));
		value_type* pt = nullptr;
		while (node != nullptr) {
			if (pred(*node)) {
				node = static_cast<value_type*>(node->node_tag::right());
			} else {
				pt = node;
				node = static_cast<value_type*>(node->node_tag::left());
			}
		}
		return iterator(*this, pt);
	}
	template <typename Pred>
	const_iterator partition_point(Pred&& pred) const
	{
		const value_type* node = static_cast<const value_type*>(
		  static_cast<const node_tag*>(this->m_root));
		const value_type* pt = nullptr;
		while (node != nullptr) {
			if (pred(*node)) {
				node = static_cast<const value_type*>(node->node_tag::right());
			} else {
				pt = node;
				node = static_cast<const value_type*>(node->node_tag::left());
			}
		}
		return const_iterator(*this, pt);
	}

	iterator insert_before(Node& before, Node& ins) noexcept
	{
		super::insert_before(static_cast<node_tag&>(before),
		                     static_cast<node_tag&>(ins));
		return iterator(*this, ins);
	}
	iterator insert_before(iterator before, Node& ins) noexcept
	{
		assert(before.tree() == this);
		if (before == nullptr) {
			if (this->m_root == nullptr) {
				insertRootNode(static_cast<node_tag&>(ins));
			} else {
				insertUnderLeafNode(static_cast<node_tag&>(this->back()),
				                    1,
				                    static_cast<node_tag&>(ins));
			}
		} else {
			insert_before(*before, ins);
		}
		return iterator(*this, ins);
	}

	iterator insert_after(Node& after, Node& ins) noexcept
	{
		super::insert_after(static_cast<node_tag&>(after),
		                    static_cast<node_tag&>(ins));
		return iterator(*this, ins);
	}
	iterator insert_after(iterator after, Node& ins) noexcept
	{
		assert(after.tree() == this);
		if (after == nullptr) {
			if (this->m_root == nullptr) {
				insertRootNode(static_cast<node_tag&>(ins));
			} else {
				insertUnderLeafNode(static_cast<node_tag&>(this->front()),
				                    0,
				                    static_cast<node_tag&>(ins));
			}
		} else {
			insert_after(*after, ins);
		}
		return iterator(*this, ins);
	}

	iterator insert(iterator it, Node& ins) { return insert_before(it, ins); }

	Node& erase(Node& node) noexcept
	{
		super::eraseNode(static_cast<node_tag&>(node));
		return node;
	}
	Node& erase(iterator it) noexcept
	{
		assert(it.tree() == this && it != nullptr);
		auto& node = *it;
		erase(*it);
		return node;
	}

	iterator begin() noexcept
	{
		return iterator(*this,
		                this->m_root != nullptr ? &this->front() : nullptr);
	}
	const_iterator begin() const noexcept
	{
		return const_iterator(
		  *this, this->m_root != nullptr ? &this->front() : nullptr);
	}
	iterator end() noexcept { return iterator(*this, nullptr); }
	const_iterator end() const noexcept
	{
		return const_iterator(*this, nullptr);
	}
	const_iterator cbegin() const noexcept { return begin(); }
	const_iterator cend() const noexcept { return end(); }
	reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
	const_reverse_iterator rbegin() const noexcept
	{
		return const_reverse_iterator(end());
	}
	const_reverse_iterator crbegin() const noexcept { return rbegin(); }
	reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
	const_reverse_iterator rend() const noexcept
	{
		return const_reverse_iterator(begin());
	}
	const_reverse_iterator crend() const noexcept { return rend(); }
};

} // namespace inx::data

#endif // INXLIB_DATA_REDBLACK_TREE_HPP
