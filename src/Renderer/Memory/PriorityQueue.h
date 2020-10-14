#pragma once
#include <vector>
#include <intrin.h>
#include <functional>

namespace Renderer::Memory
{
	template <class T>
	class PriorityQueue
	{
	protected:
		std::vector<T> data;

	protected:
		void virtual Add(T entry)
		{
			int position = BinarySearchClosestIndex(entry);
			data.insert(data.begin() + position, entry);
		}

		void virtual Remove(T entry)
		{
			int position = BinarySearchClosestIndex(entry);
			data.erase(data.begin() + position);
		}

		void virtual Insert(int pos, T entry) { data.insert(data.begin() + pos, entry); }

		void virtual Erase(int pos) { data.erase(data.begin() + pos); }
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

		size_t Size() const { return data.size(); }
		T& Peak() const { return data[0]; }
		T& View(int idx) const { return &data[idx]; }

		T Take(int idx)
		{
			auto val = data[idx];
			Erase(idx);
			return val;
		}

		void Push(T entry) { Add(entry); }


		int BinarySearchClosestIndex(T value, std::function<bool(T&, T&)> comparer = nullptr)
		{
			if (comparer == nullptr) comparer = std::less<T>();

			const unsigned long size = static_cast<unsigned long>(data.size());
			if (size < 2) return 0;
			if (comparer(data[size - 1], value)) return static_cast<int>(size);

			unsigned long stepVal;
			_BitScanReverse(&stepVal, size - 1);
			uint32_t step = 1 << stepVal;

			int pos = comparer(data[step - 1], value) ? size - step - 1 : -1;

			while ((step >>= 1) > 0) { pos = (comparer(data[pos + step], value) ? pos + step : pos); }
			return pos + 1;
		}
	};
}
