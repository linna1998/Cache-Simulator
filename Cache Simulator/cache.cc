#include "cache.h"
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
	new_block.dirty = false;
	for (int i = 0; i < config_.B; i++)
	{
		new_block.data_block.push_back(0);
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

// ??? read == 1 : read?
void Cache::HandleRequest(uint64_t addr, int bytes, int read,
	char *content, int &hit, int &time)
{
	hit = 0;
	time = 0;
	uint64_t tag;
	uint64_t set_index;
	uint64_t block_offset;

	// Bypass?
	if (!BypassDecision())
	{
		PartitionAlgorithm(addr, tag, set_index, block_offset);
		// Miss?
		if (ReplaceDecision(tag, set_index))
		{
			// Choose victim
			ReplaceAlgorithm(tag, set_index);
		}
		else
		{
			// return hit & time
			hit = 1;
			time += latency_.bus_latency + latency_.hit_latency;
			stats_.access_time += time;
			return;
		}
	}
	// Prefetch?
	if (PrefetchDecision())
	{
		PrefetchAlgorithm();
	}
	else
	{
		// Fetch from lower layer
		int lower_hit, lower_time;
		lower_->HandleRequest(addr, bytes, read, content,
			lower_hit, lower_time);
		hit = 0;
		time += latency_.bus_latency + lower_time;
		stats_.access_time += latency_.bus_latency;
	}
}

int Cache::BypassDecision()
{
	return FALSE;
}

// Read the tag, set_index, block_offset number from the addr.
void Cache::PartitionAlgorithm(uint64_t addr, uint64_t& tag,
	uint64_t & set_index, uint64_t& block_offset)
{
	tag = (addr >> (config_.b + config_.s))&(1 << (config_.e) - 1);
	set_index = (addr >> config_.b)&(1 << (config_.s) - 1);
	block_offset = addr&(1 << (config_.b) - 1);
}

// return true means the cache have to replace some blocks.
int Cache::ReplaceDecision(uint64_t tag, uint64_t set_index)
{
	for (int i = 0; i < config_.E; i++)
	{
		if (data_cache[set_index].data_set[i].tag == tag)
		{
			data_cache[set_index].value[i] = 0;
			for (int j = 0; j < config_.E; j++)
			{
				if (i != j) data_cache[set_index].value[j]++;
			}
			return FALSE;
		}
	}
	return TRUE;
	//return FALSE;
}

void Cache::ReplaceAlgorithm(uint64_t tag, uint64_t set_index)
{
	int out_index = 0;  // replace data_cache[set_index].data_set[out_index]
	for (int i = 0; i < config_.E; i++)
	{
		if (data_cache[set_index].data_set[i].valid > data_cache[set_index].data_set[out_index].valid)
		{
			out_index = i;
		}
	}
	// Dirty && Write_back 
	if (data_cache[set_index].data_set[out_index].dirty == true && config_.write_through == 0)
	{
		// ???
		// Write block's data back into memory
	}
	// Write a new block into cache
	data_cache[set_index].data_set[out_index].dirty = false;
	data_cache[set_index].data_set[out_index].tag = tag;
	data_cache[set_index].data_set[out_index].valid = false;
	// ???
	// Fresh data_cache[set_index].data_set[out_index].data_block;

	// fresh the LRU value
	data_cache[set_index].value[out_index] = 0;
	for (int i = 0; i < config_.E; i++)
	{
		if (i != out_index) data_cache[set_index].value[i]++;
	}
}

int Cache::PrefetchDecision()
{
	return FALSE;
}

void Cache::PrefetchAlgorithm()
{
}

