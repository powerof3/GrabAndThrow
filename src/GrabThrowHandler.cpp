#include "GrabThrowHandler.h"

#include "TrajectoryOverlay.h"

bool GrabThrowHandler::LoadSettings()
{
	(void)TrajectoryOverlay::GetSingleton();
	
	const auto store = REX::FIniSettingStore::GetSingleton();
	store->Init(path.data(), "");

	store->Load();
	store->Save();

	return true;
}

void GrabThrowHandler::OnDataLoad()
{
	REX::INFO("{:*^30}", "GAME SETTINGS");

	fPhysicsDamage1Mass = 20.0f;

	fPhysicsDamage2Mass = "fPhysicsDamage2Mass"_gs.value();
	fPhysicsDamage3Mass = "fPhysicsDamage3Mass"_gs.value();
	fPhysicsDamage1Damage = "fPhysicsDamage1Damage"_gs.value();
	fPhysicsDamage2Damage = "fPhysicsDamage2Damage"_gs.value();
	fPhysicsDamage3Damage = "fPhysicsDamage3Damage"_gs.value();
	fPhysicsDamageSpeedMin = "fPhysicsDamageSpeedMin"_gs.value();
	fPhysicsDamageSpeedMult = "fPhysicsDamageSpeedMult"_gs.value();
	fPhysicsDamageSpeedBase = "fPhysicsDamageSpeedBase"_gs.value();

	ApplyGameSettings();
}

void GrabThrowHandler::ApplyGameSettings() const
{
	constexpr auto set_gmst = [](const char* setting, float a_value) {
		auto gmst = RE::GameSettingCollection::GetSingleton()->GetSetting(setting);
		REX::INFO("{}: {} -> {}", setting, gmst->GetFloat(), a_value);
		gmst->data.f = a_value;
	};

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

void GrabThrowHandler::SaveSettings() const
{
	REX::FIniSettingStore::GetSingleton()->Save();
}

float GrabThrowHandler::GetRealMass(RE::hkpRigidBody* a_body)
{
	if (a_body) {
		if (auto property = a_body->GetProperty(RE::HK_PROPERTY_TEMPORARYMASS)) {
			return property->value.GetFloat();
		}
		return a_body->motion.GetMass();
	}

	return 0.0f;
}

bool GrabThrowHandler::HasThrownObject(RE::hkpRigidBody* a_body)
{
	return a_body && a_body->HasProperty(RE::HK_PROPERTY_GRABTHROWNOBJECT);
}

void GrabThrowHandler::SetThrownObject(RE::hkpRigidBody* a_body, float a_value)
{
	if (!a_body->HasProperty(RE::HK_PROPERTY_GRABTHROWNOBJECT)) {
		a_body->SetProperty(RE::HK_PROPERTY_GRABTHROWNOBJECT, a_value);
		a_body->AddContactListener(this);
	}
}

bool GrabThrowHandler::HasThrownObject(RE::bhkRigidBody* a_body)
{
	return HasThrownObject(a_body ? a_body->GetRigidBody() : nullptr);
}

float GrabThrowHandler::GetThrownObjectValue(RE::bhkRigidBody* a_body)
{
	if (auto hkpBody = a_body ? a_body->GetRigidBody() : nullptr) {
		if (auto property = hkpBody->GetProperty(RE::HK_PROPERTY_GRABTHROWNOBJECT)) {
			return property->value.GetFloat() + 1.0f;
		}
	}

	return 1.0f;
}

bool GrabThrowHandler::ClearThrownObject(RE::bhkRigidBody* a_body)
{
	if (auto hkpBody = a_body ? a_body->GetRigidBody() : nullptr; hkpBody && hkpBody->HasProperty(RE::HK_PROPERTY_GRABTHROWNOBJECT)) {
		hkpBody->RemoveProperty(RE::HK_PROPERTY_GRABTHROWNOBJECT);
		hkpBody->RemoveContactListener(this);
		return true;
	}

	return false;
}

float GrabThrowHandler::GetForce() const
{
	auto force = (chargeDuration * playerGrabThrowStrengthMult);
	force = std::min(force, playerGrabThrowImpulseMax.GetValue());
	force += playerGrabThrowImpulseBase;
	return force;
}

RE::NiPoint3 GrabThrowHandler::GetAimDirection(RE::PlayerCharacter* a_player) const
{
	const auto camera = RE::PlayerCamera::GetSingleton();
	
	if (camera && camera->cameraRoot) {
		const auto&  rot = camera->cameraRoot->world.rotate;
		RE::NiPoint3 dir(rot.entry[0][1], rot.entry[1][1], rot.entry[2][1]);
		if (dir.Unitize() > 0.0f) {
			return dir;
		}
	}

	const float pitch = a_player->GetAngleX();
	const float yaw = a_player->GetAngleZ();
	const float cosPitch = std::cos(pitch);

	RE::NiPoint3 dir(
		std::sin(yaw) * cosPitch,
		std::cos(yaw) * cosPitch,
		-std::sin(pitch));
	dir.Unitize();
	return dir;
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

RE::NiPoint3 GrabThrowHandler::GetThrowVelocity() const
{
	const auto player = RE::PlayerCharacter::GetSingleton();
	return GetAimDirection(player) * GetForce();
}

RE::hkpRigidBody* GrabThrowHandler::GetGrabbedBody(RE::PlayerCharacter* a_player) const
{
	for (const auto& mouseSpring : a_player->grabSpring) {
		if (mouseSpring && mouseSpring->referencedObject) {
			if (auto hkMouseSpring = skyrim_cast<RE::hkpMouseSpringAction*>(mouseSpring->referencedObject.get())) {
				return reinterpret_cast<RE::hkpRigidBody*>(hkMouseSpring->entity);
			}
		}
	}
	return nullptr;
}

float GrabThrowHandler::GetChargeFraction() const
{
	const float chargeTime = playerGrabThrowStrengthMult > 0.0f ?
	                             playerGrabThrowImpulseMax / playerGrabThrowStrengthMult :
	                             0.0f;

	return chargeTime > 0.0f ? std::clamp(chargeDuration / chargeTime, 0.0f, 1.0f) : 1.0f;
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

	RE::TESObjectREFRPtr thrownObject(bodyA ? bodyA->GetUserData() : nullptr);
	RE::TESObjectREFRPtr target(bodyB ? bodyB->GetUserData() : nullptr);

	if (auto targetActor = target ? target->As<RE::Actor>() : nullptr) {
		// accumulative hit damage
		if (auto charController = targetActor->GetCharController()) {
			if (RE::bhkCharacterController::IsHurtfulBody(bodyA)) {
				charController->ProcessHurtfulBody(bodyA, a_event.contactPoint);
			}
		}
	} else {
		// accept first hit only
		if (!a_event.firstCallbackForFullManifold || !thrownObject || IsTrigger(bodyB->collidable.GetCollisionLayer())) {
			return;
		}

		bool playerSneaking = RE::PlayerCharacter::GetSingleton()->IsSneaking();

		// Hit
		if (sendTargetHitEvents && target) {
			const RE::TESHitEvent event(target.get(), thrownObject.get(), 0x14, 0, playerSneaking ? RE::TESHitEvent::Flag::kSneakAttack : RE::TESHitEvent::Flag::kNone);
			RE::ScriptEventSourceHolder::GetSingleton()->SendEvent(&event);
		}
		if (sendThrownHitEvents) {
			const RE::TESHitEvent event(thrownObject.get(), target.get(), 0x14, 0, playerSneaking ? RE::TESHitEvent::Flag::kSneakAttack : RE::TESHitEvent::Flag::kNone);
			RE::ScriptEventSourceHolder::GetSingleton()->SendEvent(&event);
		}

		auto thrownObjectMass = GrabThrowHandler::GetRealMass(bodyA);  // not fake gPhysics1Mass we used for impulse calcs
		auto targetMass = bodyB->motion.GetMass();

		// Detection
		if (sendDetectionEvents) {
			// send detection event
			auto position = RE::HKToGame(a_event.contactPoint->position);

			if (RE::PlayerCharacter::GetSingleton()->currentProcess) {
				SKSE::GetTaskInterface()->AddTask([position, thrownObjectMass, thrownObject]() {
					auto player = RE::PlayerCharacter::GetSingleton();
					auto soundLevel = GrabThrowHandler::GetSingleton()->GetSoundLevel(thrownObjectMass);
					auto soundLevelValue = RE::AIFormulas::GetSoundLevelValue(soundLevel);

					player->currentProcess->SetActorsDetectionEvent(
						player,
						position,
						soundLevelValue,
						thrownObject.get());
				});
			}
		}

		// Destruction
		if (destroyObjects) {
			if (bodyA->motion.linearVelocity.Length3() > minDestructibleSpeed) {
				if (destructibleTargetDamage > 0.0f && thrownObjectMass >= targetMass) {
					RE::TaskQueueInterface::GetSingleton()->QueueUpdateDestructibleObject(target.get(), destructibleTargetDamage, false, nullptr);
				}
				if (destructibleSelfDamage > 0.0f) {
					RE::TaskQueueInterface::GetSingleton()->QueueUpdateDestructibleObject(thrownObject.get(), destructibleSelfDamage, false, nullptr);
				}
			}
		}
	}
}

void GrabThrowHandler::ThrowGrabbedObject(RE::PlayerCharacter* a_player)
{
	auto cell = a_player->GetParentCell();
	auto bhkWorld = cell ? cell->GetbhkWorld() : nullptr;

	if (!bhkWorld) {
		return;
	}

	RE::BSWriteLockGuard locker(bhkWorld->worldLock);

	RE::NiPoint3  velocity = GetThrowVelocity() * RE::BS_TO_HK_SCALE;
	RE::hkVector4 hkVelocity(velocity.x, velocity.y, velocity.z, 0);

	for (const auto& mouseSpring : a_player->grabSpring) {
		if (mouseSpring && mouseSpring->referencedObject) {
			if (auto hkMouseSpring = skyrim_cast<RE::hkpMouseSpringAction*>(mouseSpring->referencedObject.get())) {
				if (auto hkpRigidBody = reinterpret_cast<RE::hkpRigidBody*>(hkMouseSpring->entity)) {
					auto bhkRigidBody = reinterpret_cast<RE::bhkRigidBody*>(hkpRigidBody->userData);

					SetThrownObject(hkpRigidBody, GetChargeDuration());

					auto mass = hkpRigidBody->motion.GetMass();
					if (mass < fPhysicsDamage1Mass) {
						RE::TESHavokUtilities::PushTemporaryMass(bhkRigidBody, fPhysicsDamage1Mass);
						mass = fPhysicsDamage1Mass;
					}

					hkpRigidBody->SetLinearVelocity(RE::hkVector4());
					hkpRigidBody->SetAngularVelocity(RE::hkVector4());
					hkpRigidBody->ApplyLinearImpulse(hkVelocity * mass);
				}
			}
		}
	}
}
