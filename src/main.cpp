#include <print>
#include <vector>
#include <meta>

struct TT
{
	int a{};
};

int main() {
	std::vector<int> vvv;
	std::vector v{ 1,2 };
	std::println("hello {}", std::meta::info(^^TT));

}
