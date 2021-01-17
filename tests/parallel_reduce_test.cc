// clang++ -stdlib=libc++ -std=c++1y  tests/parallel_reduce_test.cc && ./a.out
#include "../src/parallel_reduce.h"
#include <vector>
#include <iostream>
#include <chrono>
#include <thread>

using std::shared_ptr;
using std::string;

int main()
{

	// std::cout << parallel_reduce<int>({1}, [](auto a, auto b) { return a + b; }).get() << "\n";

	std::cout << parallel_reduce<int>({1, 2},
																		[](auto a, auto b) {
																			// std::this_thread::sleep_for(std::chrono::seconds(1));
																			return a + b;
																		})
									 .get()
						<< "\n";
	std::cout << parallel_reduce<int>({1, 2, 3}, [](auto a, auto b) { return a + b; }).get() << "\n";
	std::cout
			<< parallel_reduce<int>({1, 2, 3, 4, 5, 6, 7}, [](auto a, auto b) { return a + b; }).get()
			<< "\n";

	// std::cout << parallel_reduce<int>(values, [](auto a, auto b) { return a + b; }).get() << "\n";
}
