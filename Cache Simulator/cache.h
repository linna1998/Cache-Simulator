#ifndef CACHE_CACHE_H_
#define CACHE_CACHE_H_

#include <bitset>
#include <math.h>
#include <stdint.h>
#include "storage.h"
#include <vector>

using namespace std;
#define INF 0x3fffffff
typedef struct CacheConfig_
{
	// int size;
	 // int associativity;
	 // int set_num; // Number of cache sets
	int write_through; // 0|1 for back|through
	int write_allocate; // 0|1 for no-alc|alc

	int s;
	int S;  // S = 2^s sets
	int e;
	int E;  // E = 2^e lines per set
	int b;
	int B;  // B = 2b bytes per cache block (the data)
	int C;  // C = S x E x B data bytes
} CacheConfig;

typedef struct Block
{
	bool valid;
	int tag;
	vector<bool> dirty;  // dirty==true means ot has been written  
	vector<int> data_block;
	int IRR, Recency;  // 用于LIRS替换策略
};
typedef struct Set
{
	vector<Block> data_set;
	vector<int> value;  // 用于判断LRU的替换策略
};

class Cache : public Storage
{
public:
	Cache()
	{
		inCache.reset();
	}
	~Cache() {}
	int PFA;  // Prefetch Algorithm. 
	int RA;  // ReplaceAlgorithm
	bitset<(1 << 24)> inCache;  // 判断某个地址是否在cache中

	// Sets & Gets
	void SetConfig(CacheConfig cc);
	void GetConfig(CacheConfig cc);
	void BuildCache();
	void PrintCache();
	void PrintStatus();
	// Main access process
	void HandleRequest(uint64_t addr, int bytes, int read,
		char *content, int &hit, int &time);

private:

	// Bypassing
	int BypassDecision(uint64_t addr);
	void BypassAlgorithm(uint64_t addr, int &time);
	// Partitioning
	void PartitionAlgorithm(uint64_t addr, uint64_t& tag,
		uint64_t & set_index, uint64_t& block_offset);
	void MergeAlgorithm(uint64_t &addr, uint64_t tag,
		uint64_t  set_index, uint64_t block_offset);
	// Replacement
	int ReplaceDecision(uint64_t tag, uint64_t set_index);
	void ReplaceAlgorithmLRU(uint64_t tag, uint64_t set_index,
		StorageStats & stats_, int &time);
	void ReplaceAlgorithmLIRS(uint64_t tag, uint64_t set_index,
		StorageStats & stats_, int &time);
	// Prefetching
	int PrefetchDecision();
	void PrefetchAlgorithm(int addr, int &time);

	CacheConfig config_;
	vector<Set> data_cache;

	DISALLOW_COPY_AND_ASSIGN(Cache);
};

#endif //CACHE_CACHE_H_ 
