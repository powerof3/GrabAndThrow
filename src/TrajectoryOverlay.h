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

	void Load();
	void Save();

private:
	friend class SettingsTool;

	struct Segment
	{
		RE::NiPoint3 a;
		RE::NiPoint3 b;
	};

	struct ColorSetting
	{
		ColorSetting(std::string_view a_key, ImVec4 a_defaultRGB);

		ImVec4&       GetColor() { return color; }
		const ImVec4& GetColor() const { return color; }

		ImVec4 GetColor(float a_alpha) const { return ImVec4(color.x, color.y, color.z, a_alpha); }

		void Load();
		void Save();

	private:
		static ImVec4      ToColor(const std::string& a_str);
		static std::string ToString(const ImVec4& a_color, bool a_hex);

		REX::TIniSetting<std::string> setting;
		ImVec4                        color;
	};

	static void TickObjectPath(RE::NiPoint3& a_position, RE::NiPoint3& a_velocity, const RE::NiPoint3& a_gravity, float a_dt);
	ImVec4      GetLineColor(float a_chargeFraction) const;

	// members
	REX::TIniSetting<bool> enabled{ "Trajectory", "bShowTrajectory", true };
	REX::TIniSetting<bool> onlyWhileCharging{ "Trajectory", "bOnlyWhileCharging", true };

	REX::TIniSetting<float> thicknessImpl{ "Trajectory", "fLineThickness", 3.0f };
	float                   thickness{ 0.0f };

	REX::TIniSetting<float> markerSizeImpl{ "Trajectory", "fMarkerSize", 6.0f };
	float                   markerSize{ 0.0f };

	REX::TIniSetting<std::int32_t> trajectoryAlpha{ "Trajectory", "iOpacity", 230 };

	ColorSetting lineColorUncharged{ "sLineColorUncharged", ImVec4(0.984f, 0.984f, 0.984f, 1.0f) };
	ColorSetting lineColorCharged{ "sLineColorCharged", ImVec4(1.0f, 0.667f, 0.157f, 1.0f) };
	ColorSetting markerColor{ "sMarkerColor", ImVec4(0.984f, 0.984f, 0.984f, 1.0f) };
	ColorSetting markerColorActor{ "sMarkerColorActor", ImVec4(1.0f, 0.157f, 0.157f, 1.0f) };

	static constexpr std::int32_t segCount{ 128 };
	static constexpr float        timeStep{ 1.0f / 20.0f };
};
