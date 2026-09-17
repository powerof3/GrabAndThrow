#pragma once

class GrabThrowHandler :
	public REX::TSingleton<GrabThrowHandler>,
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

	void ThrowGrabbedObject(RE::PlayerCharacter* a_player);

	RE::NiPoint3      GetThrowVelocity() const;  // game units/s
	RE::hkpRigidBody* GetGrabbedBody(RE::PlayerCharacter* a_player) const;

	void  SetChargeDuration(float a_heldDuration) { chargeDuration = a_heldDuration; }
	float GetChargeDuration() const { return chargeDuration; }
	float GetChargeFraction() const;

	void SaveSettings() const;
	void ApplyGameSettings() const;

private:
	friend class SettingsTool;

	static float    GetRealMass(RE::hkpRigidBody* a_body);
	static bool     HasThrownObject(RE::hkpRigidBody* a_body);
	void            SetThrownObject(RE::hkpRigidBody* a_body, float a_value);
	float           GetForce() const;
	RE::NiPoint3    GetAimDirection(RE::PlayerCharacter* a_player) const;
	bool            IsTrigger(RE::COL_LAYER a_colLayer);
	RE::SOUND_LEVEL GetSoundLevel(float a_mass) const;

	void ContactPointCallback(const RE::hkpContactPointEvent& a_event) override;

	// members
	REX::TIniSetting<bool> sendDetectionEvents{ "Events", "bDetectionEvent", true };
	REX::TIniSetting<bool> sendTargetHitEvents{ "Events", "bHitEventTarget", true };
	REX::TIniSetting<bool> sendThrownHitEvents{ "Events", "bHitEventThrownObject", true };

	REX::TIniSetting<bool>  destroyObjects{ "Destruction", "bEnable", true };
	REX::TIniSetting<float> minDestructibleSpeed{ "Destruction", "fDestructibleMinSpeed", 8.0f };
	REX::TIniSetting<float> destructibleSelfDamage{ "Destruction", "fDestructibleSelfDamage", 1.0f };
	REX::TIniSetting<float> destructibleTargetDamage{ "Destruction", "fDestructibleTargetDamage", 1.0f };

	REX::TIniSetting<float> playerGrabThrowImpulseBase{ "Throw", "fPlayerGrabThrowImpulseBase", 250.0f };
	REX::TIniSetting<float> playerGrabThrowImpulseMax{ "Throw", "fPlayerGrabThrowImpulseMax", 1000.0f };
	REX::TIniSetting<float> playerGrabThrowStrengthMult{ "Throw", "fPlayerGrabThrowStrengthMult", 500.0f };
	REX::TIniSetting<float> playerGrabThrowDamageMult{ "Throw", "fPlayerGrabThrowDamageMult", 5.0f };

	REX::TIniSetting<float> fZKeyMaxContactDistance{ "GameSettings", "fZKeyMaxContactDistance", 30.0f };
	REX::TIniSetting<float> fZKeyMaxForce{ "GameSettings", "fZKeyMaxForce", 175.0f };
	REX::TIniSetting<float> fZKeyMaxForceWeightHigh{ "GameSettings", "fZKeyMaxForceWeightHigh", 150.0f };
	REX::TIniSetting<float> fZKeyMaxForceWeightLow{ "GameSettings", "fZKeyMaxForceWeightLow", 0.0f };
	REX::TIniSetting<float> fZKeyObjectDamping{ "GameSettings", "fZKeyObjectDamping", 0.75f };
	REX::TIniSetting<float> fZKeySpringDamping{ "GameSettings", "fZKeySpringDamping", 0.5f };
	REX::TIniSetting<float> fZKeySpringElasticity{ "GameSettings", "fZKeySpringElasticity", 0.2f };
	REX::TIniSetting<float> fZKeyHeavyWeight{ "GameSettings", "fZKeyHeavyWeight", 100.0f };
	REX::TIniSetting<float> fZKeyComplexHelperWeightMax{ "GameSettings", "fZKeyComplexHelperWeightMax", 100.0f };

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

	float chargeDuration{ 0.0f };

	static constexpr auto path = R"(Data\SKSE\Plugins\po3_GrabAndThrow.ini)"sv;
};
