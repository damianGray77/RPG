#pragma once
#ifndef TWEEN_H
#define TWEEN_H

enum EaseType : uint8 {
	EASE_LINEAR,
	EASE_IN,       // quadratic: t*t
	EASE_OUT,      // quadratic: t*(2-t)
	EASE_IN_OUT    // quadratic: t<0.5 ? 2*t*t : -1+(4-2*t)*t
};

struct Tween {
	float start;
	float end;
	float duration;    // seconds
	float elapsed;     // seconds
	EaseType ease;
	bool active;

	void begin(float from, float to, float dur, EaseType e = EASE_LINEAR) {
		start    = from;
		end      = to;
		duration = dur;
		elapsed  = 0.0f;
		ease     = e;
		active   = true;
	}

	// update target while tween is in progress (entity moved)
	void retarget(float new_end) {
		start = value();
		end   = new_end;
	}

	float value() const {
		if (!active || duration <= 0.0f) { return end; }
		float t = elapsed / duration;
		if (t >= 1.0f) { t = 1.0f; }
		float e = apply_ease(t);
		return start + (end - start) * e;
	}

	bool update(float delta) {
		if (!active) { return false; }
		elapsed += delta;
		if (elapsed >= duration) {
			active = false;
			return false;
		}
		return true;
	}

	bool done() const { return !active; }

	static float apply_ease(float t, EaseType type) {
		switch (type) {
			case EASE_IN:     return t * t;
			case EASE_OUT:    return t * (2.0f - t);
			case EASE_IN_OUT: return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
			default:          return t;
		}
	}

	float apply_ease(float t) const { return apply_ease(t, ease); }
};

#endif
