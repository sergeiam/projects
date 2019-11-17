#include "hashmap_robinhood.h"

template< class T > void verify_hashmap()
{
	T map;

#define TEST(x) do{if(!(x)) { int d = 0; printf("%d", 1/d );}} while(0)

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

	// --- erase

	//auto it = map.erase(map.begin());
	//TEST(map.size() == 3 && it != map.end() && it == map.begin());

	while (!map.empty()) map.erase(map.begin());
	TEST(map.empty());

	// -- clear

	map[5] = true;
	TEST(map.size() == 1);

	map.clear();
	TEST(map.empty());
	TEST(map.size() == 0);

	// --- grow
	for (int i = 0; i < 1000; ++i)
	{
		map[i] = true;
	}
	TEST(map.size() == 1000);

#undef TEST
}

struct Verify
{
	Verify()
	{
		verify_hashmap<hashmap_robinhood<int, bool>>();
	}
};

static Verify v;