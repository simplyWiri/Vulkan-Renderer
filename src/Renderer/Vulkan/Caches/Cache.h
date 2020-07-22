#pragma once
#include <map>
#include <unordered_map>

namespace Renderer
{
	// T is our type, for ex. Renderpass
	// K is our key, for ex. RenderpassKey
	template <class T, class K>
	class Cache
	{
	protected:
		std::map<K, T*> cache;
		std::map<uint16_t, K> localToKey;
		uint16_t count = 0;
		virtual void clearEntry(T*) = 0;

	public:
		virtual bool add(const K& key) = 0;
		virtual bool add(const K& key, uint16_t& local) = 0;
		virtual T* get(const K& key) = 0;

		void clearCache()
		{
			auto iter = cache.begin();

			while (iter != cache.end())
			{
				clearEntry(iter->second);
				delete iter->second;
				++iter;
			}

			cache.clear();
			localToKey.clear();
		}

		T* operator[](K key) { return get(key); }
		T* operator[](const uint16_t& index) { return get(localToKey.at(index)); }
		T* indexToValue(const uint16_t& index) { return get(localToKey.at(index)); }

		K indexToKey(const uint16_t& index) { return localToKey.at(index); }

		uint16_t registerInput(K key)
		{
			localToKey.emplace(count, key);
			return count++;
		}
	};
}
