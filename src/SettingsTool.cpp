#include "SettingsTool.h"

#include "GrabThrowHandler.h"
#include "TrajectoryOverlay.h"

void SettingsTool::Draw()
{
	const auto handler = GrabThrowHandler::GetSingleton();

	if (FUCK::BeginTabBar("##Settings")) {
		if (FUCK::BeginTabItem("$GT_Throw"_T)) {
			FUCK::SliderFloat("$GT_BaseImpulse"_T, &stl::setting(handler->playerGrabThrowImpulseBase), 0.0f, 1000.0f, "%.0f");
			FUCK::SliderFloat("$GT_MaxChargeImpulse"_T, &stl::setting(handler->playerGrabThrowImpulseMax), 0.0f, 4000.0f, "%.0f");
			FUCK::SliderFloat("$GT_ChargeRate"_T, &stl::setting(handler->playerGrabThrowStrengthMult), 50.0f, 2000.0f, "%.0f");
			FUCK::SliderFloat("$GT_DamageMult"_T, &stl::setting(handler->playerGrabThrowDamageMult), 0.0f, 1000.0f, "%.1f");
			FUCK::EndTabItem();
		}

		if (FUCK::BeginTabItem("$GT_Events"_T)) {
			FUCK::Checkbox("$GT_DetectionEvents"_T, &stl::setting(handler->sendDetectionEvents));
			FUCK::Checkbox("$GT_HitEventTarget"_T, &stl::setting(handler->sendTargetHitEvents));
			FUCK::Checkbox("$GT_HitEventThrown"_T, &stl::setting(handler->sendThrownHitEvents));
			FUCK::EndTabItem();
		}

		if (FUCK::BeginTabItem("$GT_Destruction"_T)) {
			FUCK::Checkbox("$GT_Enable"_T, &stl::setting(handler->destroyObjects));

			FUCK::BeginDisabled(!handler->destroyObjects);
			{
				FUCK::SliderFloat("$GT_SelfDamage"_T, &stl::setting(handler->destructibleSelfDamage), 0.0f, 10.0f, "%.1f");
				FUCK::SliderFloat("$GT_TargetDamage"_T, &stl::setting(handler->destructibleTargetDamage), 0.0f, 10.0f, "%.1f");
				FUCK::SliderFloat("$GT_MinDestructibleSpeed"_T, &stl::setting(handler->minDestructibleSpeed), 0.0f, 30.0f, "%.1f");
			}
			FUCK::EndDisabled();
			FUCK::EndTabItem();
		}

		if (FUCK::BeginTabItem("$GT_GrabPhysics"_T)) {
			bool edited = false;
			edited |= FUCK::SliderFloat("$GT_MaxForce"_T, &stl::setting(handler->fZKeyMaxForce), 0.0f, 2000.0f, "%.0f");
			edited |= FUCK::SliderFloat("$GT_ForceWeightHigh"_T, &stl::setting(handler->fZKeyMaxForceWeightHigh), 0.0f, 1000.0f, "%.0f");
			edited |= FUCK::SliderFloat("$GT_ForceWeightLow"_T, &stl::setting(handler->fZKeyMaxForceWeightLow), 0.0f, 1000.0f, "%.0f");
			edited |= FUCK::SliderFloat("$GT_MaxContactDistance"_T, &stl::setting(handler->fZKeyMaxContactDistance), 10.0f, 500.0f, "%.0f");
			edited |= FUCK::SliderFloat("$GT_ObjectDamping"_T, &stl::setting(handler->fZKeyObjectDamping), 0.0f, 2.0f, "%.2f");
			edited |= FUCK::SliderFloat("$GT_SpringDamping"_T, &stl::setting(handler->fZKeySpringDamping), 0.0f, 2.0f, "%.2f");
			edited |= FUCK::SliderFloat("$GT_SpringElasticity"_T, &stl::setting(handler->fZKeySpringElasticity), 0.0f, 2.0f, "%.2f");
			edited |= FUCK::SliderFloat("$GT_HeavyWeight"_T, &stl::setting(handler->fZKeyHeavyWeight), 0.0f, 1000.0f, "%.0f");
			edited |= FUCK::SliderFloat("$GT_ComplexHelperWeightMax"_T, &stl::setting(handler->fZKeyComplexHelperWeightMax), 0.0f, 1000.0f, "%.0f");

			if (edited) {
				gmstEdited = true;
			}

			FUCK::BeginDisabled(!gmstEdited);
			{
				if (FUCK::Button("$GT_ApplyGameSettings"_T)) {
					handler->ApplyGameSettings();
					gmstEdited = false;
				}
			}
			FUCK::EndDisabled();
			FUCK::EndTabItem();
		}

		if (FUCK::BeginTabItem("$GT_TrajectoryPreview"_T)) {
			const auto overlay = TrajectoryOverlay::GetSingleton();

			FUCK::Checkbox("$GT_ShowTrajectory"_T, &stl::setting(overlay->enabled));

			FUCK::BeginDisabled(!overlay->enabled);
			{
				FUCK::Checkbox("$GT_OnlyWhileCharging"_T, &stl::setting(overlay->onlyWhileCharging));

				FUCK::SliderInt("$GT_Opacity"_T, &stl::setting(overlay->trajectoryAlpha), 0, 255);

				if (FUCK::SliderFloat("$GT_LineThickness"_T, &stl::setting(overlay->thicknessImpl), 1.0f, 10.0f, "%.1f")) {
					overlay->thickness = FUCK::Scale(overlay->thicknessImpl);
				}

				if (FUCK::SliderFloat("$GT_MarkerSize"_T, &stl::setting(overlay->markerSizeImpl), 2.0f, 24.0f, "%.0f")) {
					overlay->markerSize = FUCK::Scale(overlay->markerSizeImpl);
				}

				FUCK::ColorEdit3("$GT_LineColorUncharged"_T, &overlay->lineColorUncharged.GetColor().x);
				FUCK::ColorEdit3("$GT_LineColorCharged"_T, &overlay->lineColorCharged.GetColor().x);

				FUCK::ColorEdit3("$GT_MarkerColor"_T, &overlay->markerColor.GetColor().x);
				FUCK::ColorEdit3("$GT_MarkerColorActor"_T, &overlay->markerColorActor.GetColor().x);
			}
			FUCK::EndDisabled();

			FUCK::EndTabItem();
		}
		FUCK::EndTabBar();
	}

	FUCK::Spacing();
	FUCK::SeparatorThick();
	FUCK::Spacing();

	if (FUCK::Button("$GT_SaveToINI"_T)) {
		handler->SaveSettings();
	}
	FUCK::SameLine();
	if (FUCK::Button("$GT_ReloadFromINI"_T)) {
		handler->LoadSettings();
		handler->ApplyGameSettings();
		gmstEdited = false;
	}
}
