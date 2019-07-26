#pragma once

#include <vector>
#include <string>
#include <stdio.h>
#include <sdk/pugixml/src/pugixml.hpp>

using namespace std;


#if 1
struct STREAM_WRITE_BINARY
{
	STREAM_WRITE_BINARY(const char* filename)
	{
		m_filename = filename;
		m_buffer.reserve(16384);
	}
	~STREAM_WRITE_BINARY()
	{
		FILE* fp = fopen(m_filename.c_str(), "wb");
		if (fp)
		{
			fwrite(&m_buffer[0], 1, m_buffer.size(), fp);
			fclose(fp);
		}
	}
	template< class T > void serialize_pod(T& x)
	{
		return serialize_memory(&x, sizeof(T));
	}
	template< class T >	void serialize_vector(vector<T>& x)
	{
		size_t size = x.size();
		serialize_pod(size);
		for (size_t i = 0; i < size; ++i)
		{
			serialize(*this, x[i]);
		}
	}
	void serialize_string(string& x)
	{
		short size = (short)x.size();
		serialize_pod(size);
		serialize_memory(&x[0], size);
	}
	void begin_structure(const char*) {}
	void end_structure() {}

private:
	void serialize_memory(void* ptr, int size)
	{
		size_t s = m_buffer.size();
		m_buffer.resize(s + size);
		memcpy(&m_buffer[s], ptr, size);
	}

	vector<char>	m_buffer;
	string			m_filename;
};

void serialize(STREAM_WRITE_BINARY& s, const char* tag, int& x)
{
	s.serialize_pod(x);
}

void serialize(STREAM_WRITE_BINARY& s, const char* tag, float& x)
{
	s.serialize_pod(x);
}

void serialize(STREAM_WRITE_BINARY& s, const char* tag, bool& x)
{
	s.serialize_pod(x);
}

void serialize(STREAM_WRITE_BINARY& s, const char* tag, string& x)
{
	s.serialize_string(x);
}

template< class T > void serialize(STREAM_WRITE_BINARY& s, const char* tag, vector<T>& x)
{
	s.serialize_vector(x);
}
#endif


#if 1
struct STREAM_WRITE_XML
{
	STREAM_WRITE_XML(const char* filename)
	{
		m_filename = filename;
		m_node = m_doc.append_child("file");
	}
	~STREAM_WRITE_XML()
	{
		m_doc.save_file(m_filename.c_str());
	}
	template< class T > void serialize_pod(const char* name, T& x)
	{
		m_node.append_attribute(name).set_value(x);
	}
	template< class T >	void serialize_vector(const char* name, vector<T>& x)
	{
		begin_structure(name);
		for (auto it = x.begin(); it != x.end(); ++it)
		{
			serialize(*this, *it);
		}
		end_structure();
	}
	void serialize_string(const char* name, string& x)
	{
		m_node.append_attribute(name).set_value(x.c_str());
	}
	void begin_structure(const char* name)
	{
		m_node = m_node.append_child(name);
	}
	void end_structure()
	{
		m_node = m_node.parent();
	}

private:
	pugi::xml_document	m_doc;
	pugi::xml_node		m_node;
	string				m_filename;
};

void serialize(STREAM_WRITE_XML& s, const char* tag, int& x)
{
	s.serialize_pod(tag, x);
}

void serialize(STREAM_WRITE_XML& s, const char* tag, float& x)
{
	s.serialize_pod(tag, x);
}

void serialize(STREAM_WRITE_XML& s, const char* tag, bool& x)
{
	s.serialize_pod(tag, x);
}

void serialize(STREAM_WRITE_XML& s, const char* tag, string& x)
{
	s.serialize_string(tag, x);
}

template< class T > void serialize(STREAM_WRITE_XML& s, const char* tag, vector<T>& x)
{
	s.serialize_vector(tag, x);
}
#endif


#if 1
struct STREAM_READ_BINARY
{
	STREAM_READ_BINARY(const char* filename)
	{
		FILE* fp = fopen(filename, "rb");
		if (fp)
		{
			fseek(fp, 0, SEEK_END);
			long size = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			m_buffer.resize(size);
			fread(&m_buffer[0], 1, size, fp);
			fclose(fp);
		}
		m_curr = 0;
	}
	~STREAM_READ_BINARY()
	{
	}
	template< class T > void serialize_pod(T& x)
	{
		return serialize_memory(&x, sizeof(T));
	}
	template< class T >	void serialize_vector(vector<T>& x)
	{
		size_t size = x.size();
		serialize_pod(size);
		for (size_t i = 0; i < size; ++i)
		{
			serialize(*this, x[i]);
		}
	}
	void serialize_string(string& x)
	{
		short size = (short)x.size();
		serialize_pod(size);
		serialize_memory(&x[0], size);
	}
	void begin_structure(const char*) {}
	void end_structure() {}

private:
	void serialize_memory(void* ptr, int size)
	{
		memcpy(ptr, &m_buffer[m_curr], size);
		m_curr += size;
	}

	vector<char>	m_buffer;
	int				m_curr;
};

void serialize(STREAM_READ_BINARY& s, const char* tag, int& x)
{
	s.serialize_pod(x);
}

void serialize(STREAM_READ_BINARY& s, const char* tag, float& x)
{
	s.serialize_pod(x);
}

void serialize(STREAM_READ_BINARY& s, const char* tag, bool& x)
{
	s.serialize_pod(x);
}

void serialize(STREAM_READ_BINARY& s, const char* tag, string& x)
{
	s.serialize_string(x);
}

template< class T > void serialize(STREAM_READ_BINARY& s, const char* tag, vector<T>& x)
{
	s.serialize_vector(x);
}
#endif


struct STREAM_READ_XML
{
	STREAM_READ_XML(const char* filename)
	{
		m_doc.load_file(filename);
		m_nodes.push_back( m_doc.first_child());
	}
	~STREAM_READ_XML()
	{
	}
	void serialize_pod(const char* name, int& x)
	{
		x = m_nodes.back().attribute(name).as_int();
	}
	void serialize_pod(const char* name, short& x)
	{
		x = m_nodes.back().attribute(name).as_int();
	}
	void serialize_pod(const char* name, float& x)
	{
		x = m_nodes.back().attribute(name).as_float();
	}
	void serialize_pod(const char* name, bool& x)
	{
		x = m_nodes.back().attribute(name).as_bool();
	}
	template< class T >	void serialize_vector(const char* name, vector<T>& x)
	{
		begin_structure(name);
		for (auto it = x.begin(); it != x.end(); ++it)
		{
			serialize(*this, *it);
		}
		end_structure();
	}
	void serialize_string(const char* name, string& x)
	{
		x = m_nodes.back().attribute(name).as_string();
	}
	void begin_structure(const char* name)
	{
		m_nodes.back().ch
/*		m_nodes.push_back( m_nodes.back().first_)
		m_nodes.back()
		m_node = m_node.find_child(name);
		m_node.ne*/
	}
	void end_structure()
	{
		m_nodes.back() = m_nodes.back().next_sibling();
		if (m_nodes.back().empty())
			m_nodes.pop_back();
	}

private:
	pugi::xml_document	m_doc;
	pugi::xml_node		m_node;
	vector<pugi::xml_node>	m_nodes;
};

void serialize(STREAM_READ_XML& s, const char* tag, int& x)
{
	s.serialize_pod(tag, x);
}

void serialize(STREAM_READ_XML& s, const char* tag, float& x)
{
	s.serialize_pod(tag, x);
}

void serialize(STREAM_READ_XML& s, const char* tag, bool& x)
{
	s.serialize_pod(tag, x);
}

void serialize(STREAM_READ_XML& s, const char* tag, string& x)
{
	s.serialize_string(tag, x);
}

template< class T > void serialize(STREAM_READ_XML& s, const char* tag, vector<T>& x)
{
	s.serialize_vector(tag, x);
}