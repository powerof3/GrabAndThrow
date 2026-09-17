#include "TrajectoryOverlay.h"

#include "GrabThrowHandler.h"

void TrajectoryOverlay::TickObjectPath(RE::NiPoint3& a_position, RE::NiPoint3& a_velocity, const RE::NiPoint3& a_gravity, float a_dt) const
{
	const RE::NiPoint3 step = a_gravity - (a_velocity * RE::OBJECT_LINEAR_DAMPING);

	a_velocity += step * a_dt;
	a_position += a_velocity * a_dt;
}

void TrajectoryOverlay::RenderOverlay()
{
	if (!enabled) {
		return;
	}

	const auto player = RE::PlayerCharacter::GetSingleton();
	if (!player || player->grabType != RE::PlayerCharacter::GrabbingType::kNormal) {
		return;
	}

	const auto cell = player->GetParentCell();
	const auto bhkWorld = cell ? cell->GetbhkWorld() : nullptr;
	if (!bhkWorld) {
		return;
	}

	const auto handler = GrabThrowHandler::GetSingleton();

	std::array<Segment, segCount> points;
	std::int32_t                  entries = 0;

	RE::hkpRigidBody* grabbedBody = nullptr;
	bool              hit = false;
	bool              hitCharacter = false;
	RE::NiPoint3      hitPos;

	{
		RE::BSReadLockGuard locker(bhkWorld->worldLock);

		grabbedBody = handler->GetGrabbedBody(player);
		if (!grabbedBody) {
			return;
		}

		auto pos = RE::HKToGame(grabbedBody->motion.motionState.transform.translation);
		auto velocity = handler->GetThrowVelocity();
		auto gravity = RE::GetGravity(bhkWorld);

		RE::CFilter filterInfo;
		player->GetCollisionFilterInfo(filterInfo);
		filterInfo.SetCollisionLayer(RE::COL_LAYER::kLOS);

		RE::NiPoint3 lastPos = pos;
		RE::NiPoint3 curPos = pos;

		for (std::int32_t i = 0; i < segCount; i++) {
			TickObjectPath(curPos, velocity, gravity, timeStep);

			RE::bhkPickData pickData;
			pickData.rayInput.from = lastPos * RE::BS_TO_HK_SCALE;
			pickData.rayInput.to = curPos * RE::BS_TO_HK_SCALE;
			pickData.rayInput.filterInfo = filterInfo;
			pickData.rayInput.enableShapeCollectionFilter = true;

			if (bhkWorld->PickObject(pickData); pickData.rayOutput.HasHit()) {
				if (const auto& collidable = pickData.rayOutput.rootCollidable; collidable && collidable != &grabbedBody->collidable) {
					hitPos = lastPos + (curPos - lastPos) * pickData.rayOutput.hitFraction;
					points[entries++] = { lastPos, hitPos };
					hit = true;

					switch (collidable->GetCollisionLayer()) {
					case RE::COL_LAYER::kCharController:
					case RE::COL_LAYER::kBiped:
					case RE::COL_LAYER::kBipedNoCC:
					case RE::COL_LAYER::kDeadBip:
						hitCharacter = true;
						break;
					default:
						break;
					}	

					// exit simulation
					break;
				}
			}

			points[entries++] = { lastPos, curPos };
			lastPos = curPos;
		}

		if (entries <= 0) {
			return;
		}
	}

	const float chargeT = handler->GetChargeFraction();
	const auto  r = 1.0f;
	const auto  g = std::lerp(251.0f, 170.0f, chargeT) / 255.0f;
	const auto  b = std::lerp(251.0f, 40.0f, chargeT) / 255.0f;

	const float max = static_cast<float>(entries);
	const auto  lineThickness = FUCK::Scale(thickness);

	for (std::int32_t i = 0; i < entries; i++) {
		auto& [p1, p2] = points[i];

		ImVec2 sp1, sp2;
		if (!FUCK::WorldToScreenLoc(p1, sp1) || !FUCK::WorldToScreenLoc(p2, sp2)) {
			continue;
		}
	
		const float cur = static_cast<float>(i);
		const float dt = max - (cur + 0.5f);
		const float alphaLinear = 1.0f - std::clamp(dt / max, 0.0f, 1.0f);
		const float alpha = std::pow(alphaLinear, 2.20f);

		FUCK::DrawLine(sp1, sp2, ImVec4(r, g, b, (trajectoryAlpha * alpha) /  255.0f), lineThickness);
	}

	if (hit) {
		ImVec2 screen;
		if (FUCK::WorldToScreenLoc(hitPos, screen))
			{
			const auto markerRadius = FUCK::Scale(markerSizeImpl);
			const auto markerColor = hitCharacter ?
			                             ImVec4(255.0f / 255.0f, 40.0f / 255.0f, 40.0f / 255.0f, static_cast<float>(trajectoryAlpha) / 255.0f) :
			                             ImVec4(251.0f / 255.0f, 251.0f / 255.0f, 251.0f / 255.0f, static_cast<float>(trajectoryAlpha) / 255.0f);
			FUCK::DrawCircleFilled(screen, markerRadius, markerColor);
		}
	}
}
