#include <stdio.h>
#include <memory>
#include <vector>

template< typename T > class CONST_UNIQUE_PTR : public std::unique_ptr<T>
{
public:

	CONST_UNIQUE_PTR() = default;
	CONST_UNIQUE_PTR(const CONST_UNIQUE_PTR& rhs) = delete;
	CONST_UNIQUE_PTR(CONST_UNIQUE_PTR&& rhs) { this->swap(rhs);  }
	//CONST_UNIQUE_PTR(std::unique_ptr<T>&& rhs) : std::unique_ptr<T>(std::forward<T>(rhs)) { /*this->swap(rhs);  */}
	CONST_UNIQUE_PTR(T* rhs) : std::unique_ptr<T>(rhs) {}

	T* get() { return std::unique_ptr<T>::get(); }
	const T* get() const { return std::unique_ptr<T>::get(); }

	T* operator-> () {
		return get();
	}
	const T* operator-> () const {
		return get();
	}
	T& operator*() {
		return *get();
	}
	const T& operator*() const {
		return *get();
	}
};

template< typename T >
using CONST_UNIQUE_PTR_VECTOR = std::vector<CONST_UNIQUE_PTR<T>>;


struct SHAPE
{
	float pos[3];

	SHAPE() = default;
	SHAPE(float a) { pos[0] = pos[1] = pos[2] = a; }

	void method()
	{
		printf("method\n");
	}

	void const_method() const
	{
		printf("const_method\n");
	}

	virtual ~SHAPE()
	{
		printf("~SHAPE %d\n", this);
	}
};

struct SHAPE2 : public SHAPE
{
	float uv[2];

	virtual ~SHAPE2()
	{
		printf("~SHAPE2 %X\n", this);
	}
};

void test_const_method(const CONST_UNIQUE_PTR_VECTOR<SHAPE>& v)
{
	v.back()->const_method();
	(*v.back()).const_method();
}
 
int main()
{
	{
		CONST_UNIQUE_PTR_VECTOR<SHAPE>	shapes;
		shapes.push_back(new SHAPE());
		shapes.push_back(new SHAPE2());
		shapes.emplace_back( new SHAPE());

		shapes.back()->method();
		(*shapes.back()).const_method();

		test_const_method(shapes);

		CONST_UNIQUE_PTR<SHAPE> mine = std::move(shapes.back());
		shapes.resize(shapes.size() - 1);

		mine->method();
	}
	
	return 0;
}