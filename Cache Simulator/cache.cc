﻿#include "cache.h"
#include "def.h"

void Cache::SetConfig(CacheConfig cc)
{
	// 类似的set所有设置	
	config_.write_through = cc.write_through;
	config_.write_allocate = cc.write_allocate;
	config_.b = cc.b;
	config_.e = cc.e;
	config_.s = cc.s;
	config_.B = pow(2, config_.b);
	config_.E = pow(2, config_.e);
	config_.S = pow(2, config_.s);
	config_.C = config_.B * config_.E * config_.S;
}

void Cache::GetConfig(CacheConfig cc)
{
	// 类似的get所有设置	
	cc.write_through = config_.write_through;
	cc.write_allocate = config_.write_allocate;
	cc.b = config_.b;
	cc.e = config_.e;
	cc.s = config_.s;
	cc.B = pow(2, cc.b);
	cc.E = pow(2, cc.e);
	cc.S = pow(2, cc.s);
	cc.C = cc.B * cc.E * cc.S;
}

void Cache::BuildCache()
{
	Block new_block;
	new_block.tag = 0;
	new_block.valid = false;
	new_block.IRR = INF;
	new_block.Recency = 0;
	for (int i = 0; i < config_.B; i++)
	{
		new_block.data_block.push_back(0);
		new_block.dirty.push_back(false);
	}
	Set new_set;
	for (int i = 0; i < config_.E; i++)
	{
		new_set.value.push_back(0);
		new_set.data_set.push_back(new_block);
	}
	for (int i = 0; i < config_.S; i++)
	{
		data_cache.push_back(new_set);
	}
}

void Cache::PrintCache()
{
	printf("S= %d, E= %d, B= %d\n", config_.S, config_.E, config_.B);
	printf("All the tag values.\n");
	for (int i = 0; i < config_.S; i++)
	{
		printf("[%3x]", i);
		for (int j = 0; j < config_.E; j++)
		{
			if (data_cache[i].data_set[j].valid == true)
			{
				//printf("cache[%d][%d]: tag = %x(%u) valid= %d\n", i, j,
				//	data_cache[i].data_set[j].tag, data_cache[i].data_set[j].tag, 
				//	data_cache[i].data_set[j].valid);		
				printf("%5x ", data_cache[i].data_set[j].tag);
				//printf("%5x ", data_cache[i].value[j]);
			}
			else
			{
				printf("%5d", -1);
				//printf("%5d ", -1);
			}
		}
		printf("\n");
	}
}

void Cache::PrintStatus()
{
	StorageStats ss = stats_;
	printf("L1 Cache: \n");
	printf("Hit = %d\n", ss.access_counter - ss.miss_num);
	printf("Num = %d\n", ss.access_counter);
	printf("Miss Rate= %f\n", (double)(ss.miss_num) / (double)(ss.access_counter));
	printf("Time= %d cycles\n", ss.access_time);
	printf("Replacement = %d\n", ss.replace_num);
	printf("Fetch num = %d\n", ss.fetch_num);
	printf("Prefetch num = %d\n", ss.prefetch_num);
	printf("Bypass num = %d\n", ss.bypass_num);
	printf("\n");

	Storage * ll = lower_;
	ll->GetStats(ss);
	printf("L2 Cache: \n");
	printf("Hit = %d\n", ss.access_counter - ss.miss_num);
	printf("Num = %d\n", ss.access_counter);
	printf("Miss Rate= %f\n", (double)(ss.miss_num) / (double)(ss.access_counter));
	printf("Time= %d cycles\n", ss.access_time);
	printf("Replacement = %d\n", ss.replace_num);
	printf("Fetch num = %d\n", ss.fetch_num);
	printf("Prefetch num = %d\n", ss.prefetch_num);
	printf("Bypass num = %d\n", ss.bypass_num);
	printf("\n");

	Storage* ml;
	ll->GetLower(ml);
	ml->GetStats(ss);
	printf("Memory:\n");
	printf("Hit = %d\n", ss.access_counter - ss.miss_num);
	printf("Num = %d\n", ss.access_counter);
	printf("Miss Rate= %f\n", (double)(ss.miss_num) / (double)(ss.access_counter));
	printf("Time= %d cycles\n", ss.access_time);
	//printf("Replacement = %d\n", ss.replace_num);
	//printf("Fetch num = %d\n", ss.fetch_num);
	//printf("Bypass num = %d\n", ss.bypass_num);
	printf("\n");
}

void Cache::HandleRequest(uint64_t addr, int bytes, int read,
	char *content, int &hit, int &time)
{
	hit = 0;
	time = 0;
	uint64_t tag;
	uint64_t set_index;
	uint64_t block_offset;

	stats_.access_counter++;

	// Read.
	if (read == 1)
	{
		// 除了bypass，无论如何都会有hit latency		
		// bypass 后面会减掉时间开销,access time 等等
		time += latency_.bus_latency + latency_.hit_latency;
		stats_.access_time += latency_.bus_latency + latency_.hit_latency;

		PartitionAlgorithm(addr, tag, set_index, block_offset);
		// Miss?
		// if (ReplaceDecision(tag, set_index))
		if (BypassDecision(addr))
		{
			// DEBUG
			// printf("BypassDecision(addr) = %d\n", BypassDecision(addr));
			// printf("MISS\n");

			stats_.access_counter--;
			stats_.bypass_num++;
			time -= (latency_.bus_latency + latency_.hit_latency);
			stats_.access_time -= (latency_.bus_latency + latency_.hit_latency);
			// Choose victim
			if (RA == 0)
			{
				ReplaceAlgorithmLRU(tag, set_index, stats_, time);
			}
			else
			{
				ReplaceAlgorithmLIRS(tag, set_index, stats_, time);
			}
			inCache[addr >> config_.b] = 1;
		}
		else
		{
			// return hit & time, Read hit cache
			//PrintCache();
			// printf("READ HIT!\n");
			uint64_t block_index = 0;
			for (int i = 0; i < config_.E; i++)
			{
				if (data_cache[set_index].data_set[i].tag == tag)
				{
					block_index = i;
					break;
				}
			}
			// Fresh LRU
			//printf("Read HIT! Fresh LRU\n");
			data_cache[set_index].value[block_index] = 0;
			for (int i = 0; i < config_.E; i++)
			{
				if (i != block_index && data_cache[set_index].data_set[i].valid == true)
				{
					//printf("[%d]: old value %d ", i, data_cache[set_index].value[i]);
					data_cache[set_index].value[i]++;
					//printf("new value %d \n", data_cache[set_index].value[i]);
				}
			}
			// Fresh LIRS
			data_cache[set_index].data_set[block_index].IRR
				= data_cache[set_index].data_set[block_index].Recency;
			data_cache[set_index].data_set[block_index].Recency = 0;
			for (int i = 0; i < config_.E; i++)
			{
				if (i != block_index && data_cache[set_index].data_set[i].valid == true)
				{
					data_cache[set_index].data_set[i].Recency++;
				}
			}
			hit = 1;
			return;
		}

		// DEBUG
		// printf("Before prefetch\n");
		// Prefetch?
		if (PrefetchDecision())
		{
			PrefetchAlgorithm(addr, time);
		}
		else
		{
			// Fetch from lower layer
			int lower_hit, lower_time;
			lower_->HandleRequest(addr, bytes, read, content,
				lower_hit, lower_time);

			// DEBUG
			// printf("addr>>config_.b = %d\n", addr >> config_.b);

			inCache[addr >> config_.b] = 1;
			time += lower_time;
			stats_.fetch_num++;
		}
	}
	// Write
	else if (read == 0)
	{
		// 只考虑write back && write allocate !!!
		// 除了bypass，无论如何都会有hit latency		
		time += latency_.bus_latency + latency_.hit_latency;
		stats_.access_time += latency_.bus_latency + latency_.hit_latency;

		PartitionAlgorithm(addr, tag, set_index, block_offset);
		// Miss?
		bool miss = ReplaceDecision(tag, set_index);
		if (miss)
		{
			stats_.miss_num++;
			// Choose victim
			if (RA == 0)
			{
				ReplaceAlgorithmLRU(tag, set_index, stats_, time);
			}
			else
			{
				ReplaceAlgorithmLIRS(tag, set_index, stats_, time);
			}
			// Fetch from lower layer
			int lower_hit, lower_time;
			lower_->HandleRequest(addr, bytes, read, content,
				lower_hit, lower_time);
			hit = 0;
			time += lower_time;
			stats_.fetch_num++;
			inCache[addr >> config_.b] = 1;
		}
		else
		{
			hit = 1;
		}

		// Write to cache, data_cache[set_index].data_set[block_index]
		// Find block index.
		uint64_t block_index = 0;
		for (int i = 0; i < config_.E; i++)
		{
			if (data_cache[set_index].data_set[i].tag == tag)
			{
				block_index = i;
				break;
			}
		}
		if (!miss)
		{
			// Fresh LRU
			//printf("Write HIT! Fresh LRU\n");
			data_cache[set_index].value[block_index] = 0;
			for (int i = 0; i < config_.E; i++)
			{
				if (i != block_index && data_cache[set_index].data_set[i].valid == true)
				{
					//printf("[%d]: old value %d ", i, data_cache[set_index].value[i]);
					data_cache[set_index].value[i]++;
					//printf("new value %d \n", data_cache[set_index].value[i]);
				}
			}

			// Fresh LIRS
			data_cache[set_index].data_set[block_index].IRR
				= data_cache[set_index].data_set[block_index].Recency;
			data_cache[set_index].data_set[block_index].Recency = 0;
			for (int i = 0; i < config_.E; i++)
			{
				if (i != block_index && data_cache[set_index].data_set[i].valid == true)
				{
					data_cache[set_index].data_set[i].Recency++;
				}
			}

			// ???
			// Write to data_cache[set_index].data_set[block_index].data_block[block_offset]

			// Write dirty bit;			
			data_cache[set_index].data_set[block_index].dirty[block_offset] = true;
		}
	}
}

int Cache::BypassDecision(uint64_t addr)
{
	/*
	uint64_t tag;
	uint64_t set_index;
	uint64_t block_offset;
	PartitionAlgorithm(addr, tag, set_index, block_offset);
	return ReplaceDecision(tag, set_index);
	*/
	return (!inCache[addr >> config_.b]);

	// return FALSE;
	////都cache bypass 
	//return TRUE;
}
void Cache::BypassAlgorithm(uint64_t addr, int& time)
{
	// Pass this layer, find in the lower layer
	int lower_hit, lower_time;
	char content[32];
	lower_->HandleRequest(addr, 4, 1, content,
		lower_hit, lower_time);
	time += lower_time;
	stats_.access_counter--;
	stats_.bypass_num++;
}
// Read the tag, set_index, block_offset number from the addr.
void Cache::PartitionAlgorithm(uint64_t addr, uint64_t& tag,
	uint64_t & set_index, uint64_t& block_offset)
{
	tag = (addr >> (config_.b + config_.s))&
		((1 << (32 - (config_.b + config_.s))) - 1);
	set_index = (addr >> config_.b)&((1 << (config_.s)) - 1);
	block_offset = addr&((1 << (config_.b)) - 1);

	/*
	printf("Partition Algorithm: tag : %lx(%lu)\n", tag, tag);
	printf("Partition Algorithm: set_index : %lx(%lu)\n", set_index, set_index);
	printf("Partition Algorithm: block_offset : %lx(%lu)\n",
	block_offset, block_offset);
	*/
}
// Merge the addr from the tag, set_index, block_offset.
void Cache::MergeAlgorithm(uint64_t& addr, uint64_t tag,
	uint64_t set_index, uint64_t block_offset)
{
	addr = 0;
	addr += block_offset;
	addr += (set_index << (config_.b));
	addr += (tag << (config_.b + config_.s));
	/*
	printf("Partition Algorithm: tag : %lx(%lu)\n", tag, tag);
	printf("Partition Algorithm: set_index : %lx(%lu)\n", set_index, set_index);
	printf("Partition Algorithm: block_offset : %lx(%lu)\n",
	block_offset, block_offset);
	printf("Partition Algorithm: addr : %lx(%lu)\n",
	addr, addr);
	*/
}

// return true means the cache miss.
int Cache::ReplaceDecision(uint64_t tag, uint64_t set_index)
{
	for (int i = 0; i < config_.E; i++)
	{
		if (data_cache[set_index].data_set[i].tag == tag
			&& data_cache[set_index].data_set[i].valid == true)
		{
			return FALSE;
		}
	}
	return TRUE;
	//return FALSE;
}

// LRU Algorithm
void Cache::ReplaceAlgorithmLRU(uint64_t tag, uint64_t set_index,
	StorageStats & stats_, int &time)
{
	int out_index = -1;  // replace data_cache[set_index].data_set[out_index]
	for (int i = 0; i < config_.E; i++)
	{
		if (data_cache[set_index].data_set[i].valid == false) out_index = i;
	}
	if (out_index == -1)
	{
		stats_.replace_num++;  // Need to replace some blocks.
		out_index = 0;  // If not, cause segmentation fault.

						// LRU Algorithm
		for (int i = 0; i < config_.E; i++)
		{
			//if (data_cache[set_index].data_set[i].valid >
			//	data_cache[set_index].data_set[out_index].valid)
			if (data_cache[set_index].value[i] >
				data_cache[set_index].value[out_index])
			{
				out_index = i;
			}
		}
	}
	uint64_t addr;
	MergeAlgorithm(addr, data_cache[set_index].data_set[out_index].tag,
		set_index, 0);
	inCache[addr >> config_.b] = 0;

	// Dirty && Write_back	
	// 假设一次能写回所有脏位
	for (int i = 0; i < config_.B; i++)
	{
		if (data_cache[set_index].data_set[out_index].dirty[i] == true
			&& config_.write_through == 0)
		{
			// ???
			// Write block's data[i] back into lower layer
			int lower_hit = 0, lower_time = 0;
			char content[32];
			MergeAlgorithm(addr, data_cache[set_index].data_set[out_index].tag,
				set_index, i);
			lower_->HandleRequest(addr, 4, 0, content,
				lower_hit, lower_time);
			time += lower_time;
			stats_.fetch_num++;
			break;
			// DEBUG
			// printf("write to lower layer in address %d\n", addr);
		}
	}

	// Write a new block into cache
	for (int i = 0; i < config_.B; i++)
	{
		data_cache[set_index].data_set[out_index].dirty[i] = false;
	}
	data_cache[set_index].data_set[out_index].tag = tag;
	data_cache[set_index].data_set[out_index].valid = true;
	// ???
	// Fresh data_cache[set_index].data_set[out_index].data_block;

	// fresh the LRU value
	//printf("Fresh LRU in ReplaceAlgorithm.\n");
	data_cache[set_index].value[out_index] = 0;
	for (int i = 0; i < config_.E; i++)
	{
		if (i != out_index && data_cache[set_index].data_set[i].valid == true)
			data_cache[set_index].value[i]++;
	}
}

//LIRS算法首先替换IRR最大的页面，其中infinite的值最大，
//当IRR相同时，替换Recency最大的页面。
//IRR在一定程度上反应了页面的访问频度，
//LIRS倾向于认为：一个页面的IRR越大，将来的IRR会变得更大。
//Recency参数相当于LRU，替换时IRR优先于Recency，这就降低了最后一次访问数据的优先级，
//因为有些数据虽然最近访问却不一定常用，可能访问过一次之后很久都不再使用，
//如果Recency优于IRR，这些只使用了一次数据有可能会停留相当长时间。
// LIRS Algorithm
void Cache::ReplaceAlgorithmLIRS(uint64_t tag, uint64_t set_index,
	StorageStats & stats_, int &time)
{
	int out_index = -1;  // replace data_cache[set_index].data_set[out_index]
						 // 扔出非法块
	for (int i = 0; i < config_.E; i++)
	{
		if (data_cache[set_index].data_set[i].valid == false) out_index = i;
	}
	//先比较INF，再比较Recent，扔块
	if (out_index == -1)
	{
		stats_.replace_num++;  // Need to replace some blocks.
		out_index = 0;  // If not, cause segmentation fault.

		for (int i = 0; i < config_.E; i++)
		{
			if (data_cache[set_index].data_set[i].IRR >
				data_cache[set_index].data_set[out_index].IRR)
			{
				out_index = i;
			}
			else if (data_cache[set_index].data_set[i].IRR ==
				data_cache[set_index].data_set[out_index].IRR)
			{
				if (data_cache[set_index].data_set[i].Recency >
					data_cache[set_index].data_set[out_index].Recency)
				{
					out_index = i;
				}
			}
		}
	}
	uint64_t addr;
	MergeAlgorithm(addr, data_cache[set_index].data_set[out_index].tag,
		set_index, 0);
	inCache[addr >> config_.b] = 0;

	// Dirty && Write_back	
	// 假设一次能写回所有脏位
	for (int i = 0; i < config_.B; i++)
	{
		if (data_cache[set_index].data_set[out_index].dirty[i] == true
			&& config_.write_through == 0)
		{
			// ???
			// Write block's data[i] back into lower layer
			int lower_hit = 0, lower_time = 0;
			char content[32];
			MergeAlgorithm(addr, data_cache[set_index].data_set[out_index].tag,
				set_index, i);
			lower_->HandleRequest(addr, 4, 0, content,
				lower_hit, lower_time);
			time += lower_time;
			stats_.fetch_num++;

			break;
			// DEBUG
			// printf("write to lower layer in address %d\n", addr);
		}
	}

	// Write a new block into cache
	for (int i = 0; i < config_.B; i++)
	{
		data_cache[set_index].data_set[out_index].dirty[i] = false;
	}
	data_cache[set_index].data_set[out_index].tag = tag;
	data_cache[set_index].data_set[out_index].valid = true;
	data_cache[set_index].data_set[out_index].IRR = INF;
	data_cache[set_index].data_set[out_index].Recency = 0;
	// ???
	// Fresh data_cache[set_index].data_set[out_index].data_block;

	// fresh the LIRS value	
	data_cache[set_index].data_set[out_index].IRR =
		data_cache[set_index].data_set[out_index].Recency;
	data_cache[set_index].data_set[out_index].Recency = 0;
	for (int i = 0; i < config_.E; i++)
	{
		if (i != out_index)
		{
			data_cache[set_index].data_set[out_index].Recency++;
		}
	}
}


// PFA=0 means no prefetch algorithm.
// PFA=1 means next-line prefetch algorithm.
// PFA=2 means prefetch 2 lines.
// PFA=3 means prefetch 4 lines.
int Cache::PrefetchDecision()
{
	if (PFA == 0) return FALSE;
	else return TRUE;
}

// 数据预取，时间只加一次
// PFA=0 means no prefetch algorithm.
// PFA=1 means next-line prefetch algorithm.
// PFA=2 means prefetch 2 lines.
// PFA=3 means prefetch 4 lines.
void Cache::PrefetchAlgorithm(int addr, int &time)
{
	if (PFA == 0) return;
	char content[32];
	// Fetch from lower layer
	int lower_hit, lower_time;
	int new_addr[4] = { 0 };  // 预取地址
	int fetch_lines = 0;  // 预取个数
	if (PFA == 1 || PFA == 2) fetch_lines = 2;
	else if (PFA == 3) fetch_lines = 4;
	if (PFA == 1)
	{
		new_addr[0] = addr;
		new_addr[1] = new_addr[0] + (1 << config_.b);
	}
	else if (PFA == 2)
	{
		new_addr[0] = addr & (~(3 << config_.b));
		new_addr[1] = new_addr[0] + (1 << config_.b);
	}
	else if (PFA == 3)
	{
		new_addr[0] = addr & (~(3 << config_.b));
		for (int i = 1; i <= 3; i++)
		{
			new_addr[i] = new_addr[i - 1] + (1 << config_.b);
		}
	}

	for (int i = 0; i < fetch_lines; i++)
	{
		uint64_t tag, set_index, block_offset;
		PartitionAlgorithm(new_addr[i], tag, set_index, block_offset);
		// Miss?
		if (ReplaceDecision(tag, set_index))
		{
			// Choose victim			
			if (RA == 0)
			{
				ReplaceAlgorithmLRU(tag, set_index, stats_, time);
			}
			else if (RA == 1)
			{
				ReplaceAlgorithmLIRS(tag, set_index, stats_, time);
			}
			inCache[new_addr[i] >> config_.b] = 1;
		}
	}
	// Only prefetch from lower layer one time.
	lower_->HandleRequest(new_addr[0], 4, 1, content,
		lower_hit, lower_time);
	time += lower_time;
	stats_.prefetch_num++;
}

