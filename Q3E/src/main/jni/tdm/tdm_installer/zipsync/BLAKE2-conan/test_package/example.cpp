#include <array>
#include <blake2.h>
#include <iomanip>
#include <iostream>

int main() {
	const std::string input = "hello world";
	std::array<unsigned char, BLAKE2B_OUTBYTES> output;

	blake2(output.data(), output.size(), input.data(), input.size(), nullptr, 0);

	std::cout << std::setfill('0') << std::hex;
	for (auto byte : output)
		std::cout << std::setw(2) << static_cast<unsigned>(byte);
	std::cout << std::endl;
}
