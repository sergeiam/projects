#ifndef _DEBUG
	#define _ITERATOR_DEBUG_LEVEL 0
	#define _HAS_EXCEPTIONS 0
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unordered_map>
#include <vector>
#include <string>

#include "hashmap_robinhood.h"

#ifndef _DEBUG
	#define ASSERT(x)
#else
	#define ASSERT(x) do{if(!(x)) { int a=0; printf("%d", 1/a);}}while(0)
#endif

#define MAX_TESTS 16

#define VALUE_TYPE BigObject
//#define VALUE_TYPE int



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



int				g_times[MAX_TESTS][MAX_TESTS] = { 0 };
std::string		g_container_names[MAX_TESTS], g_test_names[MAX_TESTS];
int				g_containers, g_tests;



template< class T, class V > void test_insert1(const char* name, std::vector<std::pair<int,V>>& input, T& map, int container_index, int test_index)
{
	g_test_names[test_index] = name;
	TimeScope ts(g_times[container_index][test_index]);
	for (int i = 0, n = (int)input.size(); i < n; ++i)
	{
		auto& kv = input[i];
		map[kv.first] = kv.second;
	}
}

template< class T > void test_find1(const char* name, int N, int Range, T& map, int container_index, int test_index)
{
	g_test_names[test_index] = name;

	int found = 0;

	TimeScope ts(g_times[container_index][test_index]);

	for (int i = 0; i < N; ++i)
	{
		int k = rand_range(Range);
		if (map.find(k) != map.end())
		{
			found++;
		}
	}
	g_side_effect += found;
}

// inserts N elements into map, then starts to insert/erase K elements

template< class T, class V > void test_erase(const char* name, int n, int k, int container_index, int test_index)
{
	g_test_names[test_index] = name;

	TimeScope ts(g_times[container_index][test_index]);

	T map;
	for (int i = 0; i < n; ++i)
	{
		map[i] = V();
	}
	for (int i = 0; i < k; ++i)
	{
		int j = rand_range(n);
		map[j] = V();
		auto it = map.find(j);
		map.erase(it);
	}
}

template< class T, class V > void test_container(const char* name, int Range, int insert_num, int find_num, int erase_num, int container_index, T& map)
{
	g_container_names[container_index] = name;

	srand(78432234);

	std::vector<std::pair<int, V>> input;

	input.resize(insert_num);
	for (int i = 0; i < insert_num; ++i)
	{
		input[i] = std::make_pair(rand_range(Range), V());
	}

	g_tests = 0;

	test_insert1<T, V>(	"insert_range     ", input, map, container_index, g_tests++);
	test_find1<T>(		"find_range       ", find_num, Range, map, container_index, g_tests++);
	test_find1<T>(		"find_range x10   ", find_num, Range * 10, map, container_index, g_tests++);
	test_find1<T>(		"find_range x100  ", find_num, Range * 100, map, container_index, g_tests++);
	test_erase<T, V>(   "erase            ", Range, erase_num, container_index, g_tests++);

	T empty_map;
	test_find1<T>(		"search_empty_map ", find_num, Range, empty_map, container_index, g_tests++);

	empty_map[12345] = V();
	test_find1<T>(      "search_1_el_map  ", find_num, Range, empty_map, container_index, g_tests++);
}

void print_statistics(const char* test_name)
{
	printf("\n=== %s =====================================\n", test_name);
	printf("\t");
	for (int i = 0; i < g_containers; ++i)
		printf("\t%s ", g_container_names[i].c_str());
	printf("\n");

	for (int i = 0; i < g_tests; ++i)
	{
		printf("%s", g_test_names[i].c_str());
		printf("%4dms\t", g_times[0][i]);
		for (int j = 1; j < g_containers; ++j)
			printf("%4dms (%d%%)\t", g_times[j][i], g_times[j][i]*100 / g_times[0][i] );
		printf("\n");
	}
}

void test(const char* test_name, int Range, int insert_num, int find_num, int erase_num)
{
	g_containers = 0;

	std::unordered_map<int, VALUE_TYPE> stl_map;
	test_container<std::unordered_map<int, VALUE_TYPE>, VALUE_TYPE>(	"STL ", Range, insert_num, find_num, erase_num, g_containers++, stl_map);

	hashmap_robinhood<int, VALUE_TYPE> rh_map(15);
	test_container<hashmap_robinhood<int, VALUE_TYPE>, VALUE_TYPE>(		"R.H.", Range, insert_num, find_num, erase_num, g_containers++, rh_map);

	print_statistics(test_name);
}

int main()
{
#ifdef _D43EBUG
	int insert_num = 35'000;
	int find_num = 15'000;
#else
	int insert_num = 3'500'000;
	int find_num = 1'500'000;
	int erase_num = 750'000;
#endif

	test(" 4k int range used",  4086, insert_num, find_num, erase_num);
	test(" 8k int range used",  8192, insert_num, find_num, erase_num);
	test("16k int range used", 16384, insert_num, find_num, erase_num);
	test("64k int range used", 65536, insert_num, find_num, erase_num);

	printf("Side effect = %d\n", g_side_effect);

 	return 0;
}