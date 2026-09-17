#pragma once

class SettingsTool :
	public REX::TSingleton<SettingsTool>,
	public FUCK::ITool
{
public:
	const char* Name() const override { return "$GT_Title"_T; }
	void        Draw() override;

private:
	bool gmstEdited{ false };
};
