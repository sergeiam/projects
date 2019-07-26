#include <vector>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <sdk/pugixml/src/pugixml.hpp>
#include <xr/time.h>

#pragma warning( disable : 4996 )

#include "streams.h"




using namespace std;


string rand_string(int len)
{
	string s;
	for (int i = 0; i < len; ++i)
		s.push_back('a' + rand() % 26);
	return s;
}



struct ENTITY_A
{
	int a, b, c;
	float x, y, z;
	string	name, group, type, description;

	struct PROPERTY
	{
		string	name;
		int		value, default_value;
		string	tooltip;

		PROPERTY()
		{
			name = rand_string(5);
			value = rand();
			default_value = rand();
			tooltip = rand_string(32);
		}
	};
	vector<PROPERTY>	properties;

	ENTITY_A()
	{
		a = rand();
		b = rand();
		c = rand();
		x = rand() / RAND_MAX;
		y = rand() / RAND_MAX;
		z = rand() / RAND_MAX;
		name = rand_string(10);
		group = rand_string(6);
		type = rand_string(8);
		description = rand_string(26);

		properties.resize(rand() % 10);
	}
};

template< class STREAM > void serialize(STREAM& s, ENTITY_A::PROPERTY& prop)
{
	s.begin_structure("p");
		serialize(s, "name", prop.name);
		serialize(s, "val", prop.value);
		serialize(s, "def_val", prop.default_value);
		serialize(s, "tooltip", prop.tooltip);
	s.end_structure();
}

template< class STREAM > void serialize(STREAM& s, ENTITY_A& e)
{
	s.begin_structure("ea");
		serialize(s, "a", e.a);
		serialize(s, "b", e.b);
		serialize(s, "c", e.c);
		serialize(s, "x", e.x);
		serialize(s, "y", e.y);
		serialize(s, "z", e.z);
		serialize(s, "name", e.name);
		serialize(s, "group", e.group);
		serialize(s, "type", e.type);
		serialize(s, "descr", e.description);
		serialize(s, "ps", e.properties);
	s.end_structure();
}

int main()
{
	vector<ENTITY_A> test;
	test.resize(32768);

	{
		xr::TIME_SCOPE t;
		STREAM_WRITE_BINARY swb("stream.bin");
		serialize(swb, "test", test);
		printf("Bin write: %d ms\n", t.measure_duration_ms());
	}
	{
		xr::TIME_SCOPE t;
		STREAM_WRITE_XML swx("stream.xml");
		serialize(swx, "test", test);
		printf("Xml write: %d ms\n", t.measure_duration_ms());
	}
	{
		xr::TIME_SCOPE t;
		STREAM_READ_BINARY srb("stream.bin");
		serialize(srb, "test", test);
		printf("Bin read: %d ms\n", t.measure_duration_ms());
	}
	{
		xr::TIME_SCOPE t;
		STREAM_READ_XML srx("stream.xml");
		//serialize(srx, "test", test);
		printf("Xml read: %d ms\n", t.measure_duration_ms());
	}


	return 0;
}