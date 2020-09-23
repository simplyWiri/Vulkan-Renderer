#pragma once
#include <unordered_map>

namespace Renderer
{
	// T is our type, for ex. Renderpass
	// K is our key, for ex. RenderpassKey
	template <class T, class K>
	class Cache
	{
		protected:
			std::unordered_map<K, T*> cache;
			std::unordered_map<uint16_t, K> localToKey;
			uint16_t count = 0;
			virtual void ClearEntry(T*) = 0;

		public:
			virtual bool Add(const K& key) = 0;
			virtual bool Add(const K& key, uint16_t& local) = 0;
			virtual T* Get(const K& key) = 0;

			void ClearCache()
			{
				auto iter = cache.begin();

				while (iter != cache.end())
				{
					ClearEntry(iter->second);
					delete iter->second;
					++iter;
				}

				cache.clear();
				localToKey.clear();
			}

			T* operator[](K key) { return Get(key); }
			T* operator[](const uint16_t& index) { return Get(localToKey.at(index)); }
			T* IndexToValue(const uint16_t& index) { return Get(localToKey.at(index)); }

			K IndexToKey(const uint16_t& index) { return localToKey.at(index); }

			uint16_t RegisterInput(K key)
			{
				localToKey.emplace(count, key);
				return count++;
			}
	};
}
