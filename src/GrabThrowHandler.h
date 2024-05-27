#pragma once

class GrabThrowHandler :
	public ISingleton<GrabThrowHandler>
{
public:
	bool LoadSettings();
	void OnDataLoad();

	static bool  HasThrownObject(RE::bhkRigidBody* a_body);
	static float GetThrownObjectValue(RE::bhkRigidBody* a_body);
	static void  SetThrownObject(RE::hkpRigidBody* a_body, float a_value);
	static bool  ClearThrownObject(RE::bhkRigidBody* a_body, bool a_removeTelekinesisProp);

	float GetFinalDamageForImpact(float a_mass, float a_speed) const;
	float GetFinalDamageForImpact(float a_damage) const;

	void ThrowGrabbedObject(RE::PlayerCharacter* a_player, float a_strength);

private:
	float GetForce(float a_timeHeld, [[maybe_unused]] float a_avModifier) const;

	// members
	float playerGrabThrowImpulseBase{ 250.0f };
	float playerGrabThrowImpulseMax{ 1000.0f };
	float playerGrabThrowStrengthMult{ 500.0f };
	float playerGrabThrowDamageMult{ 5.0f };

	// cache gmst values
	float fPhysicsDamage1Mass{};
	float fPhysicsDamage2Mass{};
	float fPhysicsDamage3Mass{};
	float fPhysicsDamage1Damage{};
	float fPhysicsDamage2Damage{};
	float fPhysicsDamage3Damage{};
	float fPhysicsDamageSpeedMin{};
	float fPhysicsDamageSpeedMult{};
	float fPhysicsDamageSpeedBase{};

	//modifiable gmsts
	float fZKeyMaxContactDistance{ 30.0f };
	float fZKeyMaxForce{ 175.0f };
	float fZKeyMaxForceWeightHigh{ 150.0f };
	float fZKeyMaxForceWeightLow{ 0.0f };
	float fZKeyObjectDamping{ 0.75f };
	float fZKeySpringDamping{ 0.5f };
	float fZKeySpringElasticity{ 0.2f };

	static constexpr std::uint32_t HK_PROPERTY_TELEKINESIS{ 314159 };
	static constexpr std::uint32_t HK_PROPERTY_GRABTHROWNOBJECT{ 628318 };
};
