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
		template <class T>
		T ToColor(const std::string& a_str);

		template <class T>
		std::string ToString(const T& a_style, bool a_hex);

		REX::TIniSetting<std::string> setting;
		ImVec4                        color;
	};

	static void TickObjectPath(RE::NiPoint3& a_position, RE::NiPoint3& a_velocity, const RE::NiPoint3& a_gravity, float a_dt);
	ImVec4      GetLineColor(float a_chargeFraction) const;

	// members
	REX::TIniSetting<bool> enabled{ "Trajectory", "bEnable", true };
	REX::TIniSetting<bool> showWhenCharging{ "Trajectory", "bEnableWhenCharging", true };

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

template <class T>
T TrajectoryOverlay::ColorSetting::ToColor(const std::string& a_str)
{
	if constexpr (std::is_same_v<ImVec4, T>) {
		static boost::regex rgb_pattern("([0-9]+),([0-9]+),([0-9]+),([0-9]+)");
		static boost::regex hex_pattern("#([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2})");

		boost::smatch rgb_matches;
		boost::smatch hex_matches;

		if (boost::regex_match(a_str, rgb_matches, rgb_pattern)) {
			auto red = std::stoi(rgb_matches[1]);
			auto green = std::stoi(rgb_matches[2]);
			auto blue = std::stoi(rgb_matches[3]);
			auto alpha = std::stoi(rgb_matches[4]);

			return { red / 255.0f, green / 255.0f, blue / 255.0f, alpha / 255.0f };
		}
		if (boost::regex_match(a_str, hex_matches, hex_pattern)) {
			auto red = std::stoi(hex_matches[1], 0, 16);
			auto green = std::stoi(hex_matches[2], 0, 16);
			auto blue = std::stoi(hex_matches[3], 0, 16);
			auto alpha = std::stoi(hex_matches[4], 0, 16);

			return { red / 255.0f, green / 255.0f, blue / 255.0f, alpha / 255.0f };
		}

		return T();
	} else {
		return REX::STR::TO_NUM<T>(a_str);
	}
}

template <class T>
std::string TrajectoryOverlay::ColorSetting::ToString(const T& a_style, bool a_hex)
{
	if constexpr (std::is_same_v<ImVec4, T>) {
		if (a_hex) {
			return std::format("#{:02X}{:02X}{:02X}{:02X}", static_cast<std::uint8_t>(255.0f * a_style.x), static_cast<std::uint8_t>(255.0f * a_style.y), static_cast<std::uint8_t>(255.0f * a_style.z), static_cast<std::uint8_t>(255.0f * a_style.w));
		}
		return std::format("{},{},{},{}", static_cast<std::uint8_t>(255.0f * a_style.x), static_cast<std::uint8_t>(255.0f * a_style.y), static_cast<std::uint8_t>(255.0f * a_style.z), static_cast<std::uint8_t>(255.0f * a_style.w));
	} else {
		return std::format("{:.3f}", a_style);
	}
}
