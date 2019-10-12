#include <charconv>
#include <stdio.h>
#include <stdlib.h>
#include <xr/time.h>

#define TEST_SIZE			500000
#define MAX_CHARS_PER_NUMBER (3+1+4+1)

int main()
{
	char* ps = new char[TEST_SIZE * MAX_CHARS_PER_NUMBER];

	for (int i = 0; i < TEST_SIZE; ++i)
	{
		float f = 999.0f * rand() / RAND_MAX;
		sprintf_s(ps + i * MAX_CHARS_PER_NUMBER, MAX_CHARS_PER_NUMBER, "%3.4f", f);
	}

	xr::TIME_SCOPE scope;

	float sum = 0.0f;
	for (int i = 0; i < TEST_SIZE; ++i)
	{
		float f = strtof(ps + i * MAX_CHARS_PER_NUMBER, NULL);
		sum += f;
	}
	printf("strtof: %d ms (%0.2f)\n", scope.measure_duration_ms(), sum);

	scope.reset();

	sum = 0.0f;
	for (int i = 0; i < TEST_SIZE; ++i)
	{
		float f = atof(ps + i * MAX_CHARS_PER_NUMBER);
		sum += f;
	}
	printf("atof: %d ms (%0.2f)\n", scope.measure_duration_ms(), sum);

	scope.reset();

	sum = 0.0f;
	for (int i = 0; i < TEST_SIZE; ++i)
	{
		float f;
		std::from_chars(ps + i * MAX_CHARS_PER_NUMBER, ps + (i+1) * MAX_CHARS_PER_NUMBER, f);
		sum += f;
	}
	printf("from_chars: %d ms (%0.2f)\n", scope.measure_duration_ms(), sum);

	return 0;
}