#include "GrabThrowHandler.h"

bool GrabThrowHandler::LoadSettings()
{
	const auto path = std::format("Data/SKSE/Plugins/{}.ini", Version::PROJECT);

	CSimpleIniA ini;
	ini.SetUnicode();

	ini.LoadFile(path.c_str());

	ini::get_value(ini, sendDetectionEvents, "Settings", "bThrownObjectDetectionEnable", nullptr);
	
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
	ini::get_value(ini, fZKeyHeavyWeight, "GameSettings", "fZKeyHeavyWeight", nullptr);
	ini::get_value(ini, fZKeyComplexHelperWeightMax, "GameSettings", "fZKeyComplexHelperWeightMax", nullptr);

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
	set_gmst("fZKeyHeavyWeight", fZKeyHeavyWeight);
	set_gmst("fZKeyComplexHelperWeightMax", fZKeyComplexHelperWeightMax);
}

bool GrabThrowHandler::HasThrownObject(RE::hkpRigidBody* a_body)
{
	return a_body && a_body->HasProperty(HK_PROPERTY_GRABTHROWNOBJECT);
}

void GrabThrowHandler::SetThrownObject(RE::hkpRigidBody* a_body, float a_value)
{
	if (!a_body->HasProperty(HK_PROPERTY_GRABTHROWNOBJECT)) {
		a_body->SetProperty(HK_PROPERTY_GRABTHROWNOBJECT, a_value);
		a_body->AddContactListener(this);
	}
}

bool GrabThrowHandler::HasThrownObject(RE::bhkRigidBody* a_body)
{
	return HasThrownObject(a_body ? a_body->GetRigidBody() : nullptr);
}

float GrabThrowHandler::GetThrownObjectValue(RE::bhkRigidBody* a_body)
{
	auto hkpBody = a_body ? a_body->GetRigidBody() : nullptr;

	if (hkpBody) {
		if (auto property = hkpBody->GetProperty(HK_PROPERTY_GRABTHROWNOBJECT)) {
			return property->value.GetFloat() + 1.0f;
		}
	}

	return 1.0f;
}

bool GrabThrowHandler::ClearThrownObject(RE::bhkRigidBody* a_body)
{
	auto hkpBody = a_body ? a_body->GetRigidBody() : nullptr;

	if (hkpBody && hkpBody->HasProperty(HK_PROPERTY_GRABTHROWNOBJECT)) {
		hkpBody->RemoveProperty(HK_PROPERTY_GRABTHROWNOBJECT);
		hkpBody->RemoveContactListener(this);
		return true;
	}

	return false;
}

float GrabThrowHandler::GetForce(float a_timeHeld, [[maybe_unused]] float a_avModifier) const
{
	auto force = (a_timeHeld * playerGrabThrowStrengthMult);
	if (force > playerGrabThrowImpulseMax) {
		force = playerGrabThrowImpulseMax;
	}
	return force + playerGrabThrowImpulseBase;
}

bool GrabThrowHandler::IsTrigger(RE::COL_LAYER a_colLayer)
{
	switch (a_colLayer) {
	case RE::COL_LAYER::kUnidentified:
	case RE::COL_LAYER::kTrigger:
	case RE::COL_LAYER::kCloudTrap:
	case RE::COL_LAYER::kActorZone:
	case RE::COL_LAYER::kProjectileZone:
	case RE::COL_LAYER::kGasTrap:
	case RE::COL_LAYER::kSpellExplosion:
		return true;
	default:
		return false;
	}
}

RE::SOUND_LEVEL GrabThrowHandler::GetSoundLevel(float a_mass) const
{
	if (a_mass >= fPhysicsDamage1Mass) {
		if (a_mass >= fPhysicsDamage2Mass) {
			if (a_mass >= fPhysicsDamage3Mass) {
				return RE::SOUND_LEVEL::kVeryLoud;
			} else {
				return RE::SOUND_LEVEL::kLoud;
			}
		} else {
			return RE::SOUND_LEVEL::kNormal;
		}
	} else {
		return RE::SOUND_LEVEL::kQuiet;
	}
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

	return GetFinalDamageForImpact(force * speed);
}

float GrabThrowHandler::GetFinalDamageForImpact(float a_damage) const
{
	return a_damage * playerGrabThrowDamageMult;
}

void GrabThrowHandler::ContactPointCallback(const RE::hkpContactPointEvent& a_event)
{
	if (a_event.contactPointProperties->flags.any(RE::hkContactPointMaterial::Flag::kIsDisabled) || !a_event.contactPoint) {
		return;
	}

	auto bodyA = a_event.bodies[0];  // thrown object
	auto bodyB = a_event.bodies[1];  // target surface/actor

	if (!bodyA || !bodyB) {
		return;
	}

	if (HasThrownObject(bodyB)) {
		bodyA = a_event.bodies[1];
		bodyB = a_event.bodies[0];
	}

	RE::TESObjectREFRPtr thrownObject(bodyA ? bodyB->GetUserData() : nullptr);
	RE::TESObjectREFRPtr target(bodyB ? bodyB->GetUserData() : nullptr);

	if (auto targetActor = target ? target->As<RE::Actor>() : nullptr) {
		// hit damage
		if (auto charController = targetActor->GetCharController()) {
			if (RE::bhkCharacterController::IsHurtfulBody(bodyA)) {
				charController->ProcessHurtfulBody(bodyA, a_event.contactPoint);
			}
		}
	} else {
		if (IsTrigger(bodyB->collidable.GetCollisionLayer())) {
			return;
		}

		if (sendDetectionEvents) {
			// send detection event
			const auto&  contactPos = a_event.contactPoint->position;
			RE::NiPoint3 position = RE::NiPoint3(
										contactPos.quad.m128_f32[0],
										contactPos.quad.m128_f32[1],
										contactPos.quad.m128_f32[2]) *
			                        69.9912510936f;

			if (auto currentProcess = RE::PlayerCharacter::GetSingleton()->currentProcess) {
				auto mass = bodyA->motion.GetMass();

				SKSE::GetTaskInterface()->AddTask([currentProcess, position, mass, thrownObject]() {
					auto soundLevel = GrabThrowHandler::GetSingleton()->GetSoundLevel(mass);
					auto soundLevelValue = RE::AIFormulas::GetSoundLevelValue(soundLevel);

					currentProcess->SetActorsDetectionEvent(RE::PlayerCharacter::GetSingleton(), position, soundLevelValue, thrownObject.get());
				});
			}
		}
	}
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
