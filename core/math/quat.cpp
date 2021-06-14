/*************************************************************************/
/*  quat.cpp                                                             */
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

#include "quat.h"

#include "core/math/basis.h"
#include "core/print_string.h"

// set_euler_xyz expects a vector containing the Euler angles in the format
// (ax,ay,az), where ax is the angle of rotation around x axis,
// and similar for other axes.
// This implementation uses XYZ convention (Z is the first rotation).
void Quat::set_euler_xyz(const Vector3 &p_euler) {
	real_t half_a1 = p_euler.x * 0.5;
	real_t half_a2 = p_euler.y * 0.5;
	real_t half_a3 = p_euler.z * 0.5;

	// R = X(a1).Y(a2).Z(a3) convention for Euler angles.
	// Conversion to quaternion as listed in https://ntrs.nasa.gov/archive/nasa/casi.ntrs.nasa.gov/19770024290.pdf (page A-2)
	// a3 is the angle of the first rotation, following the notation in this reference.

	real_t cos_a1 = Math::cos(half_a1);
	real_t sin_a1 = Math::sin(half_a1);
	real_t cos_a2 = Math::cos(half_a2);
	real_t sin_a2 = Math::sin(half_a2);
	real_t cos_a3 = Math::cos(half_a3);
	real_t sin_a3 = Math::sin(half_a3);

	set(sin_a1 * cos_a2 * cos_a3 + sin_a2 * sin_a3 * cos_a1,
			-sin_a1 * sin_a3 * cos_a2 + sin_a2 * cos_a1 * cos_a3,
			sin_a1 * sin_a2 * cos_a3 + sin_a3 * cos_a1 * cos_a2,
			-sin_a1 * sin_a2 * sin_a3 + cos_a1 * cos_a2 * cos_a3);
}

// get_euler_xyz returns a vector containing the Euler angles in the format
// (ax,ay,az), where ax is the angle of rotation around x axis,
// and similar for other axes.
// This implementation uses XYZ convention (Z is the first rotation).
Vector3 Quat::get_euler_xyz() const {
	Basis m(*this);
	return m.get_euler_xyz();
}

// set_euler_yxz expects a vector containing the Euler angles in the format
// (ax,ay,az), where ax is the angle of rotation around x axis,
// and similar for other axes.
// This implementation uses YXZ convention (Z is the first rotation).
void Quat::set_euler_yxz(const Vector3 &p_euler) {
	real_t half_a1 = p_euler.y * 0.5;
	real_t half_a2 = p_euler.x * 0.5;
	real_t half_a3 = p_euler.z * 0.5;

	// R = Y(a1).X(a2).Z(a3) convention for Euler angles.
	// Conversion to quaternion as listed in https://ntrs.nasa.gov/archive/nasa/casi.ntrs.nasa.gov/19770024290.pdf (page A-6)
	// a3 is the angle of the first rotation, following the notation in this reference.

	real_t cos_a1 = Math::cos(half_a1);
	real_t sin_a1 = Math::sin(half_a1);
	real_t cos_a2 = Math::cos(half_a2);
	real_t sin_a2 = Math::sin(half_a2);
	real_t cos_a3 = Math::cos(half_a3);
	real_t sin_a3 = Math::sin(half_a3);

	set(sin_a1 * cos_a2 * sin_a3 + cos_a1 * sin_a2 * cos_a3,
			sin_a1 * cos_a2 * cos_a3 - cos_a1 * sin_a2 * sin_a3,
			-sin_a1 * sin_a2 * cos_a3 + cos_a1 * cos_a2 * sin_a3,
			sin_a1 * sin_a2 * sin_a3 + cos_a1 * cos_a2 * cos_a3);
}

// get_euler_yxz returns a vector containing the Euler angles in the format
// (ax,ay,az), where ax is the angle of rotation around x axis,
// and similar for other axes.
// This implementation uses YXZ convention (Z is the first rotation).
Vector3 Quat::get_euler_yxz() const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(!is_normalized(), Vector3(0, 0, 0), "The quaternion must be normalized.");
#endif
	Basis m(*this);
	return m.get_euler_yxz();
}

void Quat::operator*=(const Quat &p_q) {

	set(w * p_q.x + x * p_q.w + y * p_q.z - z * p_q.y,
			w * p_q.y + y * p_q.w + z * p_q.x - x * p_q.z,
			w * p_q.z + z * p_q.w + x * p_q.y - y * p_q.x,
			w * p_q.w - x * p_q.x - y * p_q.y - z * p_q.z);
}

Quat Quat::operator*(const Quat &p_q) const {

	Quat r = *this;
	r *= p_q;
	return r;
}

bool Quat::is_equal_approx(const Quat &p_quat) const {

	return Math::is_equal_approx(x, p_quat.x) && Math::is_equal_approx(y, p_quat.y) && Math::is_equal_approx(z, p_quat.z) && Math::is_equal_approx(w, p_quat.w);
}

real_t Quat::length() const {

	return Math::sqrt(length_squared());
}

void Quat::normalize() {
	*this /= length();
}

Quat Quat::normalized() const {
	return *this / length();
}

bool Quat::is_normalized() const {
	return Math::is_equal_approx(length_squared(), 1.0, UNIT_EPSILON); //use less epsilon
}

Quat Quat::inverse() const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(!is_normalized(), Quat(), "The quaternion must be normalized.");
#endif
	return Quat(-x, -y, -z, w);
}

Quat Quat::slerp(const Quat &p_to, const real_t &p_weight) const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(!is_normalized(), Quat(), "The start quaternion must be normalized.");
	ERR_FAIL_COND_V_MSG(!p_to.is_normalized(), Quat(), "The end quaternion must be normalized.");
#endif
	Quat to1;
	real_t omega, cosom, sinom, scale0, scale1;

	// calc cosine
	cosom = dot(p_to);

	// adjust signs (if necessary)
	if (cosom < 0.0) {
		cosom = -cosom;
		to1.x = -p_to.x;
		to1.y = -p_to.y;
		to1.z = -p_to.z;
		to1.w = -p_to.w;
	} else {
		to1.x = p_to.x;
		to1.y = p_to.y;
		to1.z = p_to.z;
		to1.w = p_to.w;
	}

	// calculate coefficients

	if ((1.0 - cosom) > CMP_EPSILON) {
		// standard case (slerp)
		omega = Math::acos(cosom);
		sinom = Math::sin(omega);
		scale0 = Math::sin((1.0 - p_weight) * omega) / sinom;
		scale1 = Math::sin(p_weight * omega) / sinom;
	} else {
		// "from" and "to" quaternions are very close
		//  ... so we can do a linear interpolation
		scale0 = 1.0 - p_weight;
		scale1 = p_weight;
	}
	// calculate final values
	return Quat(
			scale0 * x + scale1 * to1.x,
			scale0 * y + scale1 * to1.y,
			scale0 * z + scale1 * to1.z,
			scale0 * w + scale1 * to1.w);
}

Quat Quat::slerpni(const Quat &p_to, const real_t &p_weight) const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(!is_normalized(), Quat(), "The start quaternion must be normalized.");
	ERR_FAIL_COND_V_MSG(!p_to.is_normalized(), Quat(), "The end quaternion must be normalized.");
#endif
	const Quat &from = *this;

	real_t dot = from.dot(p_to);

	if (Math::absf(dot) > 0.9999) return from;

	real_t theta = Math::acos(dot),
		   sinT = 1.0 / Math::sin(theta),
		   newFactor = Math::sin(p_weight * theta) * sinT,
		   invFactor = Math::sin((1.0 - p_weight) * theta) * sinT;

	return Quat(invFactor * from.x + newFactor * p_to.x,
			invFactor * from.y + newFactor * p_to.y,
			invFactor * from.z + newFactor * p_to.z,
			invFactor * from.w + newFactor * p_to.w);
}

Quat Quat::cubic_slerp(const Quat &p_q, const Quat &p_prep, const Quat &p_postq, const real_t &p_t) const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(!is_normalized(), Quat(), "The start quaternion must be normalized.");
	ERR_FAIL_COND_V_MSG(!p_q.is_normalized(), Quat(), "The end quaternion must be normalized.");
#endif
	// Modify quaternions for shortest path
	// https://math.stackexchange.com/questions/2650188/super-confused-by-squad-algorithm-for-quaternion-interpolation
	const Quat q_a = *this;
	Quat prep = (q_a - p_prep).length_squared() < (q_a + p_prep).length_squared() ? p_prep : p_prep * -1.0f;
	Quat q_b = (q_a - p_q).length_squared() < (q_a + p_q).length_squared() ? p_q : p_q * -1.0f;
	Quat postq = (p_q - p_postq).length_squared() < (p_q + p_postq).length_squared() ? p_postq : p_postq * -1.0f;

	return prep.spline_segment(q_a, q_b, postq, p_t);
}

Quat Quat::squad(const Quat p_a, const Quat p_b, const Quat p_post, const float p_t) const {
	Quat pre = *this;
	float slerp_t = 2.0 * p_t * (1.0 - p_t);
	Quat slerp_1 = pre.slerpni(p_post, p_t);
	Quat slerp_2 = p_a.slerpni(p_b, p_t);
	return slerp_1.slerpni(slerp_2, slerp_t);
}

Quat Quat::log() const {
	// http://www.cs.jhu.edu/~misha/Fall20/29.pdf Exponential map quat are guaranteed to be rotations
	Vector3 v_norm = Vector3(x, y, z).normalized();
	float q_norm = w;
	if (Math::is_zero_approx(q_norm)) {
		// Undefined
		Vector3 vec = Vector3(NAN, NAN, NAN) * Vector3(x, y, z);
		return Quat(vec.x, vec.y, vec.z, -INFINITY);
	} else {
		// real quaternions - no imaginary part
		return Quat(0.0f, 0.0f, 0.0f, Math::log(q_norm));
	}
	Vector3 vec = Vector3(x, y, z) / v_norm;
	vec = acos(w / q_norm) * vec;
	return Quat(vec.x, vec.y, vec.z, Math::log(q_norm));
}

Quat Quat::log_map(Quat p_p) const {
	// A tangent vector having the length and direction given by the geodesic joining this quat and eta.
	// http://www.cs.jhu.edu/~misha/Fall20/29.pdf Exponential map quat are guaranteed to be rotations
	// https://math.stackexchange.com/questions/2552/the-logarithm-of-quaternion
	// https://github.com/KieranWynn/pyquaternion
	return ((*this).inverse() * p_p).log();
}

Quat Quat::exp_map(Quat p_eta) const {
	return (*this).exp(p_eta);
}

Quat Quat::exp(Quat p_eta) const {
	Vector3 vec = Vector3(x, y, z);
	real_t v_norm = vec.length();
	if (!Math::is_zero_approx(v_norm)) {
		vec = vec / v_norm;
	}
	real_t magnitude = Math::exp(w);
	vec = magnitude * sin(v_norm) * vec;
	return Quat(vec.x, vec.y, vec.z, magnitude * cos(v_norm));
}

Quat Quat::intermediate(Quat p_a, Quat p_b) const {
	Quat a_inv = p_a.inverse();
	Quat c_1 = a_inv * p_b;
	Quat c_2 = a_inv * (*this);
	c_1 = c_1.log();
	c_2 = c_2.log();
	Quat c_3 = c_2 + c_1;
	c_3 = c_3 * -0.25f;
	c_3 = c_3.exp_map();
	Quat r = p_a * c_3;
	return r.normalized();
}

Quat Quat::spline_segment(const Quat p_a, const Quat p_b, const Quat p_post, const float p_t) const {
	Quat pre = *this;
	Quat q_a = pre.intermediate(p_a, p_b);
	Quat q_b = p_a.intermediate(p_b, p_post);
	return p_a.squad(q_a, q_b, p_b, p_t);
}

Quat::operator String() const {

	return String::num(x) + ", " + String::num(y) + ", " + String::num(z) + ", " + String::num(w);
}

void Quat::set_axis_angle(const Vector3 &axis, const real_t &angle) {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_MSG(!axis.is_normalized(), "The axis Vector3 must be normalized.");
#endif
	real_t d = axis.length();
	if (d == 0)
		set(0, 0, 0, 0);
	else {
		real_t sin_angle = Math::sin(angle * 0.5);
		real_t cos_angle = Math::cos(angle * 0.5);
		real_t s = sin_angle / d;
		set(axis.x * s, axis.y * s, axis.z * s,
				cos_angle);
	}
}
