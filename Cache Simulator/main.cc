#include "stdio.h"
#include "cache.h"
#include "memory.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace std;
bool SF = false;

void Ana_trace(FILE* &input, Cache &l1)
{
	char io_op;
	char c_addr[20];
	uint64_t addr;
	int read;
	char content[32];
	int hit = 0, time = 0;
	
	for (int i = 0; i < 20; i++)
		c_addr[i] = '\0';

	while (fscanf(input, "%c\t %s\n", &io_op, &c_addr) != EOF)
	{
		// Read in the addr in hex style
		addr = 0;
		int pointer = 2;
		while (pointer < 20 && c_addr[pointer] != '\0')
		{
			if (c_addr[pointer] >= '0' && c_addr[pointer] <= '9')
				addr = addr * 16 + c_addr[pointer] - '0';
			else if (c_addr[pointer] >= 'a' && c_addr[pointer] <= 'f')
				addr = addr * 16 + c_addr[pointer] - 'a' + 10;
			pointer++;
		}
		for (int i = 0; i < 20; i++)
			c_addr[i] = '\0';

		if (SF)
		{
			cin.get();  // Stepping			
			printf("*************************************************************************\n");
			printf("io_op= %c, addr= %lx(%lu)\n", io_op, addr, addr);
		}
		if (io_op == 'w')
		{
			read = 0;
		}
		else if (io_op == 'r')
		{
			read = 1;
		}
		addr >>= 2;
		l1.HandleRequest(addr, 4, read, content, hit, time);
		if (SF)
		{
			printf("Request access time: %d ns\n", time);
			printf("Cache.\n");
			l1.PrintCache();
			printf("*************************************************************************\n");
		}
	}
	StorageStats_ ss;
	l1.GetStats(ss);
	printf("Total hit = %d\n", ss.access_counter - ss.miss_num);
	printf("Total num = %d\n", ss.access_counter);
	printf("Miss Rate= %f\n", (double)(ss.miss_num) / (double)(ss.access_counter));
	printf("Total time= %d cycles\n", ss.access_time);
	printf("Total replacement = %d\n", ss.replace_num);
}

int main(int argc, char* argv[])
{
	char filename[15] = "./1.trace";
	FILE* input;

	// Parse the arguments.
	if (argc != 1)
	{
		for (int i = 1; i < argc; i++)
		{
			if (strcmp(argv[i], "--help") == 0)
			{
				cout << "HELP   :" << endl;
				cout << "  --name : The excutable file's name. './1.trace' by default." << endl;
				cout << "           For example, --name=./1.trace" << endl;
				cout << "  --SF   : Enter stepping flag mode. Not SF Mode by default." << endl;
				return 0;
			}
			else if (strstr(argv[i], "--name=") != NULL)
			{
				strcpy(filename, argv[i] + 7);
			}
			else if (strcmp(argv[i], "--SF") == 0)
			{
				cout << "Stepping flag." << endl;
				SF = true;
			}
			else
			{
				cout << "ERROR:  Unknown flags. See --help." << endl;
				return 0;
			}
		}
	}
	// Open the file.
	input = fopen(filename, "r");
	if (input == NULL)
	{
		printf("Input trace file not found!\n");
		return 0;
	}

	// Set cache.
	CacheConfig_ cc1, cc2;
	cc1.s = 6;
	cc1.e = 3;
	cc1.b = 4;
	cc1.write_through = 0;
	cc1.write_allocate = 1;

	cc2.s = 9;
	cc2.e = 3;
	cc2.b = 4;
	cc2.write_through = 0;
	cc2.write_allocate = 1;

	Memory m;
	Cache l1, l2;
	l1.SetLower(&l2);
	l1.SetConfig(cc1);
	l1.BuildCache();
	l2.SetLower(&m);
	l2.SetConfig(cc2);
	l2.BuildCache();

	StorageStats s;
	s.access_time = 0;
	l1.SetStats(s);
	l2.SetStats(s);
	m.SetStats(s);

	StorageLatency ml;
	// ml.bus_latency = 6;	
	ml.hit_latency = 100;
	m.SetLatency(ml);

	StorageLatency ll1, ll2;
	ll1.bus_latency = 0;
	ll1.hit_latency = 3;
	l1.SetLatency(ll1);
	ll2.bus_latency = 6;
	ll2.hit_latency = 4;
	l2.SetLatency(ll2);

	Ana_trace(input, l1);
	return 0;
}
