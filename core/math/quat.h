/*************************************************************************/
/*  quat.h                                                               */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2021 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2021 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

// Circular dependency between Vector3 and Basis :/
#include "core/math/vector3.h"

#ifndef QUAT_H
#define QUAT_H

#include "core/math/math_defs.h"
#include "core/math/math_funcs.h"
#include "core/string/ustring.h"

class Quat {
public:
	union {
		struct {
			real_t x;
			real_t y;
			real_t z;
			real_t w;
		};
		real_t components[4] = { 0, 0, 0, 1.0 };
	};

	_FORCE_INLINE_ real_t &operator[](int idx) {
		return components[idx];
	}
	_FORCE_INLINE_ const real_t &operator[](int idx) const {
		return components[idx];
	}
	_FORCE_INLINE_ real_t length_squared() const;
	bool is_equal_approx(const Quat &p_quat) const;
	real_t length() const;
	void normalize();
	Quat normalized() const;
	bool is_normalized() const;
	Quat inverse() const;
	_FORCE_INLINE_ real_t dot(const Quat &p_q) const;

	Vector3 get_euler_xyz() const;
	Vector3 get_euler_yxz() const;
	Vector3 get_euler() const { return get_euler_yxz(); };

	Quat slerp(const Quat &p_to, const real_t &p_weight) const;
	Quat slerpni(const Quat &p_to, const real_t &p_weight) const;
	Quat cubic_slerp(const Quat &p_q, const Quat &p_prep, const Quat &p_postq, const real_t &p_t) const;

	_FORCE_INLINE_ void get_axis_angle(Vector3 &r_axis, real_t &r_angle) const {
		r_angle = 2 * Math::acos(w);
		real_t r = ((real_t)1) / Math::sqrt(1 - w * w);
		r_axis.x = x * r;
		r_axis.y = y * r;
		r_axis.z = z * r;
	}

	// Squad (Spherical Spline Quaternions, [Shoemake 1987]) implementation for Unity by Vegard Myklebust.
	// Made available under Creative Commons license CC0. License details can be found here:
	// https://creativecommons.org/publicdomain/zero/1.0/legalcode.txt
	// https://gist.github.com/usefulslug
	// Returns a smooth approximation between the current quaternion and post using a and b as 'tangents'
	Quat squad(const Quat p_a, const Quat p_b, const Quat p_post, const float p_t) const;
	Quat log() const;
	Quat exp() const;

	// Tries to compute sensible tangent values for the quaternion
	Quat intermediate(Quat p_a, Quat p_b) const;

	// Returns a quaternion between a and b as part of a smooth squad segment
	Quat spline_segment(const Quat p_a, const Quat p_b, const Quat p_post, const float p_t) const;

	void operator*=(const Quat &q);
	Quat operator*(const Quat &q) const;

	Quat operator*(const Vector3 &v) const {
		return Quat(w * v.x + y * v.z - z * v.y,
				w * v.y + z * v.x - x * v.z,
				w * v.z + x * v.y - y * v.x,
				-x * v.x - y * v.y - z * v.z);
	}

	_FORCE_INLINE_ Vector3 xform(const Vector3 &v) const {
#ifdef MATH_CHECKS
		ERR_FAIL_COND_V_MSG(!is_normalized(), v, "The quaternion must be normalized.");
#endif
		Vector3 u(x, y, z);
		Vector3 uv = u.cross(v);
		return v + ((uv * w) + u.cross(uv)) * ((real_t)2);
	}

	_FORCE_INLINE_ Vector3 xform_inv(const Vector3 &v) const {
		return inverse().xform(v);
	}

	_FORCE_INLINE_ void operator+=(const Quat &p_q);
	_FORCE_INLINE_ void operator-=(const Quat &p_q);
	_FORCE_INLINE_ void operator*=(const real_t &s);
	_FORCE_INLINE_ void operator/=(const real_t &s);
	_FORCE_INLINE_ Quat operator+(const Quat &q2) const;
	_FORCE_INLINE_ Quat operator-(const Quat &q2) const;
	_FORCE_INLINE_ Quat operator-() const;
	_FORCE_INLINE_ Quat operator*(const real_t &s) const;
	_FORCE_INLINE_ Quat operator/(const real_t &s) const;

	_FORCE_INLINE_ bool operator==(const Quat &p_quat) const;
	_FORCE_INLINE_ bool operator!=(const Quat &p_quat) const;

	operator String() const;

	_FORCE_INLINE_ Quat() {}

	_FORCE_INLINE_ Quat(real_t p_x, real_t p_y, real_t p_z, real_t p_w) :
			x(p_x),
			y(p_y),
			z(p_z),
			w(p_w) {
	}

	Quat(const Vector3 &p_axis, real_t p_angle);

	Quat(const Vector3 &p_euler);

	Quat(const Quat &p_q) :
			x(p_q.x),
			y(p_q.y),
			z(p_q.z),
			w(p_q.w) {
	}

	Quat &operator=(const Quat &p_q) {
		x = p_q.x;
		y = p_q.y;
		z = p_q.z;
		w = p_q.w;
		return *this;
	}

	Quat(const Vector3 &v0, const Vector3 &v1) // shortest arc
	{
		Vector3 c = v0.cross(v1);
		real_t d = v0.dot(v1);

		if (d < -1.0 + CMP_EPSILON) {
			x = 0;
			y = 1;
			z = 0;
			w = 0;
		} else {
			real_t s = Math::sqrt((1.0 + d) * 2.0);
			real_t rs = 1.0 / s;

			x = c.x * rs;
			y = c.y * rs;
			z = c.z * rs;
			w = s * 0.5;
		}
	}
};

real_t Quat::dot(const Quat &p_q) const {
	return x * p_q.x + y * p_q.y + z * p_q.z + w * p_q.w;
}

real_t Quat::length_squared() const {
	return dot(*this);
}

void Quat::operator+=(const Quat &p_q) {
	x += p_q.x;
	y += p_q.y;
	z += p_q.z;
	w += p_q.w;
}

void Quat::operator-=(const Quat &p_q) {
	x -= p_q.x;
	y -= p_q.y;
	z -= p_q.z;
	w -= p_q.w;
}

void Quat::operator*=(const real_t &s) {
	x *= s;
	y *= s;
	z *= s;
	w *= s;
}

void Quat::operator/=(const real_t &s) {
	*this *= 1.0 / s;
}

Quat Quat::operator+(const Quat &q2) const {
	const Quat &q1 = *this;
	return Quat(q1.x + q2.x, q1.y + q2.y, q1.z + q2.z, q1.w + q2.w);
}

Quat Quat::operator-(const Quat &q2) const {
	const Quat &q1 = *this;
	return Quat(q1.x - q2.x, q1.y - q2.y, q1.z - q2.z, q1.w - q2.w);
}

Quat Quat::operator-() const {
	const Quat &q2 = *this;
	return Quat(-q2.x, -q2.y, -q2.z, -q2.w);
}

Quat Quat::operator*(const real_t &s) const {
	return Quat(x * s, y * s, z * s, w * s);
}

Quat Quat::operator/(const real_t &s) const {
	return *this * (1.0 / s);
}

bool Quat::operator==(const Quat &p_quat) const {
	return x == p_quat.x && y == p_quat.y && z == p_quat.z && w == p_quat.w;
}

bool Quat::operator!=(const Quat &p_quat) const {
	return x != p_quat.x || y != p_quat.y || z != p_quat.z || w != p_quat.w;
}

_FORCE_INLINE_ Quat operator*(const real_t &p_real, const Quat &p_quat) {
	return p_quat * p_real;
}

#endif // QUAT_H
