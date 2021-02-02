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

		uint16_t framesInFlight;
		uint16_t currentFrame;

		virtual void ClearEntry(T*) = 0;

		Cache(uint16_t framesInFlight = 1)
		{
			this->framesInFlight = framesInFlight;
			this->currentFrame = 0;
		}

	public:
		virtual bool Add(const K& key) = 0;
		virtual T* Get(const K& key) = 0;
		T* operator[](K key) { return Get(key); }

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
		}

		virtual void Tick() { currentFrame = (currentFrame + 1) % framesInFlight; }
	};
}
