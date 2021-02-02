#pragma once
#include <cstdlib>

namespace Common
{
	template <class K, class V, int level = 20>
	class Skiplist 
	{
		class Node
		{
		public:
			Node(K key, V value, int level) : key(key), value(value), actualLevel(level)
			{
				for(int i = 0; i < actualLevel; i++)
					next[i] = prev[i] = nullptr;
			}
			
			K key;
			V value;
			int actualLevel;
			Node* next[level];
			Node* prev[level];
		};

		Skiplist(K min, K max)
		{
			head = new Node(min, nullptr, level);
			tail = new Node(max, nullptr, level);

			for(auto i = 0; i < level; i++)
			{
				head->next[i] = tail;
				tail->prev[i] = head;
			}
		}

		~Skiplist()
		{
			delete head;
			delete tail;
		}

	private:
		
		int RandomLevel()
		{
			int randomNumber = 0;
			while (static_cast<float>(rand()) / RAND_MAX < .5 && randomNumber < level) randomNumber++;

			return randomNumber;
		}

	public:
		
		Node* Find(const K& key, Node** previous = nullptr)
		{
			auto* node = head;

			for(int i = currentMaxLevel; i >= 0; --i)
			{
				while(node->next[i]->key < key)
					node = node->next[i];

				if(previous != nullptr)
					previous[i] = node;
			}

			node = node->next[0];
			
			return node;
		}

		void Add(K key, V value)
		{
			auto level = RandomLevel();
			auto node = new Node(key, value, level);

			Node** nodesToUpdate = nullptr;

			if(level > currentMaxLevel) currentMaxLevel = level;
			
			auto currentNode = Find(key, nodesToUpdate);

			for(int i = 0; i <= level; i++)
			{
				auto prev = nodesToUpdate[i];
				auto next = nodesToUpdate[i]->next[i];

				// Add pointers for our current node
				node->prev[i] = prev;
				node->next[i] = next;

				// Add pointers to our current node 
				next->prev[i] = node;
				prev->next[i] = node;
			}
		
		}
		void Remove(K key)
		{
			auto node = Find(key);

			if (node == nullptr || node->key != key) return;
			
			for(int i = 0; i <= node->actualLevel; i++)
			{
				auto prev = node->prev[i];
				auto next = node->next[i];

				prev->next[i] = next;
				next->prev[i] = prev;
			}

			if( node->actualLevel == currentMaxLevel )
			{
				while( currentMaxLevel > 1 && head->next[currentMaxLevel] != tail) 
					currentMaxLevel--;
			}
			
			delete node;
		}
		void Clear();





	private:		
		Node* head;
		Node* tail;
		int currentMaxLevel = 0;
		int entryCount = 0;

		
	};

	
}
