#include <vector>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <sdk/pugixml/src/pugixml.hpp>
#include <xr/time.h>

#pragma warning( disable : 4996 )

#include "streams.h"

//#define USE_STRINGS
//#define USE_INTS
#define USE_FLOATS


using namespace std;


string rand_string(int len)
{
	string s;
	for (int i = 0; i < len; ++i)
		s.push_back('a' + rand() % 26);
	return s;
}

float randf(float a, float b)
{
	return a + (b - a) * rand() / RAND_MAX;
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
		float	scale1, scale2, scale3;
		float	g1, g2, g3, g4;

		PROPERTY()
		{
			name = rand_string(5);
			value = rand();
			default_value = rand();
			tooltip = rand_string(32);
			scale1 = randf(0.0f, 1.0f);
			scale2 = randf(0.0f, 1.0f);
			scale3 = randf(0.0f, 1.0f);
			g1 = randf(0.0f, 1000.0f);
			g2 = randf(0.0f, 1000.0f);
			g3 = randf(0.0f, 1000.0f);
			g4 = randf(0.0f, 1000.0f);
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
#ifdef USE_STRINGS
		serialize(s, "name", prop.name);
		serialize(s, "tooltip", prop.tooltip);
#endif
#ifdef USE_INTS
		serialize(s, "val", prop.value);
		serialize(s, "def_val", prop.default_value);
#endif
#ifdef USE_FLOATS
		serialize(s, "scale1", prop.scale1);
		serialize(s, "scale2", prop.scale2);
		serialize(s, "scale3", prop.scale3);
		serialize(s, "g1", prop.g1);
		serialize(s, "g2", prop.g2);
		serialize(s, "g3", prop.g3);
		serialize(s, "g4", prop.g4);
#endif
	s.end_structure();
}

template< class STREAM > void serialize(STREAM& s, ENTITY_A& e)
{
	s.begin_structure("ea");
#ifdef USE_INTS
		serialize(s, "a", e.a);
		serialize(s, "b", e.b);
		serialize(s, "c", e.c);
#endif
#ifdef USE_FLOATS
		serialize(s, "x", e.x);
		serialize(s, "y", e.y);
		serialize(s, "z", e.z);
#endif
#ifdef USE_STRINGS
		serialize(s, "name", e.name);
		serialize(s, "group", e.group);
		serialize(s, "type", e.type);
		serialize(s, "descr", e.description);
#endif
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
		serialize(srx, "test", test);
		printf("Xml read: %d ms\n", t.measure_duration_ms());
	}


	return 0;
}