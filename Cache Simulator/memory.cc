#include "memory.h"

void Memory::HandleRequest(uint64_t addr, int bytes, int read,
	char *content, int &hit, int &time)
{
	stats_.access_counter++;
	hit = 1;
	// printf("Memory time: old %d\n", time);
	// printf("latency_.hit_latency %d", latency_.hit_latency);
	// printf("latency_.bus_latency %d", latency_.bus_latency);
	time = latency_.hit_latency + latency_.bus_latency;
	// printf("Memory time: new %d\n", time);
	// printf("Memory time access_time: old %d\n", stats_.access_time);
	stats_.access_time += time;
	// printf("Memory time access_time: new %d\n", stats_.access_time);
}

