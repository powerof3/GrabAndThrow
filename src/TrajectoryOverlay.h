#pragma once

class TrajectoryOverlay :
	public REX::TSingleton<TrajectoryOverlay>,
	public FUCK::IWindow
{
public:
	const char* Id() const override { return "ThrowTrajectory"; }
	const char* Title() const override { return "$GT_Title"_T; }

	void Draw() override {}
	void RenderOverlay() override;

	bool IsOpen() const override { return true; }
	void SetOpen(bool) override {}

	FUCK::WindowFlags GetFlags() const override
	{
		return FUCK::WindowFlags::kNoBackground | FUCK::WindowFlags::kNoDecoration | FUCK::WindowFlags::kNoResize | FUCK::WindowFlags::kNoMove |
		       FUCK::WindowFlags::kPassInputToGame | FUCK::WindowFlags::kIgnoreUserScale | FUCK::WindowFlags::kCustomPosition;
	}

private:
	friend class SettingsTool;

	struct Segment
	{
		RE::NiPoint3 a;
		RE::NiPoint3 b;
	};

	void TickObjectPath(RE::NiPoint3& a_position, RE::NiPoint3& a_velocity, const RE::NiPoint3& a_gravity, float a_dt) const;

	REX::TIniSetting<bool>         enabled{ "Trajectory", "bEnable", true };
	REX::TIniSetting<float>        thickness{ "Trajectory", "fLineThickness", 3.0f };
	REX::TIniSetting<float>        markerSizeImpl{ "Trajectory", "fMarkerSize", 6.0f };
	REX::TIniSetting<std::int32_t> trajectoryAlpha{ "Trajectory", "iOpacity", 230 };

	static constexpr std::int32_t segCount{ 128 };
	static constexpr float        timeStep{ 1.0f / 20.0f };
};
