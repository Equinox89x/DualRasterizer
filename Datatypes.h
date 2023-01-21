#pragma once
#include "Math.h"
#include "vector"

namespace dae
{
	struct Vertex
	{
		Vector3 position{};
		ColorRGB color{ colors::White };
		Vector2 uv{};
		Vector3 normal{};
		Vector3 tangent{};
		Vector3 viewDirection{};
	};

	struct Vertex_Out
	{
		Vector4 position{};
		ColorRGB color{ colors::White };
		Vector2 uv{};
		Vector3 normal{};
		Vector3 tangent{};
		Vector3 viewDirection{};
	};

	enum class LightingMode {
		ObservedArea,
		Diffuse,
		Specular,
		Combined
	};

	enum class CullMode {
		None,
		Front,
		Back
	};

	enum class SampleMode {
		Point,
		Linear,
		Anisotropic
	};
}