#pragma once

namespace RE
{
	inline constexpr std::uint32_t HK_PROPERTY_TELEKINESIS{ 314159 };
	inline constexpr std::uint32_t HK_PROPERTY_TEMPORARYMASS{ 314160 };
	inline constexpr std::uint32_t HK_PROPERTY_GRABTHROWNOBJECT{ 628318 };
	inline constexpr float         BS_TO_HK_SCALE{ 0.0142875f };
	inline constexpr float         HK_TO_BS_SCALE{ 69.991251f };
	inline constexpr float         OBJECT_LINEAR_DAMPING{ 0.099609375f };

	inline NiPoint3 HKToGame(const hkVector4& a_vec)
	{
		return NiPoint3(
				   a_vec.quad.m128_f32[0],
				   a_vec.quad.m128_f32[1],
				   a_vec.quad.m128_f32[2]) *
		       HK_TO_BS_SCALE;
	}

	inline NiPoint3 GetGravity(bhkWorld* a_world)
	{
		NiPoint3 gravity{ 0.0f, 0.0f, -9.81f * HK_TO_BS_SCALE };
		if (const auto hkpWorld = a_world ? a_world->GetWorld1() : nullptr) {
			return HKToGame(hkpWorld->gravity);
		}
		return gravity;
	}
}
