#ifndef _DEBUG
	#define _ITERATOR_DEBUG_LEVEL 0
	#define _HAS_EXCEPTIONS 0
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unordered_map>
#include <vector>
#include "hashmap_robinhood.h"



#define MAX_CONTAINERS	2
#define MAX_TESTS		5


//#define VALUE_TYPE BigObject
#define VALUE_TYPE int



#define TEST(x) do{if(!(x)) { int d = 0; printf("%d", 1/d );}} while(0)



static int g_side_effect = 0;	// here we store some results from loops, so the optimizer won't optimize them away

struct BigObject
{
	std::vector<int>	v1;
	char	buf[16];
	int		buf2[32];
};

struct TimeScope
{
	clock_t m_start;
	int&	m_duration;

	TimeScope(int& duration) : m_start(clock()), m_duration(duration) {}
	~TimeScope()
	{
		clock_t duration = clock() - m_start;
		m_duration = int(duration * 1000.0f / CLOCKS_PER_SEC);
	}
};

int rand_range(int range)
{
	return rand() % range;
}

int		g_times[MAX_CONTAINERS][MAX_TESTS] = { 0 };
char	g_container_names[MAX_CONTAINERS][256];
char	g_test_names[MAX_TESTS][256];


template< class T > void verify_hashmap()
{
	T map;

	TEST(map.find(5) == map.end());

	map[5] = true;
	TEST(map.find(5) != map.end());

	map[10] = true;
	TEST(map.find(10) != map.end());
	TEST(map[10] == true);

	map[11] = true;
	map[21] = true;
	TEST(map.find(11) != map.end());
	TEST(map.find(21) != map.end());
	TEST(map.find(12) == map.end());

	bool found[22] = { false };

	int count = 0;
	for (auto it = map.begin(); it != map.end(); ++it)
	{
		found[it->first] = true;
		count++;
	}

	TEST(count == 4 && found[5] && found[10] && found[11] && found[21]);
}

template< class T, class V > void test_insert1(const char* name, std::vector<std::pair<int,V>>& input, T& map, int container_index, int test_index)
{
	strcpy( g_test_names[test_index], name );
	{
		TimeScope ts(g_times[container_index][test_index]);
		for (int i = 0, n = (int)input.size(); i < n; ++i)
		{
			auto& kv = input[i];
			map[kv.first] = kv.second;
		}
	}
}

template< class T > void test_find1(const char* name, int N, int Range, T& map, int container_index, int test_index)
{
	strcpy(g_test_names[test_index], name);

	srand(12345);
	int found = 0;

	{
		TimeScope ts(g_times[container_index][test_index]);

		for (int i = 0; i < N; ++i)
		{
			int k = rand_range(Range);
			if (map.find(k) != map.end())
			{
				found++;
			}
		}
	}
	g_side_effect += found;
}

template< class T, class V > void test_container(const char* name, int Range, int insert_num, int find_num, int container_index, T& map)
{
	strcpy(g_container_names[container_index], name);

	srand(12345);

	std::vector<std::pair<int, V>> input;

	input.resize(insert_num);
	for (int i = 0; i < insert_num; ++i)
	{
		input[i] = std::make_pair(rand_range(Range), V());
	}

	int test_index = 0;

	test_insert1<T, V>(	"insert           ", input, map, container_index, test_index++);
	test_find1<T>(		"find_range       ", find_num, Range, map, container_index, test_index++);
	test_find1<T>(		"find_range x10   ", find_num, Range * 10, map, container_index, test_index++);
	test_find1<T>(		"find_range x100  ", find_num, Range * 100, map, container_index, test_index++);

	T empty_map;
	test_find1<T>(		"search_empty     ", find_num, Range, empty_map, container_index, test_index++);
}

void print_table()
{
	printf("\t");
	for (int i = 0; i < MAX_CONTAINERS; ++i)
		printf("\t%s ", g_container_names[i]);
	printf("\n");

	for (int i = 0; i < MAX_TESTS; ++i)
	{
		printf("%s", g_test_names[i]);
		for (int j = 0; j < MAX_CONTAINERS; ++j)
			printf("%4dms\t", g_times[j][i]);
		printf("\n");
	}
}

void test(const char* test_name, int Range, int insert_num, int find_num)
{
	printf("\n=== %s =====================================\n", test_name);

	std::unordered_map<int, VALUE_TYPE> stl_map;
	test_container<std::unordered_map<int, VALUE_TYPE>, VALUE_TYPE>(	"STL ", Range, insert_num, find_num, 0, stl_map);

	hashmap_robinhood<int, VALUE_TYPE> rh_map(14);
	test_container<hashmap_robinhood<int, VALUE_TYPE>, VALUE_TYPE>(		"R.H.", Range, insert_num, find_num, 1, rh_map);
	print_table();
	printf("=================================================\n");
}

int main()
{
	verify_hashmap<hashmap_robinhood<int, bool>>();

	test("4k items", 4000, 3500000, 1500000);
	test("8k items", 8000, 4000, 1500000);

	printf("Side effect = %d\n", g_side_effect);

 	return 0;
}