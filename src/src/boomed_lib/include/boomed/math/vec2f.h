#ifndef BOOMED_MATH_VEC2F_H_
#define BOOMED_MATH_VEC2F_H_

#include <assert.h>
#include <stdbool.h>
#include <math.h>


typedef struct vec2f_t vec2f_t;

struct vec2f_t {
    float x, y;
};


static inline vec2f_t vec2f_make_zero(void) {
    return (vec2f_t){
        0.0f, 
        0.0f
    };
}

static inline vec2f_t vec2f_make_unitx(void) {
    return (vec2f_t){
        1.0f,
        0.0f
    };
}

static inline vec2f_t vec2f_make_unity(void) {
    return (vec2f_t){
        0.0f,
        1.0f
    };
}

static inline vec2f_t vec2f_make_sincos(float angle) {
    return (vec2f_t){
        sinf(angle),
        cosf(angle)
    };
}

static inline vec2f_t vec2f_make_cossin(float angle) {
    return (vec2f_t){
        cosf(angle),
        sinf(angle)
    };
}

static inline vec2f_t vec2f_make_from_floats(const float *f) {
	return (vec2f_t){
        f[0],
        f[1]
    };
}

static inline vec2f_t vec2f_add(vec2f_t a, vec2f_t b) {
    return (vec2f_t){
        a.x + b.x,
        a.y + b.y
    };
}

static inline vec2f_t vec2f_add3(vec2f_t a, vec2f_t b, vec2f_t c) {
	return (vec2f_t){
		a.x + b.x + c.x,
		a.y + b.y + c.y
	};
}

static inline vec2f_t vec2f_add4(vec2f_t a, vec2f_t b, vec2f_t c, vec2f_t d) {
	return (vec2f_t){
		a.x + b.x + c.x + d.x,
		a.y + b.y + c.y + d.y
	};
}

static inline vec2f_t vec2f_sub(vec2f_t a, vec2f_t b) {
    return (vec2f_t){
        a.x - b.x,
        a.y - b.y
    };
}

static inline vec2f_t vec2f_scalar_mul(vec2f_t a, float b) {
    return (vec2f_t){
        a.x * b,
        a.y * b
    };
}

static inline vec2f_t vec2f_component_mul(vec2f_t a, vec2f_t b) {
	return (vec2f_t){
		a.x * b.x,
		a.y * b.y
	};
}

static inline vec2f_t vec2f_component_min(vec2f_t a, vec2f_t b) {
	return (vec2f_t){
		fminf(a.x, b.x),
		fminf(a.y, b.y)
	};
}

static inline vec2f_t vec2f_component_max(vec2f_t a, vec2f_t b) {
	return (vec2f_t){
		fmaxf(a.x, b.x),
		fmaxf(a.y, b.y)
	};
}

static inline vec2f_t vec2f_lerp(vec2f_t a, vec2f_t b, float t) {
	return (vec2f_t){
		a.x + (b.x - a.x) * t,
		a.y + (b.y - a.y) * t
	};
}

static inline float vec2f_dot(vec2f_t a, vec2f_t b) {
    return a.x * b.x + a.y * b.y;
}

static inline float vec2f_wedge(vec2f_t a, vec2f_t b) {
    return a.x * b.y - b.x * a.y;
}

static inline vec2f_t vec2f_perp(vec2f_t a) {
    return (vec2f_t){
        -a.y,
        a.x
    };
}

static inline float vec2f_lengthsqr(vec2f_t a) {
    return vec2f_dot(a, a);
}

static inline float vec2f_length(vec2f_t a) {
    return sqrtf(vec2f_lengthsqr(a));
}

static inline vec2f_t vec2f_normalize(vec2f_t a) {
    float len = vec2f_length(a);
    assert(len != 0.0f);
    return vec2f_scalar_mul(a, 1.0f / len);
}

static inline vec2f_t vec2f_normalize_safe(vec2f_t a, float tolerance) {
	float len = vec2f_length(a);
	return (len >= tolerance) ? vec2f_scalar_mul(a, 1.0f / len) : vec2f_make_zero();
}

static inline vec2f_t vec2f_negate(vec2f_t a) {
	return (vec2f_t){
        -a.x,
        -a.y
    };
}

static inline bool vec2f_is_nearly_equal(vec2f_t a, vec2f_t b, float tolerance) {
	return vec2f_lengthsqr(vec2f_sub(a, b)) < tolerance * tolerance;
}

static inline const float *vec2f_as_floats(const vec2f_t *a) {
    return &a->x;
}


#endif // ifndef BOOMED_MATH_VEC2F_H_
