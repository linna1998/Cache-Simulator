#include "stdio.h"
#include "cache.h"
#include "memory.h"
#include <iostream>
using namespace std;
void Ana_trace(Cache &l1)
{
	FILE* input;
	char io_op;
	uint64_t addr;
	int read;
	char content[32];
	int hit = 0, total_hit = 0;
	int time = 0, total_time = 0;
	int total_sum = 0;
	input = fopen("1.trace", "r");
	if (input == NULL)
	{
		printf("Input trace file not found!\n");
		return;
	}
	while (fscanf(input, "%c\t %lx\n", &io_op, &addr) != EOF)
	{
		//cin.get();
		printf("*************************************************************************\n");
		printf("io_op= %c, addr= %lx\n", io_op, addr);
		total_sum++;
		if (io_op == 'w')
		{
			read = 0;
			printf("addr=%lx(%lu). Write.\n", addr, addr);
			l1.HandleRequest(addr, 1, read, content, hit, time);
			printf("Request access time: %dns\n", time);
			total_hit += hit;
			total_time += time;
		}
		else if (io_op == 'r')
		{
			read = 1;
			printf("addr=%lx(%lu). Read.\n", addr, addr);
			l1.HandleRequest(addr, 1, read, content, hit, time);
			printf("Request access time: %dns\n", time);
			total_hit += hit;
			total_time += time;
		}

		printf("*************************************************************************\n");		
	}
	printf("Total hit = %d\n", total_hit);
	printf("Total num = %d\n", total_sum);
	printf("Miss Rate= %f\n", (double)(total_hit)/(double)(total_sum));
	printf("Total time= %dns\n", total_time);
}
int main(void)
{
	Memory m;
	Cache l1;
	l1.SetLower(&m);
	CacheConfig_ cc;
	// ÀÊ ÷…Ëµƒ
	cc.write_through = 0;
	cc.write_allocate = 0;
	cc.b = 4;
	cc.e = 3;
	cc.s = 4;
	l1.SetConfig(cc);
	l1.BuildCache();

	StorageStats s;
	s.access_time = 0;
	m.SetStats(s);
	l1.SetStats(s);

	StorageLatency ml;
	ml.bus_latency = 6;
	ml.hit_latency = 100;
	m.SetLatency(ml);

	StorageLatency ll;
	ll.bus_latency = 3;
	ll.hit_latency = 10;
	l1.SetLatency(ll);

	int hit, time;
	char content[32];
	l1.HandleRequest(0, 0, 1, content, hit, time);
	printf("Request access time: %dns\n", time);
	l1.HandleRequest(1024, 0, 1, content, hit, time);
	printf("Request access time: %dns\n", time);

	l1.GetStats(s);
	printf("Total L1 access time: %dns\n", s.access_time);
	m.GetStats(s);
	printf("Total Memory access time: %dns\n", s.access_time);

	l1.HandleRequest(0, 0, 0, content, hit, time);
	printf("Request access time: %dns\n", time);
	l1.HandleRequest(1024, 0, 0, content, hit, time);
	printf("Request access time: %dns\n", time);

	l1.GetStats(s);
	printf("Total L1 access time: %dns\n", s.access_time);
	m.GetStats(s);
	printf("Total Memory access time: %dns\n", s.access_time);

	Ana_trace(l1);
	return 0;
}
