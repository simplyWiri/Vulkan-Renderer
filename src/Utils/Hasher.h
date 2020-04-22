#include <string>
#include <vector>
#include <array>

/*
	This is my implementation of a MurmurHash
	Constants ~
	C1 - 0xcc9e2d51
	C2 - 0x1b873593
	R1 - 15
	R2 - 13
	M - 5
	N - 0xe6546b64
*/

class Hasher {
public:

	Hasher(uint32_t seed)
		: hash(seed), seed(seed) { }

	void reseed(uint32_t seed)
	{
		hash = seed;
	}

	uint32_t gethash() { return hash; }

	void HashPtr(const void* key)
	{
		// get ptr -> convert to a uint64, split in two halves, process two halves together.
		auto u64 = reinterpret_cast<uintptr_t>(key);
		std::array<uint32_t, 2> u32{ {u64 & 0xffffffffu, u64 >> 32} };

		Hash(u32.data(), 2);
	}

	void Hash(const void* key, int len)
	{
		hash = seed;
		// Get a slice of data from the overall key
		const uint8_t* data = (const uint8_t*)key;
		// We need to iterate over every 4 bytes
		const int blockCount = len / 4;

		// deal with all of our 4 byte blocks, i.e. the 'body'
		const uint32_t* body = (const uint32_t*)(data + blockCount * 4);
		for (int i = -blockCount; i; i++) 
		{
			uint32_t block = body[i];

			block *= c1; // first const
			block = ((block << r1) | (block >> (32 - r1))); // rotate by our first rot const
			block *= c2; // second const

			hash ^= block; 
			hash = ((hash << r2) | (hash >> (32 - r2))); // rotate by our second rot const
			hash = hash * 5 + n;
		}

		uint32_t block = 0;
		// now we deal with any remainders, i.e the 'tail'
		const uint8_t* tail = (const uint8_t*)(data + blockCount * 4); 

		switch (len & 3) {
		case 3: block ^= tail[2] << 16;
		case 2: block ^= tail[1] << 8;
		case 1: block ^= tail[0];

			block *= c1;
			block = ((block << r1) | (block >> (32 - r1)));
			block *= c2;
			hash ^= block;
		}

		// finalise the hash
		hash ^= len;

		// avalanche
		hash ^= hash >> 16;
		hash *= 0x85ebca6b;
		hash ^= hash >> 13;
		hash *= 0xc2b2ae35;
		hash ^= hash >> 16;
	}

private:
	uint32_t hash = 0;
	uint32_t seed;

	const uint32_t c1 = 0xcc9e2d51;
	const uint32_t c2 = 0x1b873593;
	const uint8_t r1 = 15;
	const uint8_t r2 = 13;
	const uint32_t m = 5;
	const uint32_t n = 0xe6546b64;
};