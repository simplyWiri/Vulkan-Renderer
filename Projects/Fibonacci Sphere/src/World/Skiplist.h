#pragma once
#include <cstdlib>
#include <iostream>
#include <vector>

template <class T>
class Skiplist
{
private:
	struct Node
	{
		Node(float k, T* v, int l) : key(k), level(l), value(v)
		{
			right = new Node*[l+1];
			left = new Node*[l+1];

			memset(right, 0, (l + 1) * sizeof(Node*));
			memset(left, 0, (l + 1) * sizeof(Node*));
		}
		~Node()
		{
			delete[] right;
			delete[] left;
			delete value;
		}

		float key;
		int level;
		
		T* value;

		Node** right;
		Node** left;
	};
	
public:
	Skiplist(int levels = 20)
		: maxLevel(levels)
	{
		head = new Node(std::numeric_limits<float>::min(), nullptr, levels);
	}
	
	~Skiplist()
	{
		delete head;
	}

	Node* Find(float key)
	{
		auto current = head;

		for(auto level = maxLevel; level >= 0; --level)
		{
			while(current->right[level] != NULL && current->right[level]->key <= key)
				current = current->right[level];
		}
		
		return current;
	}
	
	void Insert(float key, T* value)
	{
		auto level = RandomLevel();
		auto node = new Node(key, value, level);

		std::vector<Node*> nodesToUpdate(level + 1);

		auto current = head;

		for(int i = level; i >= 0; i--)
		{			
			while(current->right[i] != NULL && current->right[i]->key < key)
				current = current->right[i];

			nodesToUpdate[i] = current;
		}

		current = current->right[0];

		for (int i = 0; i <= level; i++)
		{
			auto l = nodesToUpdate[i];
			auto r = nodesToUpdate[i]->right[i];

			node->left[i] = l;
			node->right[i] = r;

			if(r != 0) r->left[i] = node;

			nodesToUpdate[i]->right[i] = node;
		}

		++entryCount;
	}

	void Print()
	{
		auto x = head->right[0];
		while (x != NULL) 
		{
			std::cout << x->key;
			x = x->right[0];
			if (x != NULL)
			    std::cout << " - ";
		}
		std:: cout << std::endl;
	}

	
	void Erase(float key)
	{
		std::vector<Node*> nodesToUpdate(maxLevel);

		auto current = head;

		for(int i = maxLevel - 1; i >= 0; i--)
		{			
			while(current->right[i] != NULL && current->right[i]->key < key)
				current = current->right[i];

			nodesToUpdate[i] = current;
		}

		current = current->right[0];

		if(current->key == key)
		{
			for(int i = 0; i <= current->level; i++)
			{
				if(nodesToUpdate[i]->right[i]->key != current->key) break; // too high

				auto r = current->right[i];
				nodesToUpdate[i]->right[i] = r;

				if(r != NULL)
				{
					r->left[i] = nodesToUpdate[i];
				}
			}

			delete current;

			--entryCount;
		}
	}

	bool Empty() const { return entryCount == 0; }
	int Size() const { return entryCount; }


	Node* GetHead() const { return head->right[0]; }
	
private:

	int RandomLevel()
	{
		int l = 0;
		while ((float)rand() / RAND_MAX < .5 && l < maxLevel) 
			l++;

		return l;
	}

	int maxLevel;
	int entryCount = 0;
	Node* head;
};