#include "stdio.h"
#include "cache.h"
#include "memory.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace std;
bool SF = false;

void Ana_trace(FILE* &input, Cache &l1, 
	uint64_t &total_hit, uint64_t &total_time, uint64_t &total_access_counter)
{
	char io_op;
	char c_addr[30];
	uint64_t addr;
	int read;
	char content[32];
	int hit = 0, time = 0;		
	for (int i = 0; i < 20; i++)
		c_addr[i] = '\0';	
	while (fscanf(input, "%c\t %s\n", &io_op, c_addr) != EOF)	
	{			
		total_access_counter++;
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
		total_hit += hit;
		total_time += time;

		if (SF)
		{
			printf("Hit: %d\n", hit);
			printf("Access time: %d cycles\n", time);
			l1.PrintStatus();
			// printf("L1 Cache.\n");
			// l1.PrintCache();
			// printf("*************************************************************************\n");
		}
	}
	l1.PrintStatus();
	printf("Total: \n");
	printf("Total hit = %d\n", total_hit);
	printf("Num = %d\n", total_access_counter);
	printf("Time= %lld cycles\n", total_time);
	printf("AMAT = %f \n", (double)total_time / (double)total_access_counter);
}

int main(int argc, char* argv[])
{
	char filename[40] = "./01-mcf-gem5-xcg.trace";
	FILE* input;
	int PFA = 1;  // Prefetch Algorithm.
	// Parse the arguments.
	if (argc != 1)
	{
		for (int i = 1; i < argc; i++)
		{
			if (strcmp(argv[i], "--help") == 0)
			{
				cout << "HELP   :" << endl;
				cout << "  --name : The excutable file's name. './01-mcf-gem5-xcg.trace' by default." << endl;
				cout << "           For example, --name=./01-mcf-gem5-xcg.trace" << endl;
				cout << "  --PFA : The prefetch algorithm." << endl;				
				cout << "           --PFA=0 means no prefetch algorithm." << endl;
				cout << "           --PFA=1 means next-line prefetch algorithm." << endl;
				cout << "           --PFA=2 means prefetch 2 lines." << endl;
				cout << "           --PFA=3 means prefetch 4 lines." << endl;
				cout << "           Next-line prefetch algorithm by default." << endl;
				cout << "  --SF   : Enter stepping flag mode. Not SF Mode by default." << endl;
				return 0;
			}
			else if (strstr(argv[i], "--name=") != NULL)
			{
				strcpy(filename, argv[i] + 7);
			}
			else if (strstr(argv[i], "--PFA=") != NULL)
			{
				PFA=atoi(argv[i] + 6);
				if (PFA==0) cout << "No prefetch algorithm." << endl;
				else if (PFA==1) cout << "Next-line prefetch algorithm." << endl;
				else if (PFA==2) cout << "Prefetch 2 lines." << endl;
				else if (PFA==3) cout << "Prefetch 4 lines." << endl;
				else
				{
					cout << "ERROR:  Unknown prefetch algorithm flags. See --help." << endl;
					return 0;
				}
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
	l1.PFA = PFA;
	l2.SetLower(&m);
	l2.SetConfig(cc2);
	l2.BuildCache();
	l2.PFA = PFA;
	
	StorageStats ss;
	ss.access_counter = 0;
	ss.access_time = 0;
	ss.bypass_num = 0;
	ss.fetch_num = 0;
	ss.miss_num = 0;
	ss.prefetch_num = 0;
	ss.prefetch_num = 0;
	ss.replace_num = 0;
	l1.SetStats(ss);
	l2.SetStats(ss);
	m.SetStats(ss);

	StorageLatency ml;
	ml.bus_latency = 0;	
	ml.hit_latency = 100;
	m.SetLatency(ml);

	StorageLatency ll1, ll2;
	ll1.bus_latency = 0;
	ll1.hit_latency = 3;
	l1.SetLatency(ll1);
	ll2.bus_latency = 6;
	ll2.hit_latency = 4;
	l2.SetLatency(ll2);
		
	uint64_t total_hit = 0, total_time = 0, total_access_counter = 0;
	for (int i = 1; i <= 7; i++)
	{
		// Open the file.
		input = fopen(filename, "r");
		if (input == NULL)
		{
			printf("Input trace file not found!\n");
			return 0;
		}
		printf("******************************************************\n");
		printf("Analyze trace %d times\n", i);
		Ana_trace(input, l1, total_hit, total_time, total_access_counter);
	}
	return 0;
}
