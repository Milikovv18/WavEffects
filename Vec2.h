#pragma once

#include <cmath>


// Some very easy vector class, just for fire particles
class Vec2
{
public:
	// X and Y coords of the vector
	Vec2(float x = 0.0f, float y = 0.0f) :
		m_x(x), m_y(y) {}

	// Squared vector length (for performance reasons we compare squares everywhere)
	float squaredLength() const { return m_x * m_x + m_y * m_y; }
	// True length of the vector (for vector normalization we cant use square)
	float length() const { return pow(squaredLength(), 0.5f); }
	// Making vector 1 unit length
	Vec2 normalize() const
	{
		// Division by 0 check
		if (length() > 0)
			return *this * (1 / length());
		return Vec2{};
	}

	// Unary minus operator
	Vec2 operator-() const
	{
		return { -this->m_x, -this->m_y };
	}
	// Vectors addition
	Vec2 operator+(const Vec2& vec) const
	{
		return { this->m_x + vec.m_x, this->m_y + vec.m_y };
	}
	// Multiplication vector * number (order is important)
	Vec2 operator*(float num) const
	{
		return { this->m_x * num, this->m_y * num };
	}

	// Some explicit destructor
	~Vec2() {}

/*not private*/
	float m_x{ 0 }, m_y{ 0 };
};
