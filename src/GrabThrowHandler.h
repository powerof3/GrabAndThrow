#pragma once

class GrabThrowHandler :
	public REX::Singleton<GrabThrowHandler>,
	public RE::hkpContactListener
{
public:
	bool LoadSettings();
	void OnDataLoad();

	static bool  HasThrownObject(RE::bhkRigidBody* a_body);
	static float GetThrownObjectValue(RE::bhkRigidBody* a_body);
	bool         ClearThrownObject(RE::bhkRigidBody* a_body);

	float GetFinalDamageForImpact(float a_mass, float a_speed) const;
	float GetFinalDamageForImpact(float a_damage) const;

	void ThrowGrabbedObject(RE::PlayerCharacter* a_player, float a_heldDuration);

private:
	static float    GetRealMass(RE::hkpRigidBody* a_body);
	static bool     HasThrownObject(RE::hkpRigidBody* a_body);
	void            SetThrownObject(RE::hkpRigidBody* a_body, float a_value);
	float           GetForce(float a_timeHeld, [[maybe_unused]] float a_avModifier) const;
	RE::hkVector4   GetImpulse(RE::PlayerCharacter* a_player, float a_force, float a_mass) const;
	bool            IsTrigger(RE::COL_LAYER a_colLayer);
	RE::SOUND_LEVEL GetSoundLevel(float a_mass) const;

	void ContactPointCallback(const RE::hkpContactPointEvent& a_event) override;

	// members
	bool sendDetectionEvents{ true };
	bool sendTargetHitEvents{ true };
	bool sendThrownHitEvents{ true };

	bool  destroyObjects{ true };
	float minDestructibleSpeed{ 8.0f };
	float destructibleSelfDamage{ 1.0f };
	float destructibleTargetDamage{ 1.0f };

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
	float fZKeyHeavyWeight{ 100.0f };
	float fZKeyComplexHelperWeightMax{ 100.0f };

	static constexpr std::uint32_t HK_PROPERTY_TELEKINESIS{ 314159 };
	static constexpr std::uint32_t HK_PROPERTY_TEMPORARYMASS{ 314160 };
	static constexpr std::uint32_t HK_PROPERTY_GRABTHROWNOBJECT{ 628318 };
	static constexpr float         BS_TO_HK_SCALE{ 0.0142875f };
	static constexpr float         HK_TO_BS_SCALE{ 69.991251f };
};
