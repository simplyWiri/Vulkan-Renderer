#pragma once
#include <functional>
#include <iostream>
#include <tuple>

// A c++ equivalent to https://github.com/kelvin13/voronoi/blob/master/sources/game/lib/rbtree.swift

namespace Common
{
	template <class T>
	class LooseOrderedRbTree
	{
	public:
		struct Node
		{
			enum class Color { Red, Black };

			explicit Node(T value, Color color = Color::Red, Node* parent = nullptr, Node* left = nullptr, Node* right = nullptr) : parent(parent), element(value), color(color), left(left), right(right) { }

			Node* parent;
			Node* left;
			Node* right;

			T element;
			Color color;

			bool operator==(Node* other) { return *this == *other; }

			bool operator==(Node& other) { return std::tie(element, color, parent, left, right) == std::tie(other.element, other.color, other.parent, other.left, other.right); }

			Node* Leftmost()
			{
				auto* leftMost = this;
				while (leftMost != nullptr && leftMost->left != nullptr) leftMost = leftMost->left;

				return leftMost;
			}

			Node* Rightmost()
			{
				auto* rightMost = this;
				while (rightMost != nullptr && rightMost->right != nullptr) rightMost = rightMost->right;

				return rightMost;
			}

			Node* Successor()
			{
				if (right != nullptr) return right->Leftmost();

				auto* current = this;
				if (current == nullptr) return nullptr;

				auto* parent = current->parent;
				while (parent != nullptr && current == parent->right)
				{
					current = parent;
					parent = current->parent;
				}

				return current->parent;
			}

			Node* Predecessor()
			{
				if (left != nullptr) return left->Rightmost();

				auto* current = this;
				if (current == nullptr) return nullptr;

				auto* parent = current->parent;
				while (parent != nullptr && current == parent->left)
				{
					current = parent;
					parent = current->parent;
				}

				return current->parent;
			}
		};

		Node* root = nullptr;
		int count = 0;

		~LooseOrderedRbTree() { Cleanup(root); }

		// verifies that all paths in the red-black tree have the same black height,
		// that all nodes satisfy the red property, and that the root is black
		bool Verify()
		{
			if (root == nullptr) return true;

			return root->color == Node::Color::Black && Verify(root) >= 0;
		}

		Node* Append(T element)
		{
			if (Last() != nullptr) { return Insert(element, Last()); }
			else
			{
				auto newRoot = new Node(element);
				newRoot->color = Node::Color::Black;
				this->root = newRoot;

				count = 1;
				
				return root;
			}
		}

		Node* Insert(T element, Node* after)
		{
			auto node = new Node(element);
			Insert(node, after);

			count++;
			
			return node;
		}

		Node* First() const { return root->Leftmost(); }
		Node* Last() const { return root->Rightmost(); }

		static void Rotate(Node* pivot, Node*& root, std::function<Node*(Node*)> rotation)
		{
			auto parent = pivot->parent;

			if (parent == nullptr)
			{
				root = rotation(pivot);
				return;
			}
			if (pivot == parent->left) parent->left = rotation(pivot);
			else parent->right = rotation(pivot);
		}

		static void RotateLeft(Node* pivot, Node*& root) { Rotate(pivot, root, &LeftRotation); }
		static void RotateRight(Node* pivot, Node*& root) { Rotate(pivot, root, &RightRotation); }

		static Node* LeftRotation(Node* pivot)
		{
			auto newVertex = pivot->right;

			if (newVertex->left != nullptr) newVertex->left->parent = pivot;
			newVertex->parent = pivot->parent;
			pivot->parent = newVertex;

			pivot->right = newVertex->left;
			newVertex->left = pivot;

			return newVertex;
		}

		static Node* RightRotation(Node* pivot)
		{
			auto newVertex = pivot->left;

			if (newVertex->right != nullptr) newVertex->right->parent = pivot;
			newVertex->parent = pivot->parent;
			pivot->parent = newVertex;

			pivot->left = newVertex->right;
			newVertex->right = pivot;

			return newVertex;
		}

		void Insert(Node* node, Node* predecessor)
		{
			auto right = predecessor->right;
			if (right == nullptr)
			{
				predecessor->right = node;
				node->parent = predecessor;
				BalanceInsertion(node);
				return;
			}

			auto parent = right->Leftmost();
			parent->left = node;
			node->parent = parent;
			BalanceInsertion(node);
		}

		void BalanceInsertion(Node* node)
		{
			auto parent = node->parent;
			if (parent == nullptr) // We are the root
			{
				node->color = Node::Color::Black;
				return;
			}

			// Tree is already valid
			if (parent->color == Node::Color::Black) return;

			// Our parent is red thus it must have a parent.
			auto grandparent = parent->parent;

			auto uncle = parent == grandparent->left ? grandparent->right : grandparent->left;

			// Repaint the uncle & parent as black, and fix the grandparent.
			if (uncle && uncle->color == Node::Color::Red)
			{
				parent->color = Node::Color::Black;
				uncle->color = Node::Color::Black;

				grandparent->color = Node::Color::Red;
				BalanceInsertion(grandparent);
				return;
			}

			Node* n = nullptr;
			if (node == parent->right && parent == grandparent->left)
			{
				n = parent;
				grandparent->left = LeftRotation(parent);
			}
			else if (node == parent->left && parent == grandparent->right)
			{
				n = parent;
				grandparent->right = RightRotation(parent);
			}
			else { n = node; }

			n->parent->color = Node::Color::Black;
			grandparent->color = Node::Color::Red;

			if (n == n->parent->left) RotateRight(grandparent, root);
			else RotateLeft(grandparent, root);
		}

		void Remove(Node* node)
		{
			auto replaceLink = [](Node* node, Node* other, Node* parent)
			{
				if (node == parent->left) parent->left = other;
				else parent->right = other;
			};

			if (node->left && node->right)
			{
				auto right = node->right;
				auto replacement = right->Leftmost();

				auto parent = node->parent;
				if (parent) { replaceLink(node, replacement, parent); }
				else { root = replacement; }

				if (node == replacement->parent)
				{
					replacement->parent = replacement;
					if (replacement == node->left) node->left = node;
					else node->right = node;
				}
				else { replaceLink(replacement, node, replacement->parent); }

				std::swap(replacement->parent, node->parent);
				std::swap(replacement->left, node->left);
				std::swap(replacement->right, node->right);
				std::swap(replacement->color, node->color);

				if (node->left) node->left->parent = node;
				if (node->right) node->right->parent = node;
				if (replacement->left) replacement->left->parent = replacement;
				if (replacement->right) replacement->right->parent = replacement;
			}

			auto child = node->left ? node->left : node->right;

			if (node->color == Node::Color::Red) { replaceLink(node, nullptr, node->parent); }
			else if (child && child->color == Node::Color::Red)
			{
				auto parent = node->parent;
				if (parent != nullptr) { replaceLink(node, child, parent); }
				else { root = child; }

				child->parent = node->parent;
				child->color = Node::Color::Black;
			}
			else
			{
				BalanceDeletion(node);
				auto parent = node->parent;

				if (parent != nullptr) { replaceLink(node, nullptr, parent); }
				else { root = nullptr; }
			}

			count--;
			delete node;
		}

		void BalanceDeletion(Node* node)
		{
			auto parent = node->parent;
			if (parent == nullptr) return;

			// We must have a sibling, if not, its subtree would be unbalanced.
			auto sibling = node == parent->left ? parent->right : parent->left;

			if (sibling->color == Node::Color::Red)
			{
				parent->color = Node::Color::Red;
				sibling->color = Node::Color::Black;

				if (node == parent->left) RotateLeft(parent, root);
				else RotateRight(parent, root);

				sibling = node == parent->left ? parent->right : parent->left;
			}
			else if (parent->color == Node::Color::Black && (sibling->left == nullptr || sibling->left->color == Node::Color::Black) && (sibling->right == nullptr || sibling->right->color == Node::Color::Black))
			{
				sibling->color = Node::Color::Red;

				BalanceDeletion(parent);
				return;
			}

			if (parent->color == Node::Color::Red && (sibling->left == nullptr || sibling->left->color == Node::Color::Black) && (sibling->right == nullptr || sibling->right->color == Node::Color::Black))
			{
				sibling->color = Node::Color::Red;
				parent->color = Node::Color::Black;
				return;
			}
			else if (node == parent->left && (sibling->right == nullptr || sibling->right->color == Node::Color::Black))
			{
				sibling->color = Node::Color::Red;
				if (sibling->left) sibling->left->color = Node::Color::Black;

				sibling = RightRotation(sibling);
				parent->right = sibling;
			}
			else if (node == parent->right && (sibling->left == nullptr || sibling->left->color == Node::Color::Black))
			{
				sibling->color = Node::Color::Red;
				if (sibling->right) sibling->right->color = Node::Color::Black;

				sibling = LeftRotation(sibling);
				parent->left = sibling;
			}

			sibling->color = parent->color;
			parent->color = Node::Color::Black;

			if (node == parent->left)
			{
				if (sibling->right) sibling->right->color = Node::Color::Black;
				RotateLeft(parent, root);
			}
			else
			{
				if (sibling->left) sibling->left->color = Node::Color::Black;
				RotateRight(parent, root);
			}
		}

		void Cleanup(Node* root)
		{
			if (root == nullptr) return;

			Cleanup(root->left);
			Cleanup(root->right);

			delete root;
		}

		int Verify(Node* node)
		{
			if (node == nullptr) return 1;

			if (node->color == Node::Color::Red)
			{
				if (node->left && node->left->color == Node::Color::Red) return -1;
				if (node->right && node->right->color == Node::Color::Red) return -1;
			}

			auto leftHeight = Verify(node->left);
			auto rightHeight = Verify(node->right);

			if (leftHeight == -1 || rightHeight == -1) return -1;
			if (leftHeight != rightHeight) return -1;

			return leftHeight + static_cast<int>(node->color == Node::Color::Black);
		}

		void PrintHelper(Node* root, std::string indent, bool last)
		{
			// print the tree structure on the screen
			if (root != nullptr)
			{
				std::cout << indent;
				if (last)
				{
					std::cout << "R----";
					indent += "     ";
				}
				else
				{
					std::cout << "L----";
					indent += "|    ";
				}

				std::string sColor = root->color == Node::Color::Red ? "RED" : "BLACK";
				std::cout << "[" << *root->element << "]" << "(" << sColor << ")" << std::endl;
				PrintHelper(root->left, indent, false);
				PrintHelper(root->right, indent, true);
			}
		}

		Node* GetRoot() const { return root; }
		int Count() const { return count; }
	};
}
