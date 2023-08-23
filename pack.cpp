//
// Testing the packing/unpacking of 3d normal vectors.
// Compile with:
// g++ -std=c++20 -Wall -o pack pack.cpp
//
// Published under "The Unlicense" license.
// You are absolutely free to do whatever you please with this software,
// including removing this note.
//

#include <cstdint>
#include <glm/glm.hpp>
#include <iostream>
#include <random>

constexpr bool float_eq(float x, float y)
{
	return glm::abs(x - y) < 0.005f; // Keep the epsilon as low as possible
}

constexpr float map_ufnorm(float x)
{
	return (x + 1.0f) * 0.5f;
}

static_assert(float_eq(map_ufnorm(-1.0), 0.0f));
static_assert(float_eq(map_ufnorm(0.0), 0.5f));
static_assert(float_eq(map_ufnorm(1.0), 1.0f));

constexpr float map_sfnorm(float x)
{
	return x * 2.0 - 1.0f;
}

static_assert(float_eq(map_sfnorm(0.0f), -1.0f));
static_assert(float_eq(map_sfnorm(0.5f), 0.0f));
static_assert(float_eq(map_sfnorm(1.0f), 1.0f));

constexpr uint32_t map_ftou15(float x)
{
	constexpr float s = glm::pow(2.0f, 15.0f) - 1.0f; // 32768 - 1 = 32767
	return (uint32_t)(glm::round(x * s)) & 0x0000FFFF; // eliminate overflow (?)
}

static_assert(map_ftou15(0.0f) == 0U);
static_assert(map_ftou15(1.0f) == 32767U);

constexpr uint32_t map_ftou16(float x)
{
	constexpr float s = glm::pow(2.0f, 16.0f) - 1.0f; // 65536 - 1 = 65535
	return (uint32_t)(glm::round(x * s));
}

static_assert(map_ftou16(0.0f) == 0U);
static_assert(map_ftou16(1.0f) == 65535U);

uint32_t pack(glm::vec3 n)
{
	float ufx = map_ufnorm(n.x);
	float ufy = map_ufnorm(n.y);
	uint32_t ux = map_ftou16(ufx);
	uint32_t uy = map_ftou15(ufy);
	uint32_t ua = (n.z < 0.0f) ? 1 : 0;
	uint32_t pack = (ux << 16) | (uy << 1) | ua;
	return pack;
}

glm::vec3 unpack(uint32_t p)
{
	uint32_t x = p >> 16;
	uint32_t y = (p & 0x0000FFFF) >> 1;
	uint32_t a = p & 0x01;
	float fx = map_sfnorm(float(x) / 65535.0f);
	float fy = map_sfnorm(float(y) / 32767.0f);
	float fa = (a == 1) ? -1.0f : 1.0f;
	float fz = glm::sqrt(1.0f - (fx*fx + fy*fy)) * fa;
	return { fx, fy, fz };
}

bool vector_equals(glm::vec3 v1, glm::vec3 v2)
{
	return float_eq(v1.x, v2.x) && float_eq(v1.y, v2.y) && float_eq(v1.z, v2.z);
}

std::ostream& operator<<(std::ostream& s, glm::vec3 v)
{
	return s << "[ " << v.x << ' ' << v.y << ' ' << v.z << " ]";
}

int test(glm::vec3 n)
{
	uint32_t p = pack(n);
	glm::vec3 u = unpack(p);
	if (vector_equals(n, u))
	{
		std::cout << "SUCCESS: " << n << " --> " << p << " --> " << u << '\n';
		return 0;
	}
	else
	{
		std::cout << ">>> FAIL: " << n << " --> " << p << " --> " << u << '\n';
		return 1;
	}
}

int main()
{
	int err = 0;

	// unit axis vectors
	err += test({1.0f, 0.0f, 0.0f});
	err += test({0.0f, 1.0f, 0.0f});
	err += test({0.0f, 0.0f, 1.0f});

	err += test({-1.0f, 0.0f, 0.0f});
	err += test({0.0f, -1.0f, 0.0f});
	err += test({0.0f, 0.0f, -1.0f});

	// single 0 axis
	err += test(glm::normalize(glm::vec3{1.0f, 1.0f, 0.0f}));
	err += test(glm::normalize(glm::vec3{1.0f, 0.0f, 1.0f}));
	err += test(glm::normalize(glm::vec3{0.0f, 1.0f, 1.0f}));

	err += test(glm::normalize(glm::vec3{-1.0f, -1.0f, 0.0f}));
	err += test(glm::normalize(glm::vec3{-1.0f, 0.0f, -1.0f}));
	err += test(glm::normalize(glm::vec3{0.0f, -1.0f, -1.0f}));

	err += test(glm::normalize(glm::vec3{1.0f, -1.0f, 0.0f}));
	err += test(glm::normalize(glm::vec3{-1.0f, 1.0f, 0.0f}));
	err += test(glm::normalize(glm::vec3{1.0f, 0.0f, -1.0f}));
	err += test(glm::normalize(glm::vec3{-1.0f, 0.0f, 1.0f}));
	err += test(glm::normalize(glm::vec3{0.0f, 1.0f, -1.0f}));
	err += test(glm::normalize(glm::vec3{0.0f, -1.0f, 1.0f}));

	// random normals
	std::random_device rdev;
	std::default_random_engine re(rdev());

	constexpr int rtests = 100;
	for (int i = 0; i < rtests; i++)
	{
		using dist_type = std::uniform_real_distribution<float>;
		dist_type uni(-1.0f, 1.0f);
		float x = static_cast<float>(uni(re));
		float y = static_cast<float>(uni(re));
		float z = static_cast<float>(uni(re));

		err += test(glm::normalize(glm::vec3(x, y, z)));
	}

	std::cout << "\nErrors: " << err << '\n';
	return 0;
}
