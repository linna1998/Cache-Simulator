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
		// Bypass?
		if (!BypassDecision())
		{
			// 除了bypass，无论如何都会有hit latency		
			time += latency_.bus_latency + latency_.hit_latency;
			stats_.access_time += latency_.bus_latency + latency_.hit_latency;

			PartitionAlgorithm(addr, tag, set_index, block_offset);
			// Miss?
			if (ReplaceDecision(tag, set_index, read))
			{
				stats_.miss_num++;
				// Choose victim
				ReplaceAlgorithm(tag, set_index, stats_, time);
			}
			else
			{
				// return hit & time, Read hit cache
				//PrintCache();
				//printf("READ HIT!\n");
				hit = 1;
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
			time += lower_time;
			stats_.fetch_num++;
		}
	}
	// Write
	else if (read == 0)
	{
		// 除了bypass，无论如何都会有hit latency		
		time += latency_.bus_latency + latency_.hit_latency;
		stats_.access_time += latency_.bus_latency + latency_.hit_latency;

		PartitionAlgorithm(addr, tag, set_index, block_offset);
		// Don't write allocate && write back
		bool miss = ReplaceDecision(tag, set_index, read);
		if (!miss)
		{
			hit = 1;
			// PrintCache();
			// printf("WRITE HIT!\n");			
		}
		else
		{
			hit = 0;
			stats_.miss_num++;
		}
		if (miss && (config_.write_allocate == 1))
		{
			// Load to cache.	
			int load_hit, load_time;
			StorageStats_ old_ss = stats_;
			HandleRequest(addr, bytes, 1, content,
				load_hit, load_time);
			stats_.access_counter = old_ss.access_counter;
			stats_.miss_num = old_ss.miss_num;			
			time += load_time;			

			// ReplaceAlgorithm(tag, set_index, stats_, time);  
			// LRU freshed in Replace Algorithm.
		}
		if (config_.write_allocate == 1 ||
			((!miss) && config_.write_through == 1 && config_.write_allocate == 0))
		{
			// Write to cache, data_cache[set_index].data_set[block_index]
			// Find block index.
			uint64_t block_index;
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
			}
			// ???
			// Write to data_cache[set_index].data_set[block_index].data_block[block_offset]

			// Write dirty bit;
			// if (config_.write_through == 0 && config_.write_allocate == 0)
			if (config_.write_through == 0)
			{
				data_cache[set_index].data_set[block_index].dirty[block_offset] = true;
				// DEBUG
				// printf("write dirty bit\n");
			}
		}

		// Write to lower layer
		if (config_.write_through == 1)
		{
			int lower_hit, lower_time;
			lower_->HandleRequest(addr, bytes, read, content,
				lower_hit, lower_time);
			time += lower_time;
			stats_.fetch_num++;
		}
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
	printf("Partition Algorithm: tag : %lx(%lu)\n", tag, tag);
	printf("Partition Algorithm: set_index : %lx(%lu)\n", set_index, set_index);
	printf("Partition Algorithm: block_offset : %lx(%lu)\n",
		block_offset, block_offset);
	printf("Partition Algorithm: addr : %lx(%lu)\n",
		addr, addr);
}

// return true means the cache miss.
// 在读操作时，要valid才不算miss
// 在写操作时，block在就不算miss
int Cache::ReplaceDecision(uint64_t tag, uint64_t set_index, int read)
{
	if (read == 1)
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
	}
	else if (read == 0)
	{
		for (int i = 0; i < config_.E; i++)
		{
			if (data_cache[set_index].data_set[i].tag == tag)
			{
				return FALSE;
			}
		}
		return TRUE;
	}
	//return FALSE;
}

void Cache::ReplaceAlgorithm(uint64_t tag, uint64_t set_index,
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
	// Dirty && Write_back	
	for (int i = 0; i < config_.B; i++)
	{
		if (data_cache[set_index].data_set[out_index].dirty[i] == true
			&& config_.write_through == 0)
		{
			// ???
			// Write block's data[i] back into memory

			int lower_hit, lower_time;
			char content[32];
			uint64_t addr;
			MergeAlgorithm(addr, data_cache[set_index].data_set[out_index].tag,
				set_index, i);
			lower_->HandleRequest(addr, 4, 0, content,
				lower_hit, lower_time);
			time += lower_time;
			stats_.fetch_num++;
			// DEBUG
			printf("write to lower layer in address %d\n", addr);
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

int Cache::PrefetchDecision()
{
	return FALSE;
}

void Cache::PrefetchAlgorithm()
{
}

