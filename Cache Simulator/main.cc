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
	uint64_t addr;
	int read;
	char content[32];
	int hit = 0, time = 0;

	while (fscanf(input, "%c\t %d\n", &io_op, &addr) != EOF)
	{
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
	//printf("Miss Rate= %f\n", (double)(total_sum - total_hit) / (double)(total_sum));
	printf("Total time= %dns\n", ss.access_time);
	//printf("Total time= %dns\n", total_time);
	printf("Total replacement = %d\n", ss.replace_num);
}

int main(int argc, char* argv[])
{
	char filename[15] = "./1.trace";
	FILE* input;
	CacheConfig_ cc;
	// Default cache sets.
	cc.s = 5;
	cc.e = 3;
	cc.b = 6;
	cc.write_through = 0;
	cc.write_allocate = 1;
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
				cout << "  --s    : Cache has 2^s sets. 5 by default." << endl;
				cout << "           For example, --s=5" << endl;
				cout << "  --e    : Cache's each set has 2^e lines. 3 by default." << endl;
				cout << "           For example, --e=3" << endl;
				cout << "  --b    : Cache's each block has 2^b 4-bytes. 6 by default." << endl;
				cout << "           For example, --b=6" << endl;
				cout << "  --TF   : Write through flag. Write back by default." << endl;
				cout << "  --BF   : Write back flag. Write back by default." << endl;
				cout << "  --AF   : Write allocate flag. Write allocate by default." << endl;
				cout << "  --NF   : Non-Write allocate flag. Write allocate by default." << endl;
				cout << "  --SF   : Enter stepping flag mode. Not SF Mode by default." << endl;
				return 0;
			}
			else if (strstr(argv[i], "--name=") != NULL)
			{
				strcpy(filename, argv[i] + 7);
			}
			else if (strstr(argv[i], "--s=") != NULL)
			{
				cc.s = atoi(argv[i] + 4);
				printf("Set s = %d\n", cc.s);
			}
			else if (strstr(argv[i], "--e=") != NULL)
			{
				cc.e = atoi(argv[i] + 4);
				printf("Set e = %d\n", cc.e);
			}
			else if (strstr(argv[i], "--b=") != NULL)
			{
				cc.b = atoi(argv[i] + 4);
				printf("Set b = %d\n", cc.b);
			}
			else if (strcmp(argv[i], "--TF") == 0)
			{
				cout << "Write through." << endl;
				cc.write_through = 1;
			}
			else if (strcmp(argv[i], "--BF") == 0)
			{
				cout << "Write back." << endl;
				cc.write_through = 0;
			}
			else if (strcmp(argv[i], "--AF") == 0)
			{
				cout << "Write allocate." << endl;
				cc.write_allocate = 1;
			}
			else if (strcmp(argv[i], "--NF") == 0)
			{
				cout << "Non-write allocate." << endl;
				cc.write_allocate = 0;
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

	Memory m;
	Cache l1;
	l1.SetLower(&m);
	l1.SetConfig(cc);
	l1.BuildCache();

	StorageStats s;
	s.access_time = 0;
	m.SetStats(s);
	l1.SetStats(s);

	StorageLatency ml;
	// ml.bus_latency = 6;
	// ml.hit_latency = 100;
	ml.hit_latency = 50;
	m.SetLatency(ml);

	StorageLatency ll;
	// ll.bus_latency = 3;
	// ll.hit_latency = 10;
	ll.bus_latency = 0;
	ll.hit_latency = 0;
	l1.SetLatency(ll);

	Ana_trace(input, l1);
	return 0;
}
