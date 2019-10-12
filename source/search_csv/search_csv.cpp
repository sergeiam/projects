#define _ITERATOR_DEBUG_LEVEL 0

#include <Windows.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>
#include <thread>



#define BUF_SIZE		(1 << 16)
#define NUM_THREADS		4
#define MAX_INDEX_SIZE  12



typedef unsigned char		u8;
typedef unsigned short		u16;
typedef unsigned long		u32;
typedef unsigned long long	u64;



std::vector<std::string> s_files;
size_t		s_next_file;
std::mutex	s_mutex_files;
std::mutex	s_mutex_print;

u32 s_exceptions;

u64		s_bytes_processed = 0;
clock_t	s_start_time;





template< typename T > T Min(T a, T b)
{
	return a < b ? a : b;
}

template< typename T > T Max(T a, T b)
{
	return a > b ? a : b;
}

void FindFiles(std::string folder, std::vector<std::string>& files)
{
	if (folder.back() != '/' && folder.back() != '\\')
		folder += "/";

	WIN32_FIND_DATAA fd;
	for (HANDLE h = FindFirstFileA((folder + "*.*").c_str(), &fd); h != INVALID_HANDLE_VALUE; )
	{
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (fd.cFileName[0] != '.')
				FindFiles(folder + std::string(fd.cFileName), files);
		}
		else
		{
			files.push_back(folder + fd.cFileName);
		}

		if (!FindNextFileA(h, &fd))
		{
			FindClose(h);
			break;
		}
	}
}


int utf8_to_ascii(const u8* src, u8* dest, int src_size, int& dest_size, const char* file, int block_offset)
{
	int sptr = 0, dptr = 0;
	for (; sptr < src_size; )
	{
		if (src[sptr] & 0x80)
		{
			if ((src[sptr] & 0xE0) != 0xC)	// non-cyrilic character!
			{
				printf("ERROR: non-cyrilic range char in file '%s' at offset %d\n", file, block_offset + sptr);
				return -1;
			}
			if (sptr + 1 == src_size) return sptr; // next byte is on the next block, return without fetching current byte!

			u32 utf = (u32(src[sptr] & 0x1F) << 6) | (src[sptr + 1] & 0x3F);

			if (utf < 0x410 || utf >= 0x450)
			{
				printf("ERROR: non-cyrilic char in file '%s' at offset %d\n", file, block_offset + sptr);
				return -1;
			}

			dest[dptr++] = 128 + utf - 0x410;
			sptr += 2;
		}
		else
		{
			dest[dptr++] = src[sptr++];
		}
	}
	dest_size = dptr;
	return sptr;
}



int find_chr(const u8* ptr, char chr, int size)
{
	const u8* p0 = (const u8*)memchr(ptr, chr, size);
	return p0 ? p0 - ptr : size;
}

struct String
{
	char* str;
	//int   len;

	String(String&& rhs) : str(rhs.str) /*, len(rhs.len)*/ { rhs.str = nullptr; }
	String() : str(nullptr) /*, len(0)*/ {}
	String(const char* s, int l)
	{
		int len = l;
		str = new char[len + 1];
		strncpy_s(str, len+1, s, len);
		str[len] = 0;
	}
	~String()
	{
		delete[] str;
	}
	const char* c_str() const { return str; }
	size_t size() const { return strlen(str); }
};

namespace std
{
	template<> struct hash<String>
	{
		size_t operator()(const String& s) const
		{
#if 1
			u32 hash = 0;
			for (int i = 0; s.str[i]; ++i)
			{
				hash += s.str[i];
				hash += hash << 10;
				hash ^= hash >> 6;
			}
			hash += hash << 3;
			hash ^= hash >> 11;
			hash += hash << 15;

			return hash;
#else
			u32 hash = 0;
			for (int i = 0; s.str[i]; ++i)
				hash = (hash * 1373) ^ u32(s.str[i]);
			return hash;
#endif
		}
	};
	template<> struct equal_to<String>
	{
		bool operator()(const String& lhs, const String& rhs) const
		{
			return /*lhs.size() == rhs.size() &&*/ !strcmp(lhs.str, rhs.str);
		}
	};
}

bool is_egn_interesting(const char* egn)
{
	return
		!strncmp(egn, "7910065827", 10) ||
		!strncmp(egn, "7612181532", 10) ||
		!strncmp(egn, "761231", 6) ||
		!strncmp(egn, "780612", 6) ||
		!strncmp(egn, "820116", 6) ||
		!strncmp(egn, "830116", 6) ||
		!strncmp(egn, "541224", 6) ||
		!strncmp(egn, "5511068930", 10);
}

enum eTerm
{
	TERM_TEXT,
	TERM_EGN,
	TERM_IGNORE
};

eTerm filter_term(const char* str, int len)
{
	if (len > 40) return TERM_IGNORE;

	if (len > 11 && !strncmp(str, " Record ID:", 11)) return TERM_IGNORE;
	if (len > 8 && !strncmp(str, "validate", 8)) return TERM_IGNORE;
	if (len > 6 && !strncmp(str, "\"<?xml", 6)) return TERM_IGNORE;

	//if (len > 22 && !strncmp(str, "-----BEGIN CERTIFICATE", 22)) return false;

	int letter = 0;
	for (int i = 0; i < len; ++i)
	{
		if ((str[i] >= '0' && str[i] <= '9') || str[i] == '.' || str[i] == '-' || str[i] == ' ' || str[i] == ':') continue;
		letter++;
	}
	if (letter > len / 2) return TERM_TEXT;
	if (len == 10 && letter == 0 && is_egn_interesting(str)) return TERM_EGN;
	return TERM_IGNORE;
}

struct Index
{
	u16 arr[MAX_INDEX_SIZE];

	Index() { memset( arr, 0, sizeof(arr)); }

	u32 size() const
	{
		return MAX_INDEX_SIZE;
	}
	u16 operator[](int index) const
	{
		return arr[index];
	}

	void add_index(int index)
	{
		for (int j = 0; j < MAX_INDEX_SIZE; ++j)
		{
			if (!arr[j] || arr[j] == index)
			{
				arr[j] = index;
				return;
			}
		}
		s_exceptions++;
	}

};

struct ProcessFilesState
{
	std::unordered_map<String, Index, std::hash<String>, std::equal_to<String>> m_terms;
	int m_file_index;

	static std::unordered_map<String, Index, std::hash<String>, std::equal_to<String>> s_egn;
	static std::mutex s_mutex_egn;

	void save_terms(const char* filepath)
	{
		static std::mutex m;

		std::lock_guard<std::mutex> lock(m);

		FILE* fp = nullptr;

		char buf[4096];
		int curr = 0, count = m_terms.size();

		char filename[512];

		for (auto it = m_terms.begin(), it_end = m_terms.end(); it != it_end; ++it, ++curr)
		{
			if (!fp || curr > 8 * 1024 * 1024)
			{
				count -= curr;
				curr = 0;

				if (fp)
				{
					fclose(fp);
					fp = nullptr;
				}

				static int index = 0;

				sprintf(filename, "%sterms_%d.txt", filepath, index++);
				if (fopen_s(&fp, filename, "wt"))
				{
					printf("ERROR: unable to create '%s'\n", filename);
					return;
				}
			}

			u32 str_size = it->first.size();

			memcpy(buf, it->first.c_str(), str_size);
			buf[str_size++] = ',';
			buf[str_size] = 0;

			const Index& index = it->second;

			for (int i = 0, n = index.size(); i < n; ++i)
			{
				u16 x = index[i];
				if (x)
				{
					buf[str_size++] = ' ';
					ltoa(x, buf + str_size, 10);
					str_size += strlen(buf + str_size);
				}
			}

			if (!(curr & 0xFF))
			{
				printf("Saving %s... %d %%\r", filename, curr * 100 / count);
			}

			buf[str_size] = '\n';
			fwrite(buf, 1, str_size + 1, fp);
		}
		fclose(fp);
		printf("\n");
	}

	static void save_egns(const char* filepath)
	{
		static std::mutex m;
		std::lock_guard<std::mutex> lock(m);

		if (s_egn.empty()) return;

		char buf[512];
		sprintf(buf, "%segns.txt", filepath);
		FILE* fp;
		fopen_s(&fp, buf, "wt");

		for (auto it = s_egn.begin(); it != s_egn.end(); ++it)
		{
			u32 str_size = it->first.size();

			memcpy(buf, it->first.c_str(), str_size);
			buf[str_size++] = ',';
			buf[str_size] = 0;

			const Index& index = it->second;

			for (int i = 0, n = index.size(); i < n; ++i)
			{
				u16 x = index[i];
				if (x)
				{
					buf[str_size++] = ' ';
					ltoa(x, buf + str_size, 10);
					str_size += strlen(buf + str_size);
				}
			}

			buf[str_size] = '\n';
			fwrite(buf, 1, str_size + 1, fp);
		}
		fclose(fp);
		s_egn.clear();
	}

	void add_term(const char* s, int len)
	{
		String str(s, len);
		m_terms[std::move(str)].add_index(m_file_index);
	}

	void add_egn(const char* s, int len)
	{
		String str(s, len);
		std::lock_guard<std::mutex> lock(s_mutex_egn);
		s_egn[std::move(str)].add_index(m_file_index);
	}

	int analyze_file_block(const u8* ascii, int size)
	{
		for (int i = 0; i < size; )
		{
			int s0 = find_chr(ascii + i, ',', size - i);
			int s1 = find_chr(ascii + i, '\n', size - i);

			int s = Min(s0, s1);

			if (s + i >= size) return i;

			switch (filter_term((const char*)ascii + i, s))
			{
				case TERM_TEXT:
					add_term((const char*)ascii + i, s);
					break;
				case TERM_EGN:
					add_egn((const char*)ascii + i, s);
					break;
			}
			i += s + 1;
		}
		return size;
	}

	int process_file(const char* file, int file_index)
	{
		FILE* fp;
		if (fopen_s(&fp, file, "rb"))
		{
			printf("ERROR: failed to open '%s'\n", file);
			return 0;
		}

		m_file_index = file_index;

		u8 raw[BUF_SIZE];
		int raw_size = 0, block_offset = 0;

		for (;;)
		{
			int read_size = fread(raw + raw_size, 1, BUF_SIZE - raw_size, fp);
			if (!read_size) break;

			int size = analyze_file_block(raw, raw_size + read_size);

			int total_size = raw_size + read_size;
			memcpy(raw + size, raw, total_size - size);
			raw_size = total_size - size;

			block_offset += read_size;
		}
		fclose(fp);

		return block_offset;
	}
};

std::unordered_map<String, Index, std::hash<String>, std::equal_to<String>> ProcessFilesState::s_egn;
std::mutex ProcessFilesState::s_mutex_egn;

void process_next_file()
{
	ProcessFilesState pfs;
	for (;; )
	{
		s_mutex_files.lock();
		std::string file;
		int file_index = s_next_file + 1;
		if (s_next_file < s_files.size())
			file = s_files[s_next_file++];
		s_mutex_files.unlock();

		if (file.empty()) break;

		int bytes_processed = pfs.process_file(file.c_str(), file_index);

		s_mutex_print.lock();

		s_bytes_processed += bytes_processed;
		float elapsed_sec = float(clock() - s_start_time) / CLOCKS_PER_SEC;
		float mb_per_sec = (s_bytes_processed / float(1 << 20)) / Max( elapsed_sec, 1.0f);

		printf("%d files, %d Mb, %0.2f Mbs (%d exceptions)...\n", s_next_file, u32(s_bytes_processed / (1 << 20)), mb_per_sec, s_exceptions);

		s_mutex_print.unlock();
	}
	pfs.save_terms("E:/MINFIN_BREACH/");

	ProcessFilesState::save_egns("E:/MINFIN_BREACH/");
}

int main()
{
	FindFiles("E:/MINFIN_BREACH/", s_files);
	s_next_file = 0;
	s_start_time = clock();

	FILE* fp = fopen("E:/MINFIN_BREACH/file_list.csv", "wt");
	for (auto it = s_files.begin(); it != s_files.end(); ++it)
	{
		fprintf(fp, "%d, %s\n", it - s_files.begin() + 1, (*it).c_str());
	}
	fclose(fp);

	std::thread * threads[NUM_THREADS];
	for (int i = 0; i < NUM_THREADS; ++i)
		threads[i] = new std::thread(process_next_file);

	for (int i = 0; i < NUM_THREADS; ++i)
	{
		threads[i]->join();
		delete threads[i];
	}

	printf("\nProcessing took %0.2f s\n", float(clock() - s_start_time) / CLOCKS_PER_SEC);

#if 0
	u32 bytes_processed = 0;
	for (size_t i = 0; i < files.size(); ++i)
	{
		printf("\r%d/%d, %d terms, %d Mb...", i + 1, files.size(), s_terms.size(), bytes_processed / (1<<20));
		bytes_processed += process_file(files[i].c_str());
	}
#endif

	return 0;
}
