#include "hashmap_robinhood.h"
#include "hashmap_flatlist.h"


#define TEST(x) do{if(!(x)) { int d = 0; printf("%d", 1/d );}} while(0)


template< class T > void verify_hashmap()
{
	T map;
	T::value_type default_value = T::value_type();

	TEST(map.find(5) == map.end());

	map[5] = default_value;
	TEST(map.find(5) != map.end());

	map[10] = default_value;
	TEST(map.find(10) != map.end());
	TEST(map[10] == default_value);

	map[11] = default_value;
	map[21] = default_value;
	TEST(map.find(11) != map.end());
	TEST(map.find(21) != map.end());
	TEST(map.find(12) == map.end());

	bool found[22] = { false };

	int count = 0;
	for (auto it = map.begin(); it != map.end(); ++it)
	{
		found[it->first] = default_value;
		count++;
	}

	TEST(count == 4 && found[5] == default_value && found[10] == default_value && found[11] == default_value && found[21] == default_value );

	// --- erase

	//auto it = map.erase(map.begin());
	//TEST(map.size() == 3 && it != map.end() && it == map.begin());

	while (!map.empty()) map.erase(map.begin());
	TEST(map.empty());

	// -- clear

	map[5] = default_value;
	TEST(map.size() == 1);

	map.clear();
	TEST(map.empty());
	TEST(map.size() == 0);

	// --- grow

	map.clear();
	for (int i = 0; i < 24; ++i)
	{
		map[i] = default_value;

		int count = 0;
		for (auto it = map.begin(); it != map.end(); ++it, ++count);

		TEST(count == i+1);
	}
	TEST(map.size() == 24);

	map.clear();
	for (int i = 0; i < 1000; ++i)
	{
		map[i] = default_value;
	}
	TEST(map.size() == 1000);

#undef TEST
}

struct Verify
{
	Verify()
	{
		verify_hashmap<hashmap_robinhood<int, bool>>();
		verify_hashmap<hashmap_flatlist<int, bool>>();
	}
};

static Verify v;