#pragma once

namespace ColourUtil {
	Vector4 Interpolate(const Vector4 & v1, const Vector4 & v2, float i) {
		return v1 + (v2 - v1) * i;
	}
}