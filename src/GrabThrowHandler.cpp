#include "GrabThrowHandler.h"

bool GrabThrowHandler::HasThrownObject(RE::bhkRigidBody* a_body)
{
	auto hkRigidBody = a_body ? a_body->GetRigidBody() : nullptr;
	return hkRigidBody && hkRigidBody->HasProperty(HK_PROPERTY_GRABTHROWNOBJECT);
}

float GrabThrowHandler::GetThrownObjectValue(RE::bhkRigidBody* a_body)
{
	if (auto hkRigidBody = a_body ? a_body->GetRigidBody() : nullptr) {
		if (auto property = hkRigidBody->GetProperty(HK_PROPERTY_GRABTHROWNOBJECT)) {
			return property->value.GetFloat() + 1.0f;
		}
	}

	return 1.0f;
}

void GrabThrowHandler::SetThrownObject(RE::hkpRigidBody* a_body, float a_value)
{
	if (!a_body->HasProperty(HK_PROPERTY_GRABTHROWNOBJECT)) {
		a_body->SetProperty(HK_PROPERTY_GRABTHROWNOBJECT, a_value);
		a_body->SetProperty(HK_PROPERTY_TELEKINESIS, a_value);
		a_body->AddContactListener(RE::bhkTelekinesisListener::GetSingleton());
	}
}

bool GrabThrowHandler::ClearThrownObject(RE::bhkRigidBody* a_body, bool a_removeTelekinesisProp)
{
	auto hkRigidBody = a_body ? a_body->GetRigidBody() : nullptr;

	if (hkRigidBody && hkRigidBody->HasProperty(HK_PROPERTY_GRABTHROWNOBJECT)) {
		hkRigidBody->RemoveProperty(HK_PROPERTY_GRABTHROWNOBJECT);
		if (a_removeTelekinesisProp) {
			hkRigidBody->RemoveProperty(HK_PROPERTY_TELEKINESIS);
			hkRigidBody->RemoveContactListener(RE::bhkTelekinesisListener::GetSingleton());
		}
		return true;
	}

	return false;
}

bool GrabThrowHandler::LoadSettings()
{
	const auto path = std::format("Data/SKSE/Plugins/{}.ini", Version::PROJECT);

	CSimpleIniA ini;
	ini.SetUnicode();

	ini.LoadFile(path.c_str());

	ini::get_value(ini, playerGrabThrowImpulseBase, "Settings", "fPlayerGrabThrowImpulseBase", nullptr);
	ini::get_value(ini, playerGrabThrowImpulseMax, "Settings", "fPlayerGrabThrowImpulseMax", nullptr);
	ini::get_value(ini, playerGrabThrowStrengthMult, "Settings", "fPlayerGrabThrowStrengthMult", nullptr);
	ini::get_value(ini, playerGrabThrowDamageMult, "Settings", "fPlayerGrabThrowDamageMult", nullptr);

	ini::get_value(ini, fZKeyMaxContactDistance, "GameSettings", "fZKeyMaxContactDistance", nullptr);
	ini::get_value(ini, fZKeyMaxForce, "GameSettings", "fZKeyMaxForce", nullptr);
	ini::get_value(ini, fZKeyMaxForceWeightHigh, "GameSettings", "fZKeyMaxForceWeightHigh", nullptr);
	ini::get_value(ini, fZKeyMaxForceWeightLow, "GameSettings", "fZKeyMaxForceWeightLow", nullptr);
	ini::get_value(ini, fZKeyObjectDamping, "GameSettings", "fZKeyObjectDamping", nullptr);
	ini::get_value(ini, fZKeySpringDamping, "GameSettings", "fZKeySpringDamping", nullptr);
	ini::get_value(ini, fZKeySpringElasticity, "GameSettings", "fZKeySpringElasticity", nullptr);

	(void)ini.SaveFile(path.c_str());

	return true;
}

void GrabThrowHandler::OnDataLoad()
{
	constexpr auto gmst = [](const char* setting) {
		return RE::GameSettingCollection::GetSingleton()->GetSetting(setting)->GetFloat();
	};

	constexpr auto set_gmst = [](const char* setting, float a_value) {
		RE::GameSettingCollection::GetSingleton()->GetSetting(setting)->data.f = a_value;
	};

	fPhysicsDamage1Mass = gmst("fPhysicsDamage1Mass");
	fPhysicsDamage2Mass = gmst("fPhysicsDamage2Mass");
	fPhysicsDamage3Mass = gmst("fPhysicsDamage3Mass");
	fPhysicsDamage1Damage = gmst("fPhysicsDamage1Damage");
	fPhysicsDamage2Damage = gmst("fPhysicsDamage2Damage");
	fPhysicsDamage3Damage = gmst("fPhysicsDamage3Damage");
	fPhysicsDamageSpeedMin = gmst("fPhysicsDamageSpeedMin");
	fPhysicsDamageSpeedMult = gmst("fPhysicsDamageSpeedMult");
	fPhysicsDamageSpeedBase = gmst("fPhysicsDamageSpeedBase");

	set_gmst("fZKeyMaxForce", fZKeyMaxForce);
	set_gmst("fZKeyMaxForceWeightHigh", fZKeyMaxForceWeightHigh);
	set_gmst("fZKeyMaxForceWeightLow", fZKeyMaxForceWeightLow);
	set_gmst("fZKeyMaxContactDistance", fZKeyMaxContactDistance);  // max distance between camera and object before mouse spring breaks
	set_gmst("fZKeyObjectDamping", fZKeyObjectDamping);
	set_gmst("fZKeySpringDamping", fZKeySpringDamping);
	set_gmst("fZKeySpringElasticity", fZKeySpringElasticity);
}

float GrabThrowHandler::GetForce(float a_timeHeld, [[maybe_unused]] float a_avModifier) const
{
	auto force = (a_timeHeld * playerGrabThrowStrengthMult);
	if (force > playerGrabThrowImpulseMax) {
		force = playerGrabThrowImpulseMax;
	}
	return force + playerGrabThrowImpulseBase;
}

float GrabThrowHandler::GetFinalDamageForImpact(float a_mass, float a_speed) const
{
	float force = 0.0f;
	float speed = 0.0f;

	if (a_mass >= fPhysicsDamage1Mass) {
		if (a_mass >= fPhysicsDamage2Mass) {
			if (a_mass >= fPhysicsDamage3Mass) {
				force = fPhysicsDamage3Damage;
			} else {
				force = fPhysicsDamage2Damage;
			}
		} else {
			force = fPhysicsDamage1Damage;
		}
	}

	if (a_speed > fPhysicsDamageSpeedMin) {
		speed = (fPhysicsDamageSpeedMult * a_speed) + fPhysicsDamageSpeedBase;
	}

	return (force * speed) * playerGrabThrowDamageMult;
}

float GrabThrowHandler::GetFinalDamageForImpact(float a_damage) const
{
	return a_damage * playerGrabThrowDamageMult;
}

void GrabThrowHandler::ThrowGrabbedObject(RE::PlayerCharacter* a_player, float a_strength)
{
	auto cell = a_player->GetParentCell();
	auto bhkWorld = cell ? cell->GetbhkWorld() : nullptr;

	if (!bhkWorld) {
		return;
	}

	RE::BSWriteLockGuard locker(bhkWorld->worldLock);

	float force = GetForce(a_strength, a_player->GetActorValue(RE::ActorValue::kOneHanded));

	for (const auto& mouseSpring : a_player->grabSpring) {
		if (mouseSpring && mouseSpring->referencedObject) {
			if (auto hkMouseSpring = skyrim_cast<RE::hkpMouseSpringAction*>(mouseSpring->referencedObject.get())) {
				if (auto hkpRigidBody = reinterpret_cast<RE::hkpRigidBody*>(hkMouseSpring->entity)) {
					auto bhkRigidBody = reinterpret_cast<RE::bhkRigidBody*>(hkpRigidBody->userData);

					SetThrownObject(hkpRigidBody, a_strength);

					auto mass = hkpRigidBody->motion.GetMass();
					if (mass < fPhysicsDamage1Mass) {
						RE::TESHavokUtilities::PushTemporaryMass(bhkRigidBody, fPhysicsDamage1Mass);
						mass = fPhysicsDamage1Mass;
					}

					RE::NiMatrix3 matrix = RE::PlayerCamera::GetSingleton()->cameraRoot->world.rotate;
					float         x = (matrix.entry[0][1] * force) * 0.0142875f;
					float         y = (matrix.entry[1][1] * force) * 0.0142875f;
					float         z = (matrix.entry[2][1] * force) * 0.0142875f;

					RE::hkVector4 impulse(x, y, z, 0);
					impulse = impulse * mass;

					hkpRigidBody->SetLinearVelocity(RE::hkVector4());
					hkpRigidBody->SetAngularVelocity(RE::hkVector4());
					hkpRigidBody->ApplyLinearImpulse(impulse);
				}
			}
		}
	}
}
