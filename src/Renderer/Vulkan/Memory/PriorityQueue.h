#pragma once
#include <vector>
#include <intrin.h>

namespace Renderer::Memory
{
	template<class T>
	class PriorityQueue
	{
	private:
		size_t size = 0;
		std::vector<T> data;

	private:
		void Insert(T entry)
		{
			int position = BinarySearch(entry);
			data.insert(data.begin() + position, entry);
			size++;
		}
		void Remove(T entry)
		{
			int position = BinarySearch(entry);
			data.erase(data.begin() + position);
			size--;
		}
	public:

		auto begin() { return data.begin(); }
		auto begin() const { return data.begin(); }
		auto end() { return data.end(); }
		auto end() const { return data.end(); }

		T Pop() const
		{
			T val = data[0];
			Remove(val);
			return val;
		}
		size_t Size() const { return size; }
		T& Peak() const { return data[0]; }
		void Push(T entry) { Insert(entry); }

		int BinarySearch(T value)
		{
			const unsigned long size = static_cast<unsigned long>(data.size());
			if (size < 2) return 0;
			if (data[size - 1] < value) return static_cast<int>(size);

			unsigned long stepVal;
			_BitScanReverse(&stepVal, size - 1);
			uint32_t step = 1 << stepVal;

			int pos = data[step - 1] < value ? size - step - 1 : -1;

			while ((step >>= 1) > 0) {
				pos = (data[pos + step] < value ? pos + step : pos);
			}
			return pos + 1;
		}
	};
}