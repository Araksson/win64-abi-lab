#include <Windows.h>
#include <cstdint>
#include "Exports.h"

static constexpr int SCREEN_WIDTH = 1920;
static constexpr int SCREEN_HEIGHT = 1080;

alignas(64) static std::uint8_t hScreenPixels[SCREEN_HEIGHT][SCREEN_WIDTH] = {};

[[clang::noinline]]
static void SetPixelNoInline(int nX, int nY, std::uint8_t nPixelVal)
{
	hScreenPixels[nY][nX] = nPixelVal;
}

__attribute__((always_inline)) inline
static void SetPixelInline(int nX, int nY, std::uint8_t nPixelVal)
{
	hScreenPixels[nY][nX] = nPixelVal;
}

std::uint64_t CleanScreenNoInline()
{
	for (int y = 0; y < SCREEN_HEIGHT; ++y)
	{
		for (int x = 0; x < SCREEN_WIDTH; ++x)
		{
			SetPixelNoInline(x, y, static_cast<std::uint8_t>((x + y) % 0xFF));
		}
	}

	//checksum prevents dead-store elimination
	std::uint64_t qwSum = 0;
	for (int y = 0; y < SCREEN_HEIGHT; ++y)
	{
		for (int x = 0; x < SCREEN_WIDTH; ++x)
		{
			qwSum += hScreenPixels[y][x];
		}
	}

	return qwSum;
}

std::uint64_t CleanScreenInline()
{
	for (int y = 0; y < SCREEN_HEIGHT; ++y)
	{
		for (int x = 0; x < SCREEN_WIDTH; ++x)
		{
			SetPixelInline(x, y, static_cast<std::uint8_t>((x + y) % 0xFF));
		}
	}

	//checksum prevents dead-store elimination
	std::uint64_t qwSum = 0;
	for (int y = 0; y < SCREEN_HEIGHT; ++y)
	{
		for (int x = 0; x < SCREEN_WIDTH; ++x)
		{
			qwSum += hScreenPixels[y][x];
		}
	}

	return qwSum;
}