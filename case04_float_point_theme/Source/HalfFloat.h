#pragma once
#include <bit>
#include <iostream>
#include <basetsd.h>
#include <immintrin.h>

static constexpr std::uint16_t FloatToHalf(const float fVal) noexcept
{
	const std::uint32_t dwBits = std::bit_cast<std::uint32_t>(fVal);

	const std::uint32_t dwSign = (dwBits >> 31) & 0x1;
	std::int32_t nExp = ((dwBits >> 23) & 0xFF) - 127 + 15;
	std::uint32_t dwMant = dwBits & 0x7FFFFF;

	if (nExp <= 0)
	{
		if (nExp < -10)
		{
			return static_cast<std::uint16_t>(dwSign << 15);
		}

		dwMant |= 0x800000;
		return static_cast<std::uint16_t>((dwSign << 15) | (dwMant >> (14 - nExp)));
	}
	else if (nExp >= 31)
	{
		return static_cast<std::uint16_t>((dwSign << 15) | 0x7C00); // Inf/NaN
	}
	else
	{
		return static_cast<std::uint16_t>((dwSign << 15) | static_cast<std::uint32_t>(nExp << 10) | (dwMant >> 13));
	}
}

static constexpr float HalfToFloat(const std::uint16_t h) noexcept
{
	const std::uint32_t dwSign = (h >> 15) & 0x1;
	std::uint32_t nExp = (h >> 10) & 0x1F;
	std::uint32_t dwMant = h & 0x3FF;

	std::uint32_t dwBits;
	if (!nExp)
	{
		if (!dwMant)
		{
			dwBits = dwSign << 31; // +0/-0
		}
		else
		{
			//Subnormal
			nExp = 1;
			while (!(dwMant & 0x400))
			{
				dwMant <<= 1;
				--nExp;
			}

			dwMant &= 0x3FF;
			nExp = nExp - 15 + 127;
			dwBits = (dwSign << 31) | (nExp << 23) | (dwMant << 13);
		}
	}
	else if (nExp == 0x1F)
	{
		dwBits = (dwSign << 31) | 0x7F800000 | (dwMant << 13);
	}
	else
	{
		nExp = nExp - 15 + 127;
		dwBits = (dwSign << 31) | (nExp << 23) | (dwMant << 13);
	}

	return std::bit_cast<float>(dwBits);
}

//Float 16 bits -> [sign:1][exponent:5][mantissa:10]
struct alignas(2) half
{
private:
	std::uint16_t wValue;

	constexpr int ToInt() const noexcept
	{
		//Extract the bits
		const bool bSign = ((this->wValue >> 15) & 0x1) ? true : false;
		const std::int32_t nExp = ((this->wValue >> 10) & 0x1F) - 15;
		std::uint32_t dwMant = (this->wValue & 0x3FF);

		if (nExp < 0)
		{
			return 0; //Magnitude < 1
		}

		//Reconstruction of integer value without using float
		dwMant |= 0x400; //restored leading 1
		const std::int32_t nValue = static_cast<std::int32_t>(dwMant >> (10 - nExp));

		return bSign ? -nValue : nValue;
	}

public:
	constexpr half() noexcept : wValue(0) {}

	constexpr half(float f) noexcept : wValue(FloatToHalf(f)) {}

	template<typename Ty, typename = std::enable_if_t<std::is_integral_v<Ty>>>
	explicit constexpr half(Ty v) noexcept : wValue(FloatToHalf(static_cast<float>(v))) {}

	explicit constexpr operator std::int32_t() const noexcept { return ToInt(); }
	explicit constexpr operator std::uint32_t() const noexcept { return static_cast<std::uint32_t>(ToInt()); }

	explicit constexpr operator long() const noexcept { return ToInt(); }
	explicit constexpr operator unsigned long() const noexcept { return static_cast<unsigned long>(ToInt()); }

	explicit constexpr operator long long() const noexcept { return ToInt(); }
	explicit constexpr operator unsigned long long() const noexcept { return static_cast<unsigned long long>(ToInt()); }

	explicit constexpr operator std::int16_t() const noexcept { return static_cast<std::int16_t>(ToInt()); }
	explicit constexpr operator std::uint16_t() const noexcept { return static_cast<std::uint16_t>(ToInt()); }

	explicit constexpr operator std::int8_t() const noexcept { return static_cast<std::int8_t>(ToInt()); }
	explicit constexpr operator std::uint8_t() const noexcept { return static_cast<std::uint8_t>(ToInt()); }

	constexpr operator float() const noexcept { return HalfToFloat(this->wValue); }

	template<typename Ty>
	constexpr half& operator=(const Ty & h) noexcept
	{
		this->wValue = FloatToHalf(static_cast<float>(h));
		return *this;
	}

	constexpr half operator+() const noexcept { return *this; }
	constexpr half operator-() const noexcept { return half(-static_cast<float>(*this)); }

	template<typename Ty>
	constexpr half operator+(const Ty & h) const noexcept { return half(static_cast<float>(*this) + static_cast<float>(h)); }

	template<typename Ty>
	constexpr half operator-(const Ty & h) const noexcept { return half(static_cast<float>(*this) - static_cast<float>(h)); }

	template<typename Ty>
	constexpr half operator*(const Ty & h) const noexcept { return half(static_cast<float>(*this) * static_cast<float>(h)); }

	template<typename Ty>
	constexpr half operator/(const Ty & h) const noexcept { return half(static_cast<float>(*this) / static_cast<float>(h)); }

	template<typename Ty>
	constexpr half& operator+=(const Ty & h) noexcept { *this = static_cast<float>(*this) + static_cast<float>(h); return *this; }

	template<typename Ty>
	constexpr half& operator-=(const Ty & h) noexcept { *this = static_cast<float>(*this) - static_cast<float>(h); return *this; }

	template<typename Ty>
	constexpr half& operator*=(const Ty & h) noexcept { *this = static_cast<float>(*this) * static_cast<float>(h); return *this; }

	template<typename Ty>
	constexpr half& operator/=(const Ty & h) noexcept { *this = static_cast<float>(*this) / static_cast<float>(h); return *this; }

	//prefix
	constexpr half& operator++() noexcept
	{
		*this += 1.0f;
		return *this;
	}

	constexpr half& operator--() noexcept
	{
		*this -= 1.0f;
		return *this;
	}

	//postfix
	constexpr half operator++(int) noexcept
	{
		const half hTmp = *this;
		*this += 1.0f;
		return hTmp;
	}

	constexpr half operator--(int) noexcept
	{
		const half hTmp = *this;
		*this -= 1.0f;
		return hTmp;
	}

	constexpr half operator~() noexcept { return static_cast<half>(~static_cast<std::uint16_t>(this->ToInt())); }

	constexpr half operator!() noexcept { return static_cast<half>(!static_cast<std::uint16_t>(this->ToInt())); }

	template<typename Ty>
	constexpr half operator&(const Ty & h) noexcept { return static_cast<half>(static_cast<std::uint16_t>(this->ToInt()) & static_cast<std::uint16_t>(h)); }

	template<typename Ty>
	constexpr half& operator&=(const Ty & h) noexcept { *this = static_cast<half>(static_cast<std::uint16_t>(this->ToInt()) & static_cast<std::uint16_t>(h)); return *this; }

	template<typename Ty>
	constexpr half operator|(const Ty & h) noexcept { return static_cast<half>(static_cast<std::uint16_t>(this->ToInt()) | static_cast<std::uint16_t>(h)); }

	template<typename Ty>
	constexpr half& operator|=(const Ty & h) noexcept { *this = static_cast<half>(static_cast<std::uint16_t>(this->ToInt()) | static_cast<std::uint16_t>(h)); return *this; }

	template<typename Ty>
	constexpr half operator^(const Ty & h) noexcept { return static_cast<half>(static_cast<std::uint16_t>(this->ToInt()) ^ static_cast<std::uint16_t>(h)); }

	template<typename Ty>
	constexpr half& operator^=(const Ty & h) noexcept { *this = static_cast<half>(static_cast<std::uint16_t>(this->ToInt()) ^ static_cast<std::uint16_t>(h)); return *this; }

	template<typename Ty>
	constexpr bool operator<(const Ty & h) const noexcept { return static_cast<float>(*this) < static_cast<float>(h); }

	template<typename Ty>
	constexpr bool operator<=(const Ty & h) const noexcept { return static_cast<float>(*this) <= static_cast<float>(h); }

	template<typename Ty>
	constexpr bool operator>(const Ty & h) const noexcept { return static_cast<float>(*this) > static_cast<float>(h); }

	template<typename Ty>
	constexpr bool operator>=(const Ty & h) const noexcept { return static_cast<float>(*this) >= static_cast<float>(h); }

	template<typename Ty>
	constexpr bool operator==(const Ty & h) const noexcept { return static_cast<float>(*this) == static_cast<float>(h); }

	template<typename Ty>
	constexpr bool operator!=(const Ty & h) const noexcept { return static_cast<float>(*this) != static_cast<float>(h); }

	//shift left (multiply by 2^n)
	template<typename Ty>
	constexpr half operator<<(Ty shift) const noexcept { return half(static_cast<float>(*this) * static_cast<float>(1 << shift)); }

	//shift right (divide by 2^n)
	template<typename Ty>
	constexpr half operator>>(Ty shift) const noexcept { return half(static_cast<float>(*this) / static_cast<float>(1 << shift)); }

	//compound versions
	template<typename Ty>
	constexpr half& operator<<=(Ty shift) noexcept
	{
		*this = *this << shift;
		return *this;
	}

	template<typename Ty>
	constexpr half& operator>>=(Ty shift) noexcept
	{
		*this = *this >> shift;
		return *this;
	}

	//module — Integer part only
	template<typename Ty>
	constexpr half operator%(Ty divisor) const noexcept
	{
		const std::int32_t a = static_cast<std::int32_t>(*this);
		const std::int32_t b = static_cast<std::int32_t>(divisor);
		return half(static_cast<float>(a % b));
	}

	constexpr half fmod(const half & a, const half & b) noexcept
	{
		const float fa = static_cast<float>(a);
		const float fb = static_cast<float>(b);
		return half(fa - fb * static_cast<float>(static_cast<int>(fa / fb)));
	}


	constexpr std::uint16_t Bits() const noexcept { return this->wValue; }
};

static constexpr half operator""_H(long double f) { return half(static_cast<float>(f)); }
static constexpr half operator""_h(long double f) { return half(static_cast<float>(f)); }
static constexpr half operator""_H(unsigned long long f) { return half(static_cast<float>(f)); }
static constexpr half operator""_h(unsigned long long f) { return half(static_cast<float>(f)); }

#ifndef MAX_HALF
#define MAX_HALF											65504.0h
#endif

#ifndef MIN_HALF
#define MIN_HALF											-MAX_HALF
#endif

#ifndef MIN_HALF_DIVIDE
#define MIN_HALF_DIVIDE										9.765625e-4h
#endif

#ifndef MIN_NONZERO_HALF
#define MIN_NONZERO_HALF									5.960464477539063e-8h
#endif

//std:: extension --> is floating point and iostream operators
namespace std
{
	template<> struct is_floating_point<half> : std::true_type {};
	template<> inline constexpr bool is_floating_point_v<half> = true;

	inline ostream& operator<<(std::ostream& os, const half& h)
	{
		return os << static_cast<float>(h);
	}

	inline istream& operator>>(std::istream& is, half& h)
	{
		float f;
		is >> f;
		h = f;
		return is;
	}
}