#include "TrajectoryOverlay.h"

#include "GrabThrowHandler.h"

void TrajectoryOverlay::RenderOverlay()
{
	if (!enabled) {
		return;
	}

	const auto player = RE::PlayerCharacter::GetSingleton();
	if (!player || player->grabType != RE::PlayerCharacter::GrabbingType::kNormal) {
		return;
	}

	const auto handler = GrabThrowHandler::GetSingleton();

	if (onlyWhileCharging && handler->GetChargeDuration() <= 0.0f) {
		return;
	}

	const auto cell = player->GetParentCell();
	const auto bhkWorld = cell ? cell->GetbhkWorld() : nullptr;
	if (!bhkWorld) {
		return;
	}


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

	const auto max = static_cast<float>(entries);

	if (thickness == 0.0f) {
		thickness = FUCK::Scale(thicknessImpl);
	}

	auto lineColor = GetLineColor(handler->GetChargeFraction());

	for (std::int32_t i = 0; i < entries; i++) {
		auto& [p1, p2] = points[i];

		ImVec2 sp1;
		ImVec2 sp2;
		if (!FUCK::WorldToScreenLoc(p1, sp1) || !FUCK::WorldToScreenLoc(p2, sp2)) {
			continue;
		}

		const auto  cur = static_cast<float>(i);
		const float dt = max - (cur + 0.5f);
		const float alpha = 1.0f - std::clamp(dt / max, 0.0f, 1.0f);

		FUCK::DrawLine(sp1, sp2, ImVec4(lineColor.x, lineColor.y, lineColor.z, (static_cast<float>(trajectoryAlpha) * alpha) / 255.0f), thickness);
	}

	if (hit) {
		if (ImVec2 screen; FUCK::WorldToScreenLoc(hitPos, screen)) {
			if (markerSize == 0.0f) {
				markerSize = FUCK::Scale(markerSizeImpl);
			}
			const auto actualMarkerColor = hitCharacter ?
				                               markerColorActor.GetColor(static_cast<float>(trajectoryAlpha) / 255.0f) :
				                               markerColor.GetColor(static_cast<float>(trajectoryAlpha) / 255.0f);
			FUCK::DrawCircleFilled(screen, markerSize, actualMarkerColor);
		}
	}
}

void TrajectoryOverlay::Load()
{
	lineColorUncharged.Load();
	lineColorCharged.Load();
	markerColor.Load();
	markerColorActor.Load();

	markerSize = 0.0f;
	thickness = 0.0f;
}

void TrajectoryOverlay::Save()
{
	lineColorUncharged.Save();
	lineColorCharged.Save();
	markerColor.Save();
	markerColorActor.Save();
}

TrajectoryOverlay::ColorSetting::ColorSetting(std::string_view a_key, ImVec4 a_defaultRGB) :
	setting("Trajectory", a_key, ToString(a_defaultRGB, true)),
	color(a_defaultRGB)
{}

void TrajectoryOverlay::ColorSetting::Load()
{
	color = ToColor(setting.GetValue());
}

void TrajectoryOverlay::ColorSetting::Save()
{
	setting.SetValue(ToString(color, true));
}


ImVec4 TrajectoryOverlay::ColorSetting::ToColor(const std::string& a_str)
{
	static boost::regex rgb_pattern("([0-9]+),([0-9]+),([0-9]+)");
	static boost::regex hex_pattern("#([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2})");

	boost::smatch rgb_matches;
	boost::smatch hex_matches;

	if (boost::regex_match(a_str, rgb_matches, rgb_pattern)) {
		auto red = std::stoi(rgb_matches[1]);
		auto green = std::stoi(rgb_matches[2]);
		auto blue = std::stoi(rgb_matches[3]);

		return { red / 255.0f, green / 255.0f, blue / 255.0f, 1.0f };
	}
	if (boost::regex_match(a_str, hex_matches, hex_pattern)) {
		auto red = std::stoi(hex_matches[1], 0, 16);
		auto green = std::stoi(hex_matches[2], 0, 16);
		auto blue = std::stoi(hex_matches[3], 0, 16);

		return { red / 255.0f, green / 255.0f, blue / 255.0f, 1.0f };
	}

	return {};
}

std::string TrajectoryOverlay::ColorSetting::ToString(const ImVec4& a_color, bool a_hex)
{
	if (a_hex) {
		return std::format("#{:02X}{:02X}{:02X}{:02X}", static_cast<std::uint32_t>(255.0f * a_color.x), static_cast<std::uint32_t>(255.0f * a_color.y), static_cast<std::uint32_t>(255.0f * a_color.z), static_cast<std::uint32_t>(255.0f * a_color.w));
	}
	return std::format("{},{},{},{}", static_cast<std::uint32_t>(255.0f * a_color.x), static_cast<std::uint32_t>(255.0f * a_color.y), static_cast<std::uint32_t>(255.0f * a_color.z), static_cast<std::uint32_t>(255.0f * a_color.w));
}

void TrajectoryOverlay::TickObjectPath(RE::NiPoint3& a_position, RE::NiPoint3& a_velocity, const RE::NiPoint3& a_gravity, float a_dt)
{
	const RE::NiPoint3 step = a_gravity - (a_velocity * RE::OBJECT_LINEAR_DAMPING);

	a_velocity += step * a_dt;
	a_position += a_velocity * a_dt;
}

ImVec4 TrajectoryOverlay::GetLineColor(float a_chargeFraction) const
{
	const auto& uncharged = lineColorUncharged.GetColor();
	const auto& charged = lineColorCharged.GetColor();

	return { std::lerp(uncharged.x, charged.x, a_chargeFraction),
	         std::lerp(uncharged.y, charged.y, a_chargeFraction),                 
	         std::lerp(uncharged.z, charged.z, a_chargeFraction),     
	         uncharged.w };
}
