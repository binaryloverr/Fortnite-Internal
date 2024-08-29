#pragma once

float ScreenWidth;
float ScreenHeight;


namespace Renderer_Defines {
	float Width;
	float Height;
}

namespace Settings
{
	//Aim
	static bool Aim = 0;
	static bool AimPrediction = 0;
	static bool MouseAim = 0;
	int aimbone = 66;
	float smooth = 3;
	static bool SilentAim = 0;
	static bool Bullettp = 0;
	float FovCircle_Value = 130;
	
	

	//Player ESP
	static bool Box = 1;
	static bool Skeleton = 1;
	static bool SnapLines = 0;
	static bool TargetLine = 0;
	static bool DistanceESP = 1;
	static bool PlatformESP = 0;
	
	//Loot ESP
	static bool ChestESP = 0;
	static bool LLamaESP = 0;
	static bool WeaponESP = 0;
	static bool LootESP = 0;
	static bool AmmoBoxESP = 0;
	static bool VehiclesESP = 0;


	//Exploits
	static bool FOVChanger = 0;
	static float FOV = 88.0f;

	//Misc
	static int MaxESPDistance = 300;
	static bool Crosshair = 0;
	static bool ShowFovCircle = 0;
	static bool VisibleCheck = 0;
	


}