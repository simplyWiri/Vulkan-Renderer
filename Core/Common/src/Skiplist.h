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
				memset(next, 0, level * sizeof(Node*));
				memset(prev, 0, level * sizeof(Node*));
			}
			
			
			K key;
			V value;
			int actualLevel;
			Node* next[level];
			Node* prev[level];
		};

		class Iterator
		{
		private:
			Node* current;
		public:
			explicit Iterator(Node* ptr) : current(ptr) { }
			
			Iterator& operator++()
			{
				current = current->next[0];
				return *this;
			}
			Iterator& operator--() 
			{
				current = current->prev[0];
				return *this;
			}
			Node* operator*() const { return current; }
			bool operator==(const Iterator& other) const { return current == other.current; }
			bool operator!=(const Iterator& other) const { return !(*this == other); }
		};
	
		
	public:
		Skiplist(K min = std::numeric_limits<K>::min(), K max = std::numeric_limits<K>::max())
		{
			head = new Node(min, 0, level);
			tail = new Node(max, 0, level);

			for(auto i = 0; i < level; i++)
			{
				head->next[i] = tail;
				tail->prev[i] = head;
			}
		}

		~Skiplist()
		{
			 while (head)
	        {
	            Node* old = head;
	            head = head->next[0];
	            delete old;
	        }
		}

	private:
		
		static int RandomLevel()
		{			
			int randomNumber = 0;
			while (static_cast<float>(rand()) / RAND_MAX < .5 && randomNumber < level - 1) randomNumber++;

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

		void Clear()
		{
			auto* curNode = head->next[0];

			while (curNode)
	        {
	            Node* old = curNode;
	            curNode = curNode->next[0];
				if(curNode == nullptr) break;
	            delete old;
	        }

			currentMaxLevel = 0;
			entryCount = 0;

			for(auto i = 0; i < level; i++)
			{
				head->next[i] = tail;
				tail->prev[i] = head;
			}
		}

		void Add(K key, V value)
		{
			auto nodeLevel = RandomLevel();
			auto node = new Node(key, value, nodeLevel);

			Node* nodesToUpdate[level];
			
			if(nodeLevel > currentMaxLevel) currentMaxLevel = nodeLevel;
			
			auto currentNode = Find(key, nodesToUpdate);

			for(int i = 0; i <= nodeLevel; i++)
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

			++entryCount;
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

			--entryCount;
			delete node;
		}

		int Size() const { return entryCount; }
		bool Empty() const { return entryCount == 0; }
		
		Iterator begin() { return Iterator{head}; }
		Iterator end() { return Iterator{tail}; }
		

	private:		
		Node* head;
		Node* tail;
		int currentMaxLevel = 0;
		int entryCount = 0;
	};

	
}
