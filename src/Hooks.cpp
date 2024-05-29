#include "Hooks.h"
#include "GrabThrowHandler.h"

namespace Hooks
{
	// main hooks for grab and throw
	namespace GrabThrow
	{
		struct ProcessInput
		{
			static bool thunk(RE::AttackBlockHandler* a_this, RE::InputEvent* a_event)
			{
				if (RE::PlayerCharacter::GetSingleton()->grabType == RE::PlayerCharacter::GrabbingType::kNormal) {
					return false;
				}

				return func(a_this, a_event);
			}
			static inline REL::Relocation<decltype(thunk)> func;
			static inline constexpr std::size_t            idx = 0x1;
		};

		struct ProcessButton
		{
			static void thunk(RE::ReadyWeaponHandler* a_this, RE::ButtonEvent* a_event, [[maybe_unused]] RE::PlayerControlsData* a_data)
			{
				auto player = RE::PlayerCharacter::GetSingleton();

				if (player->grabType == RE::PlayerCharacter::GrabbingType::kNormal) {
					if (a_event->IsUp()) {
						GrabThrowHandler::GetSingleton()->ThrowGrabbedObject(player, a_event->HeldDuration());
						player->DestroyMouseSprings();
					}
					return;
				}

				return func(a_this, a_event, a_data);
			}
			static inline REL::Relocation<decltype(thunk)> func;
			static inline constexpr std::size_t            idx = 0x4;
		};

		// no telekinesis damage
		struct IsTelekinesisObject
		{
			static bool thunk(RE::bhkRigidBody* a_body)
			{
				auto result = func(a_body);
				if (result && GrabThrowHandler::HasThrownObject(a_body)) {
					GrabThrowHandler::GetSingleton()->ClearThrownObject(a_body);
					RE::TESHavokUtilities::PopTemporaryMass(a_body);
					return false;
				}
				return result;
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

		struct InitializeImpactData
		{
			static void thunk(RE::HitData* a_hitData, std::uint64_t a_unk02, RE::TESObjectREFR* a_ref, float a_damageFromImpact, RE::DamageImpactData* a_impactDamageData)
			{			
				float             damageFromImpact = a_damageFromImpact;
				RE::bhkRigidBody* body = a_impactDamageData->body.get();
				bool              isThrownObject = GrabThrowHandler::HasThrownObject(body);

				if (body && isThrownObject) {
					if (damageFromImpact == 0.0f) {
						auto hkpBody = body->GetRigidBody();
						auto mass = hkpBody->motion.GetMass();
						auto speed = a_impactDamageData->velocity.Length();

						damageFromImpact = GrabThrowHandler::GetSingleton()->GetFinalDamageForImpact(mass, speed);
					} else {
						damageFromImpact = GrabThrowHandler::GetSingleton()->GetFinalDamageForImpact(damageFromImpact);
					}
					damageFromImpact *= GrabThrowHandler::GetThrownObjectValue(body);
				}

				func(a_hitData, a_unk02, a_ref, damageFromImpact, a_impactDamageData);

				if (isThrownObject) {
					a_hitData->aggressor = RE::PlayerCharacter::GetSingleton()->GetHandle();
				}
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

		struct ClearTelekinesisObject
		{
			static void thunk(RE::bhkRigidBody* a_body)
			{
				func(a_body);

				GrabThrowHandler::GetSingleton()->ClearThrownObject(a_body);
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

		void Install()
		{
			stl::write_vfunc<RE::ReadyWeaponHandler, ProcessButton>();
			stl::write_vfunc<RE::AttackBlockHandler, ProcessInput>();

			const REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(42836, 44005), 0x50 };  // HitData::InitializeImpactData
			stl::write_thunk_call<IsTelekinesisObject>(target.address());

			const REL::Relocation<std::uintptr_t> target1{ RELOCATION_ID(37186, 38185), 0x200 };  // ProcessDamageImpacts
			stl::write_thunk_call<InitializeImpactData>(target1.address());

			const REL::Relocation<std::uintptr_t> target2{ RELOCATION_ID(25327, 25850), OFFSET(0xE4, 0xF4) };  // FOCollisionListener::ReferenceDeactivated
			stl::write_thunk_call<ClearTelekinesisObject>(target2.address());

			logger::info("Installed grab throw hooks");
		}
	}

	void Install()
	{
		GrabThrow::Install();
	}
}
