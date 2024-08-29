#include "SDK.h"

typedef int (WINAPI* LPFN_MBA)(DWORD);
static LPFN_MBA InvadeKeySync;


bool ShowMenu = true;

int Menu_AimBoneInt = 0;


static const char* AimBone_TypeItems[]{
	"   Head",
	"   Neck",
	"   Dick",
	"   Feet"
};

int BoxModePosition = 0;
static const char* BoxModes[]
{
	"2D Box",
	"Cornered Box"
};

float color_red = 1.;
float color_green = 0;
float color_blue = 0;
float color_random = 0.0;
float color_speed = -10.0;

void ColorChange()
{
	static float Color[3];
	static DWORD Tickcount = 0;
	static DWORD Tickcheck = 0;
	ImGui::ColorConvertRGBtoHSV(color_red, color_green, color_blue, Color[0], Color[1], Color[2]);
	if (GetTickCount() - Tickcount >= 1)
	{
		if (Tickcheck != Tickcount)
		{
			Color[0] += 0.001f * color_speed;
			Tickcheck = Tickcount;
		}
		Tickcount = GetTickCount();
	}
	if (Color[0] < 0.0f) Color[0] += 1.0f;
	ImGui::ColorConvertHSVtoRGB(Color[0], Color[1], Color[2], color_red, color_green, color_blue);
}
inline uintptr_t GetGameBase()
{
	return *(uintptr_t*)(__readgsqword(0x60) + 0x10);
}

namespace Speed {

	template<class SpeedT>
	class MainSpeed {
		SpeedT time_offset;
		SpeedT time_last_update;

		double speed_;

	public:
		MainSpeed(SpeedT currentRealTime, double initialSpeed) {
			time_offset = currentRealTime;
			time_last_update = currentRealTime;
			speed_ = initialSpeed;
		}

		SpeedT getSpeed(SpeedT currentRealTime) {
			SpeedT difference = currentRealTime - time_last_update;
			return (SpeedT)(speed_ * difference) + time_offset;
		}

		void setSpeed(SpeedT currentRealTime, double speed) {
			time_offset = getSpeed(currentRealTime);
			time_last_update = currentRealTime;
			speed_ = speed;
		}


	};

	typedef DWORD(WINAPI* NtGetTickCount_)(void);
	typedef ULONGLONG(WINAPI* NtGetTickCount64_)(void);
	typedef DWORD(WINAPI* NtTimeGetTime_)(void);
	typedef BOOL(WINAPI* NtQueryPerformanceCounter_)(LARGE_INTEGER* lpPerformanceCount);

	static NtGetTickCount_ NtGetTickCount_original;
	static NtGetTickCount64_ NtGetTickCount64_original;
	static NtTimeGetTime_ NttimeGetTime_original;
	static NtQueryPerformanceCounter_ NtQueryPerformanceCounter_original;

	static MainSpeed<DWORD> g_MainSpeed(0, 0);
	static MainSpeed<ULONGLONG> g_MainSpeed64(0, 0);
	static MainSpeed<LONGLONG> g_MainSpeed_Long(0, 0);


	static DWORD WINAPI GetTickCountHook(void) {
		return g_MainSpeed.getSpeed(NtGetTickCount_original());
	}

	static ULONGLONG WINAPI GetTickCount64Hook(void) {
		return g_MainSpeed64.getSpeed(NtGetTickCount64_original());
	}

	static BOOL WINAPI QueryPerformanceCounterHook(LARGE_INTEGER* lpPerformanceCount) {
		LARGE_INTEGER PerformanceCount;
		BOOL ReturnValue = NtQueryPerformanceCounter_original(&PerformanceCount);
		lpPerformanceCount->QuadPart = g_MainSpeed_Long.getSpeed(PerformanceCount.QuadPart);
		return ReturnValue;
	}

	static VOID InitSpeedHack() {

	}

	static BOOL SpeedHack(double speed) {
		g_MainSpeed.setSpeed(NtGetTickCount_original(), speed);
		g_MainSpeed64.setSpeed(NtGetTickCount64_original(), speed);
		LARGE_INTEGER lpPerformanceCount;
		NtQueryPerformanceCounter_original(&lpPerformanceCount);
		g_MainSpeed_Long.setSpeed(lpPerformanceCount.QuadPart, speed);
		return TRUE;
	}
}

void DrawWaterMark(ImGuiWindow& windowshit, std::string str, ImVec2 loc, ImU32 colr, bool centered = false)
{
	ImVec2 size = { 0,0 };
	float minx = 0;
	float miny = 0;
	if (centered)
	{
		size = ImGui::GetFont()->CalcTextSizeA(windowshit.DrawList->_Data->FontSize, 0x7FFFF, 0, str.c_str());
		minx = size.x / 2.f;
		miny = size.y / 2.f;
	}

	windowshit.DrawList->AddText(ImVec2((loc.x - 1) - minx, (loc.y - 1) - miny), ImGui::GetColorU32({ 0.f, 0.f, 0.f, 1.f }), str.c_str());
	windowshit.DrawList->AddText(ImVec2((loc.x + 1) - minx, (loc.y + 1) - miny), ImGui::GetColorU32({ 0.f, 0.f, 0.f, 1.f }), str.c_str());
	windowshit.DrawList->AddText(ImVec2((loc.x + 1) - minx, (loc.y - 1) - miny), ImGui::GetColorU32({ 0.f, 0.f, 0.f, 1.f }), str.c_str());
	windowshit.DrawList->AddText(ImVec2((loc.x - 1) - minx, (loc.y + 1) - miny), ImGui::GetColorU32({ 0.f, 0.f, 0.f, 1.f }), str.c_str());
	windowshit.DrawList->AddText(ImVec2(loc.x - minx, loc.y - miny), colr, str.c_str());
}






uintptr_t TargetPawn = 0;


ID3D11Device* device = nullptr;
ID3D11DeviceContext* immediateContext = nullptr;
ID3D11RenderTargetView* renderTargetView = nullptr;
ID3D11Device* pDevice = NULL;
ID3D11DeviceContext* pContext = NULL;



WNDPROC oWndProc = NULL;


typedef HRESULT(*present_fn)(IDXGISwapChain*, UINT, UINT);
inline present_fn present_original{ };

typedef HRESULT(*resize_fn)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
inline resize_fn resize_original{ };

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void Draw2DBoundingBox(Vector3 StartBoxLoc, float flWidth, float Height, ImColor color)
{
	StartBoxLoc.x = StartBoxLoc.x - (flWidth / 2);
	ImDrawList* Renderer = ImGui::GetOverlayDrawList();
	Renderer->AddLine(ImVec2(StartBoxLoc.x, StartBoxLoc.y), ImVec2(StartBoxLoc.x + flWidth, StartBoxLoc.y), color, 1); //bottom
	Renderer->AddLine(ImVec2(StartBoxLoc.x, StartBoxLoc.y), ImVec2(StartBoxLoc.x, StartBoxLoc.y + Height), color, 1); //left
	Renderer->AddLine(ImVec2(StartBoxLoc.x + flWidth, StartBoxLoc.y), ImVec2(StartBoxLoc.x + flWidth, StartBoxLoc.y + Height), color, 1); //right
	Renderer->AddLine(ImVec2(StartBoxLoc.x, StartBoxLoc.y + Height), ImVec2(StartBoxLoc.x + flWidth, StartBoxLoc.y + Height), color, 1); //up
}

void DrawCorneredBox(int X, int Y, int W, int H, const ImU32& color, int thickness) {

	float lineW = (W / 3);
	float lineH = (H / 3);

	//Corners
	ImGui::GetOverlayDrawList()->AddLine(ImVec2(X, Y), ImVec2(X, Y + lineH), ImGui::GetColorU32(color), thickness);
	ImGui::GetOverlayDrawList()->AddLine(ImVec2(X, Y), ImVec2(X + lineW, Y), ImGui::GetColorU32(color), thickness);
	ImGui::GetOverlayDrawList()->AddLine(ImVec2(X + W - lineW, Y), ImVec2(X + W, Y), ImGui::GetColorU32(color), thickness);
	ImGui::GetOverlayDrawList()->AddLine(ImVec2(X + W, Y), ImVec2(X + W, Y + lineH), ImGui::GetColorU32(color), thickness);
	ImGui::GetOverlayDrawList()->AddLine(ImVec2(X, Y + H - lineH), ImVec2(X, Y + H), ImGui::GetColorU32(color), thickness);
	ImGui::GetOverlayDrawList()->AddLine(ImVec2(X, Y + H), ImVec2(X + lineW, Y + H), ImGui::GetColorU32(color), thickness);
	ImGui::GetOverlayDrawList()->AddLine(ImVec2(X + W - lineW, Y + H), ImVec2(X + W, Y + H), ImGui::GetColorU32(color), thickness);
	ImGui::GetOverlayDrawList()->AddLine(ImVec2(X + W, Y + H - lineH), ImVec2(X + W, Y + H), ImGui::GetColorU32(color), thickness);
}

bool IsAiming()
{
	return InvadeKeySync(VK_RBUTTON);
}

auto GetSyscallIndex(std::string ModuleName, std::string SyscallFunctionName, void* Function) -> bool
{
	auto ModuleBaseAddress = LI_FN(GetModuleHandleA)(ModuleName.c_str());
	if (!ModuleBaseAddress)
		ModuleBaseAddress = LI_FN(LoadLibraryA)(ModuleName.c_str());
	if (!ModuleBaseAddress)
		return false;

	auto GetFunctionAddress = LI_FN(GetProcAddress)(ModuleBaseAddress, SyscallFunctionName.c_str());
	if (!GetFunctionAddress)
		return false;

	auto SyscallIndex = *(DWORD*)((PBYTE)GetFunctionAddress + 4);

	*(DWORD*)((PBYTE)Function + 4) = SyscallIndex;

	return true;
}

extern "C"
{
	NTSTATUS _NtUserSendInput(UINT a1, LPINPUT Input, int Size);
};

VOID mouse_event_(DWORD dwFlags, DWORD dx, DWORD dy, DWORD dwData, ULONG_PTR dwExtraInfo)
{
	static bool doneonce;
	if (!doneonce)
	{
		if (!GetSyscallIndex(xorstr("win32u.dll"), xorstr("NtUserSendInput"), _NtUserSendInput))
			return;
		doneonce = true;
	}

	INPUT Input[3] = { 0 };
	Input[0].type = INPUT_MOUSE;
	Input[0].mi.dx = dx;
	Input[0].mi.dy = dy;
	Input[0].mi.mouseData = dwData;
	Input[0].mi.dwFlags = dwFlags;
	Input[0].mi.time = 0;
	Input[0].mi.dwExtraInfo = dwExtraInfo;

	_NtUserSendInput((UINT)1, (LPINPUT)&Input, (INT)sizeof(INPUT));
}


Vector3 head2, neck, pelvis, chest, leftShoulder, rightShoulder, leftElbow, rightElbow, leftHand, rightHand, leftLeg, rightLeg, leftThigh, rightThigh, leftFoot, rightFoot, leftFeet, rightFeet, leftFeetFinger, rightFeetFinger;

bool GetAllBones(uintptr_t CurrentActor) {


	Vector3 chesti, chestatright;

	SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 68, &head2);//
	SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 66, &neck);//
	SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 2, &pelvis);//
	SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 37, &chesti); //
	SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 8, &chestatright);//
	SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 38, &leftShoulder);//
	SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 9, &rightShoulder);//
	SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 39, &leftElbow); //
	SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 10, &rightElbow);//
	SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 62, &leftHand);//
	SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 33, &rightHand);//
	SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 78, &leftLeg); //
	SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 71, &rightLeg); //
	SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 84, &leftThigh); //
	SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 77, &rightThigh); //
	SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 81, &leftFoot); //
	SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 73, &rightFoot);//
	SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 82, &leftFeet); //
	SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 86, &rightFeet); //
	SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 83, &leftFeetFinger); //
	SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 76, &rightFeetFinger); //





	SDK::Classes::AController::WorldToScreen(head2, &head2);
	SDK::Classes::AController::WorldToScreen(neck, &neck);
	SDK::Classes::AController::WorldToScreen(pelvis, &pelvis);
	SDK::Classes::AController::WorldToScreen(chesti, &chesti);
	SDK::Classes::AController::WorldToScreen(chestatright, &chestatright);
	SDK::Classes::AController::WorldToScreen(leftShoulder, &leftShoulder);
	SDK::Classes::AController::WorldToScreen(rightShoulder, &rightShoulder);
	SDK::Classes::AController::WorldToScreen(leftElbow, &leftElbow);
	SDK::Classes::AController::WorldToScreen(rightElbow, &rightElbow);
	SDK::Classes::AController::WorldToScreen(leftHand, &leftHand);
	SDK::Classes::AController::WorldToScreen(rightHand, &rightHand);
	SDK::Classes::AController::WorldToScreen(leftLeg, &leftLeg);
	SDK::Classes::AController::WorldToScreen(rightLeg, &rightLeg);
	SDK::Classes::AController::WorldToScreen(leftThigh, &leftThigh);
	SDK::Classes::AController::WorldToScreen(rightThigh, &rightThigh);
	SDK::Classes::AController::WorldToScreen(leftFoot, &leftFoot);
	SDK::Classes::AController::WorldToScreen(rightFoot, &rightFoot);
	SDK::Classes::AController::WorldToScreen(leftFeet, &leftFeet);
	SDK::Classes::AController::WorldToScreen(rightFeet, &rightFeet);
	SDK::Classes::AController::WorldToScreen(leftFeetFinger, &leftFeetFinger);
	SDK::Classes::AController::WorldToScreen(rightFeetFinger, &rightFeetFinger);

	chest.x = chesti.x + ((chestatright.x - chesti.x) / 2);
	chest.y = chesti.y;

	return true;
}

// Simple Fov Fix lmao

bool InFov(uintptr_t CurrentPawn, int FovValue) {
	Vector3 HeadPos; SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentPawn, 68, &HeadPos); SDK::Classes::AController::WorldToScreen(HeadPos, &HeadPos);
	auto dx = HeadPos.x - (Renderer_Defines::Width / 2);
	auto dy = HeadPos.y - (Renderer_Defines::Height / 2);
	auto dist = sqrtf(dx * dx + dy * dy);

	if (dist < FovValue) {
		return true;
	}
	else {
		return false;
	}
}
inline float custom_sqrtf(float _X)
{
	return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(_X)));
}

inline float custom_sinf(float _X)
{
	return _mm_cvtss_f32(_mm_sin_ps(_mm_set_ss(_X)));
}

inline float custom_cosf(float _X)
{
	return _mm_cvtss_f32(_mm_cos_ps(_mm_set_ss(_X)));
}

inline float custom_acosf(float _X)
{
	return _mm_cvtss_f32(_mm_acos_ps(_mm_set_ss(_X)));
}

inline float custom_tanf(float _X)
{
	return _mm_cvtss_f32(_mm_tan_ps(_mm_set_ss(_X)));
}

inline float custom_atan2f(float _X, float _Y)
{
	return _mm_cvtss_f32(_mm_atan2_ps(_mm_set_ss(_X), _mm_set_ss(_Y)));
}

inline int custom_compare(const char* X, const char* Y)
{
	while (*X && *Y) {
		if (*X != *Y) {
			return 0;
		}
		X++;
		Y++;
	}

	return (*Y == '\0');
}

inline int custom_wcompare(const wchar_t* X, const wchar_t* Y)
{
	while (*X && *Y) {
		if (*X != *Y) {
			return 0;
		}
		X++;
		Y++;
	}

	return (*Y == L'\0');
}

inline const wchar_t* custom_wcsstr(const wchar_t* X, const wchar_t* Y)
{
	while (*X != L'\0') {
		if ((*X == *Y) && custom_wcompare(X, Y)) {
			return X;
		}
		X++;
	}
	return NULL;
}

inline const char* custom_strstr(const char* X, const char* Y)
{
	while (*X != '\0') {
		if ((*X == *Y) && custom_compare(X, Y)) {
			return X;
		}
		X++;
	}
	return NULL;
}

inline int custom_strlen(const char* string)
{
	int cnt = 0;
	if (string)
	{
		for (; *string != 0; ++string) ++cnt;
	}
	return cnt;
}

inline int custom_wcslen(const wchar_t* string)
{
	int cnt = 0;
	if (string)
	{
		for (; *string != 0; ++string) ++cnt;
	}
	return cnt;
}

struct APlayerCameraManager_GetCameraLocation_Params
{
	Vector3                                     ReturnValue;                                              // (Parm, OutParm, ZeroConstructor, ReturnParm, IsPlainOldData)
};
Vector3 GetCameraLocation(uintptr_t PlayerCameraManager)
{
	static PVOID AAA = 0;
	if (!AAA)
		AAA = FN::FindObject("GetCameraLocation");

	APlayerCameraManager_GetCameraLocation_Params params;

	FN::ProcessEvent(PlayerCameraManager, AAA, &params);

	return params.ReturnValue;
}

struct APlayerCameraManager_GetCameraRotation_Params
{
	Vector3                                     ReturnValue;                                              // (Parm, OutParm, ZeroConstructor, ReturnParm, IsPlainOldData)
};
Vector3 GetCameraRotation(uintptr_t PlayerCameraManager)
{
	static PVOID AAA = 0;
	if (!AAA)
		AAA = FN::FindObject("GetCameraRotation");

	APlayerCameraManager_GetCameraLocation_Params params;

	FN::ProcessEvent(PlayerCameraManager, AAA, &params);

	return params.ReturnValue;
}

struct FVector
{
	float                                              X;                                                        // 0x0000(0x0004) (Edit, BlueprintVisible, ZeroConstructor, SaveGame, IsPlainOldData)
	float                                              Y;                                                        // 0x0004(0x0004) (Edit, BlueprintVisible, ZeroConstructor, SaveGame, IsPlainOldData)
	float                                              Z;                                                        // 0x0008(0x0004) (Edit, BlueprintVisible, ZeroConstructor, SaveGame, IsPlainOldData)

	inline FVector()
		: X(0), Y(0), Z(0)
	{ }

	inline FVector(float x, float y, float z)
		: X(x),
		Y(y),
		Z(z)
	{ }

	__forceinline FVector operator-(const FVector& V) {
		return FVector(X - V.X, Y - V.Y, Z - V.Z);
	}

	__forceinline FVector operator+(const FVector& V) {
		return FVector(X + V.X, Y + V.Y, Z + V.Z);
	}

	__forceinline FVector operator*(float Scale) const {
		return FVector(X * Scale, Y * Scale, Z * Scale);
	}

	__forceinline FVector operator/(float Scale) const {
		const float RScale = 1.f / Scale;
		return FVector(X * RScale, Y * RScale, Z * RScale);
	}

	__forceinline FVector operator+(float A) const {
		return FVector(X + A, Y + A, Z + A);
	}

	__forceinline FVector operator-(float A) const {
		return FVector(X - A, Y - A, Z - A);
	}

	__forceinline FVector operator*(const FVector& V) const {
		return FVector(X * V.X, Y * V.Y, Z * V.Z);
	}

	__forceinline FVector operator/(const FVector& V) const {
		return FVector(X / V.X, Y / V.Y, Z / V.Z);
	}

	__forceinline float operator|(const FVector& V) const {
		return X * V.X + Y * V.Y + Z * V.Z;
	}

	__forceinline float operator^(const FVector& V) const {
		return X * V.Y - Y * V.X - Z * V.Z;
	}

	__forceinline FVector& operator+=(const FVector& v) {
		(*this);
		(v);
		X += v.X;
		Y += v.Y;
		Z += v.Z;
		return *this;
	}

	__forceinline FVector& operator-=(const FVector& v) {
		(*this);
		(v);
		X -= v.X;
		Y -= v.Y;
		Z -= v.Z;
		return *this;
	}

	__forceinline FVector& operator*=(const FVector& v) {
		(*this);
		(v);
		X *= v.X;
		Y *= v.Y;
		Z *= v.Z;
		return *this;
	}

	__forceinline FVector& operator/=(const FVector& v) {
		(*this);
		(v);
		X /= v.X;
		Y /= v.Y;
		Z /= v.Z;
		return *this;
	}

	__forceinline bool operator==(const FVector& src) const {
		(src);
		(*this);
		return (src.X == X) && (src.Y == Y) && (src.Z == Z);
	}

	__forceinline bool operator!=(const FVector& src) const {
		(src);
		(*this);
		return (src.X != X) || (src.Y != Y) || (src.Z != Z);
	}

	__forceinline float Size() const {
		return sqrt(X * X + Y * Y + Z * Z);
	}

	__forceinline float Size2D() const {
		return sqrt(X * X + Y * Y);
	}

	__forceinline float SizeSquared() const {
		return X * X + Y * Y + Z * Z;
	}

	__forceinline float SizeSquared2D() const {
		return X * X + Y * Y;
	}

	__forceinline float Dot(const FVector& vOther) const {
		const FVector& a = *this;

		return (a.X * vOther.X + a.Y * vOther.Y + a.Z * vOther.Z);
	}

	__forceinline float DistanceFrom(const FVector& Other) const {
		const FVector& a = *this;
		float dx = (a.X - Other.X);
		float dy = (a.Y - Other.Y);
		float dz = (a.Z - Other.Z);

		return (sqrt((dx * dx) + (dy * dy) + (dz * dz)));
	}

	__forceinline FVector Normalize() {
		FVector vector;
		float length = this->Size();

		if (length != 0) {
			vector.X = X / length;
			vector.Y = Y / length;
			vector.Z = Z / length;
		}
		else
			vector.X = vector.Y = 0.0f;
		vector.Z = 1.0f;

		return vector;
	}

	__forceinline float Distance(FVector v) {
		return float(sqrtf(powf(v.X - X, 2.0) + powf(v.Y - Y, 2.0) + powf(v.Z - Z, 2.0)));
	}


	inline float distance() {
		return sqrtf(this->X * this->X + this->Y * this->Y + this->Z * this->Z);
	}

	static FVector CalcAngle(FVector vCameraPos, FVector vAimPos)
	{
		FVector vecDelta = vCameraPos - vAimPos;
		float hyp = sqrtf((vecDelta.X * vecDelta.X + vecDelta.X * vecDelta.Y));

		FVector ViewAngles;
		ViewAngles.X = -atanf((vecDelta.Z / hyp)) * (180.0f / 3.141592653589793f);
		ViewAngles.Y = atanf((vecDelta.Y / vecDelta.X)) * (180.0f / 3.141592653589793f);
		ViewAngles.Z = 0.0f;

		if (vecDelta.X >= 0.0f)
			ViewAngles.Y += 180.0f;

		return ViewAngles;
	}

	FVector ClampAngles()
	{
		FVector angles = FVector(X, Y, Z);

		if (angles.Y < -180.0f)
			angles.Y += 360.0f;

		if (angles.Y > 180.0f)
			angles.Y -= 360.0f;

		if (angles.X < -74.0f)
			angles.X = -74.0f;

		if (angles.X > 74.0f)
			angles.X = 74.0f;

		return angles;
	}
};

Vector3 calcangle(Vector3& zaz, Vector3& daz) {

	Vector3 dalte = zaz - daz;
	Vector3 ongle;
	float hpm = sqrtf(dalte.x * dalte.x + dalte.y * dalte.y);
	ongle.y = atan(dalte.y / dalte.x) * 57.295779513082f;
	ongle.x = (atan(dalte.z / hpm) * 57.295779513082f) * -1.f;
	if (dalte.x >= 0.f) ongle.y += 180.f;
	return ongle;
}
class color
{
public:
	float R, G, B, A;

	color()
	{
		R = G = B = A = 0;
	}

	color(float R, float G, float B, float A)
	{
		this->R = R;
		this->G = G;
		this->B = B;
		this->A = A;
	}
};
struct FLinearColor
{
	float R;
	float G;
	float B;
	float A;
};
struct FPawnHighlight
{
	float Priority_28_E2E1B5344846E187B9C11B863A7F0698;
	FLinearColor Inner_21_4CC2801147EA190DE16F59B34F36853E;
	FLinearColor Outer_22_5A1D7D0543D303E8B54B66A7F7BD2E2E;
	float FresnelBrightness_23_52B0F96447FF640F47DF2895B0602E92;
	float FresnelExponent_24_B427CF0C441AA37ED49833BF7579DE6D;
	float UsesPulse_25_E29229F64E540F0617E4C4987AD77605;
};
bool IsDead(uintptr_t actor)
{
	if (!valid_pointer((uintptr_t)actor)) return false;

	struct
	{
		bool ReturnValue;
	} params;

	FN::ProcessEvent(uintptr_t(actor), ObjectsAddresses::IsDead, &params);

	return params.ReturnValue;
}

void __forceinline AnsiToWide(char* inAnsi, wchar_t* outWide)
{
	int i = 0;
	for (; inAnsi[i] != '\0'; i++)
		outWide[i] = (wchar_t)(inAnsi)[i];
	outWide[i] = L'\0';
}

void ApplyPawnHighlight(uintptr_t actor, color InnerCol, color OuterCol)
{

	struct APlayerPawn_Athena_C_ApplyPawnHighlight_Params
	{
		uintptr_t Source;                                                   
		FPawnHighlight HitGlow;                                                  
	} params;

	FPawnHighlight HitGlow;

	HitGlow.FresnelBrightness_23_52B0F96447FF640F47DF2895B0602E92 = 0.f;
	HitGlow.FresnelExponent_24_B427CF0C441AA37ED49833BF7579DE6D = 0;
	HitGlow.Inner_21_4CC2801147EA190DE16F59B34F36853E = { (float)InnerCol.R, (float)InnerCol.G, (float)InnerCol.B, (float)InnerCol.A };
	HitGlow.Outer_22_5A1D7D0543D303E8B54B66A7F7BD2E2E = { (float)OuterCol.R, (float)OuterCol.G, (float)OuterCol.B, (float)OuterCol.A };
	HitGlow.Priority_28_E2E1B5344846E187B9C11B863A7F0698 = 0.f;
	HitGlow.UsesPulse_25_E29229F64E540F0617E4C4987AD77605 = 0.f;


	params.Source = uintptr_t(actor);
	params.HitGlow = HitGlow;

	FN::ProcessEvent(uintptr_t(actor), ObjectsAddresses::ApplyPawnHighlight, &params);

}

static void RotateThatAssBby(uintptr_t PlayerController, Vector3 NewRotation, bool bResetCamera = false)
{
	auto SetControlRotation_ = (*(void(__fastcall**)(uintptr_t Controller, Vector3 NewRotation, bool bResetCamera))(*(uintptr_t*)PlayerController + 0x720));
	SetControlRotation_(PlayerController, NewRotation, bResetCamera);
}

FLOAT FovAngle;

Vector3 AimW2S;
Vector3 Aim;

bool EntitiyLoop()
{


	ImDrawList* Renderer = ImGui::GetOverlayDrawList();


	if (Settings::Crosshair)
	{
		Renderer->AddLine(ImVec2(Renderer_Defines::Width / 2 - 7, Renderer_Defines::Height / 2), ImVec2(Renderer_Defines::Width / 2 + 1, Renderer_Defines::Height / 2), ImColor(255, 0, 0, 255), 1.0);
		Renderer->AddLine(ImVec2(Renderer_Defines::Width / 2 + 8, Renderer_Defines::Height / 2), ImVec2(Renderer_Defines::Width / 2 + 1, Renderer_Defines::Height / 2), ImColor(255, 0, 0, 255), 1.0);
		Renderer->AddLine(ImVec2(Renderer_Defines::Width / 2, Renderer_Defines::Height / 2 - 7), ImVec2(Renderer_Defines::Width / 2, Renderer_Defines::Height / 2), ImColor(255, 0, 0, 255), 1.0);
		Renderer->AddLine(ImVec2(Renderer_Defines::Width / 2, Renderer_Defines::Height / 2 + 8), ImVec2(Renderer_Defines::Width / 2, Renderer_Defines::Height / 2), ImColor(255, 0, 0, 255), 1.0);
	}

	if (Settings::ShowFovCircle)
		Renderer->AddCircle(ImVec2(Renderer_Defines::Width / 2, Renderer_Defines::Height / 2), Settings::FovCircle_Value, ImColor(255, 0, 0, 255), 124);


	try
	{
		float FOVmax = 9999.f;

		float OldDistance = 0x7FFF;

		float closestDistance = FLT_MAX;
		uintptr_t closestPawn = NULL;
		bool closestPawnVisible = false;


		uintptr_t MyTeamIndex = 0, EnemyTeamIndex = 0;
		uintptr_t GWorld = read<uintptr_t>(UWorld); if (!GWorld) return false;

		uintptr_t Gameinstance = read<uint64_t>(GWorld + StaticOffsets::OwningGameInstance); if (!Gameinstance) return false;

		uintptr_t LocalPlayers = read<uint64_t>(Gameinstance + StaticOffsets::LocalPlayers); if (!LocalPlayers) return false;

		uintptr_t LocalPlayer = read<uint64_t>(LocalPlayers); if (!LocalPlayer) return false;

		PlayerController = read<uint64_t>(LocalPlayer + StaticOffsets::PlayerController); if (!PlayerController) return false;

		uintptr_t PlayerCameraManager = read<uint64_t>(PlayerController + StaticOffsets::PlayerCameraManager); if (!PlayerCameraManager) return false;

		LocalPawn = read<uint64_t>(PlayerController + StaticOffsets::AcknowledgedPawn);

		uintptr_t Ulevel = read<uintptr_t>(GWorld + StaticOffsets::PersistentLevel); if (!Ulevel) return false;

		uintptr_t AActors = read<uintptr_t>(Ulevel + StaticOffsets::AActors); if (!AActors) return false;

		uintptr_t ActorCount = read<int>(Ulevel + StaticOffsets::ActorCount); if (!ActorCount) return false;


		uintptr_t LocalRootComponent;
		Vector3 LocalRelativeLocation;

		if (valid_pointer(LocalPawn)) {
			LocalRootComponent = read<uintptr_t>(LocalPawn + 0x190);
			LocalRelativeLocation = read<Vector3>(LocalRootComponent + 0x128);
		}

		for (int i = 0; i < ActorCount; i++) {
			
			auto CurrentActor = read<uintptr_t>(AActors + i * sizeof(uintptr_t));

			auto name = SDK::Classes::UObject::GetObjectName(CurrentActor);

			bool IsVisible = false;

			if (valid_pointer(LocalPawn))
			{

				if (strstr(name, xorstr("PlayerPawn"))) {
					
					ImColor ESPColor = { 255, 255, 255, 255 };

					Vector3 HeadPos, Headbox, bottom;

					Vector3 HeadLoc;

					SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 68, &HeadPos);
					SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 0, &bottom);

					SDK::Classes::AController::WorldToScreen(Vector3(HeadPos.x, HeadPos.y, HeadPos.z + 20), &Headbox);
					SDK::Classes::AController::WorldToScreen(bottom, &bottom);

					SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 68, &HeadLoc);

					if (Headbox.x == 0 && Headbox.y == 0) continue;
					if (bottom.x == 0 && bottom.y == 0) continue;
					Vector3 udhead;
					Vector3 udbox;
					SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 68, &udbox);
					
					SDK::Classes::AController::WorldToScreen(udbox, &udhead);

					


					uintptr_t MyState = read<uintptr_t>(LocalPawn + StaticOffsets::PlayerState);
					if (!MyState) continue;

					MyTeamIndex = read<uintptr_t>(MyState + StaticOffsets::TeamIndex);
					if (!MyTeamIndex) continue;

					uintptr_t EnemyState = read<uintptr_t>(CurrentActor + StaticOffsets::PlayerState);
					if (!EnemyState) continue;

					EnemyTeamIndex = read<uintptr_t>(EnemyState + StaticOffsets::TeamIndex);
					if (!EnemyTeamIndex) continue;

					
					
					
					if (CurrentActor == LocalPawn) continue;

					Vector3 viewPoint;


					auto x = HeadPos.x - (GetSystemMetrics(0) / 2);
					auto y = HeadPos.y - (GetSystemMetrics(1) / 2);
					auto distance = sqrt(x * x + y * y);
					if (distance < Settings::FovCircle_Value * GetSystemMetrics(0) / 2 / FovAngle / 2 && distance < OldDistance)
					{
						SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 68, &Aim);
						SDK::Classes::AController::WorldToScreen(Aim, &AimW2S);
						OldDistance = distance;
					}

					if (Settings::VisibleCheck) {
						
						IsVisible = true;
					}


					if (Settings::SnapLines) {
						ImColor col;
						col = ImColor(255, 0, 0, 255);


						Vector3 LocalPelvis;
						SDK::Classes::USkeletalMeshComponent::GetBoneLocation(LocalPawn, 2, &LocalPelvis);
						SDK::Classes::AController::WorldToScreen(LocalPelvis, &LocalPelvis);

						Renderer->AddLine(ImVec2(LocalPelvis.x, LocalPelvis.y), ImVec2(pelvis.x, pelvis.y), col, 1.f);
					}
					

					if (Settings::Aim and IsAiming() and (MyTeamIndex != EnemyTeamIndex))
					{

						auto camerlocation = GetCameraLocation(PlayerCameraManager);
						auto camerrotation = GetCameraRotation(PlayerCameraManager);

						Vector3 VectorPos = udbox - camerlocation;
						float distance = VectorPos.Length();

						Vector3 RetVector;

						RetVector.x = -((acosf(VectorPos.z / distance) * (float)(180.f / M_PI)) - 90.f);
						RetVector.y = atan2f(VectorPos.y, VectorPos.x) * (float)(180.f / M_PI);

						if (RetVector.x == 0 and RetVector.y == 0) continue;

						RetVector.x = (RetVector.x - camerrotation.x) / 1.0f + Settings::smooth + camerrotation.x;
						RetVector.y = (RetVector.y - camerrotation.y) / 1.0f + Settings::smooth + camerrotation.y;

						if (GetAsyncKeyState(VK_RBUTTON))
							RotateThatAssBby(PlayerController, RetVector, false);

						if (Settings::TargetLine)
						{
							ImGui::GetOverlayDrawList()->AddLine(ImVec2(GetSystemMetrics(0) / 2, GetSystemMetrics(1) / 2), ImVec2(RetVector.x, RetVector.y), ESPColor);
						}


					}
				
				}
			}

			Vector2 calculation;


			if (strstr(name, xorstr("BP_PlayerPawn")) || strstr(name, xorstr("PlayerPawn")))
			{

				if (SDK::Utils::CheckInScreen(CurrentActor, Renderer_Defines::Width, Renderer_Defines::Height)) {

					Vector3 HeadPos, Headbox, bottom;

					SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 68, &HeadPos);
					SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 0, &bottom);

					SDK::Classes::AController::WorldToScreen(Vector3(HeadPos.x, HeadPos.y, HeadPos.z + 20), &Headbox);
					SDK::Classes::AController::WorldToScreen(bottom, &bottom);

					Vector3 HeadLoc;
					Vector3 BottomBone;
					Vector3 BottomPostition;

					SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 68, &HeadLoc);

					SDK::Classes::USkeletalMeshComponent::GetBoneLocation(CurrentActor, 0, &BottomBone);
					SDK::Classes::AController::WorldToScreen(BottomPostition, &BottomBone);


					if (Settings::Box or Settings::Skeleton) {
						GetAllBones(CurrentActor);
					}

					
					int array[20] = { head2.x, neck.x, pelvis.x, chest.x, leftShoulder.x, rightShoulder.x, leftElbow.x, rightElbow.x, leftHand.x, rightHand.x, leftLeg.x, rightLeg.x, leftThigh.x, rightThigh.x, leftFoot.x, rightFoot.x, leftFeet.x, rightFeet.x, leftFeetFinger.x, rightFeetFinger.x };
					int MostRightBone = array[0];
					int MostLeftBone = array[0];

					for (int mostrighti = 0; mostrighti < 20; mostrighti++)
					{
						if (array[mostrighti] > MostRightBone)
							MostRightBone = array[mostrighti];
					}

					for (int mostlefti = 0; mostlefti < 20; mostlefti++)
					{
						if (array[mostlefti] < MostLeftBone)
							MostLeftBone = array[mostlefti];
					}



					float ActorHeight = (Headbox.y - bottom.y);
					if (ActorHeight < 0) ActorHeight = ActorHeight * (-1.f);

					int ActorWidth = MostRightBone - MostLeftBone;

				

					if (Settings::Skeleton)
					{

						ImColor VisualColor;
						VisualColor = ImColor(255, 255, 255, 255);

						Renderer->AddLine(ImVec2(head2.x, head2.y), ImVec2(neck.x, neck.y), VisualColor, 1.f);
						Renderer->AddLine(ImVec2(neck.x, neck.y), ImVec2(chest.x, chest.y), VisualColor, 1.f);
						Renderer->AddLine(ImVec2(chest.x, chest.y), ImVec2(pelvis.x, pelvis.y), VisualColor, 1.f);
						Renderer->AddLine(ImVec2(chest.x, chest.y), ImVec2(leftShoulder.x, leftShoulder.y), VisualColor, 1.f);
						Renderer->AddLine(ImVec2(chest.x, chest.y), ImVec2(rightShoulder.x, rightShoulder.y), VisualColor, 1.f);
						Renderer->AddLine(ImVec2(leftShoulder.x, leftShoulder.y), ImVec2(leftElbow.x, leftElbow.y), VisualColor, 1.f);
						Renderer->AddLine(ImVec2(rightShoulder.x, rightShoulder.y), ImVec2(rightElbow.x, rightElbow.y), VisualColor, 1.f);
						Renderer->AddLine(ImVec2(leftElbow.x, leftElbow.y), ImVec2(leftHand.x, leftHand.y), VisualColor, 1.f);
						Renderer->AddLine(ImVec2(rightElbow.x, rightElbow.y), ImVec2(rightHand.x, rightHand.y), VisualColor, 1.f);
						Renderer->AddLine(ImVec2(pelvis.x, pelvis.y), ImVec2(leftLeg.x, leftLeg.y), VisualColor, 1.f);
						Renderer->AddLine(ImVec2(pelvis.x, pelvis.y), ImVec2(rightLeg.x, rightLeg.y), VisualColor, 1.f);
						Renderer->AddLine(ImVec2(leftLeg.x, leftLeg.y), ImVec2(leftThigh.x, leftThigh.y), VisualColor, 1.f);
						Renderer->AddLine(ImVec2(rightLeg.x, rightLeg.y), ImVec2(rightThigh.x, rightThigh.y), VisualColor, 1.f);
						Renderer->AddLine(ImVec2(leftThigh.x, leftThigh.y), ImVec2(leftFoot.x, leftFoot.y), VisualColor, 1.f);
						Renderer->AddLine(ImVec2(rightThigh.x, rightThigh.y), ImVec2(rightFoot.x, rightFoot.y), VisualColor, 1.f);
						Renderer->AddLine(ImVec2(leftFoot.x, leftFoot.y), ImVec2(leftFeet.x, leftFeet.y), VisualColor, 1.f);
						Renderer->AddLine(ImVec2(rightFoot.x, rightFoot.y), ImVec2(rightFeet.x, rightFeet.y), VisualColor, 1.f);
						Renderer->AddLine(ImVec2(leftFeet.x, leftFeet.y), ImVec2(leftFeetFinger.x, leftFeetFinger.y), VisualColor, 1.f);
						Renderer->AddLine(ImVec2(rightFeet.x, rightFeet.y), ImVec2(rightFeetFinger.x, rightFeetFinger.y), VisualColor, 1.f);
					}
					

					if (Settings::Box && BoxModePosition == 0)
					{
						Draw2DBoundingBox(Headbox, ActorWidth, ActorHeight, ImColor(255, 255, 255, 255));

					}
					else if (Settings::Box && BoxModePosition == 1)
					{

						DrawCorneredBox(Headbox.x - (ActorWidth / 2), Headbox.y, ActorWidth, ActorHeight, ImColor(255, 255, 255, 255), 1.5);

					}


				}




			}
			
		}
	}
	catch (...) {}


}

namespace ImGui
{
	IMGUI_API bool Tab(unsigned int index, const char* label, int* selected, float width = 117, float height = 25)
	{
		ImGuiStyle& style = ImGui::GetStyle();
		ImColor color = ImColor(27, 26, 35, 255)/*style.Colors[ImGuiCol_Button]*/;
		ImColor colorActive = ImColor(79, 79, 81, 255); /*style.Colors[ImGuiCol_ButtonActive]*/;
		ImColor colorHover = ImColor(62, 62, 66, 255)/*style.Colors[ImGuiCol_ButtonHovered]*/;


		if (index > 0)
			ImGui::SameLine();

		if (index == *selected)
		{
			style.Colors[ImGuiCol_Button] = colorActive;
			style.Colors[ImGuiCol_ButtonActive] = colorActive;
			style.Colors[ImGuiCol_ButtonHovered] = colorActive;
		}
		else
		{
			style.Colors[ImGuiCol_Button] = color;
			style.Colors[ImGuiCol_ButtonActive] = colorActive;
			style.Colors[ImGuiCol_ButtonHovered] = colorHover;
		}

		if (ImGui::Button(label, ImVec2(width, height)))
			*selected = index;

		style.Colors[ImGuiCol_Button] = color;
		style.Colors[ImGuiCol_ButtonActive] = colorActive;
		style.Colors[ImGuiCol_ButtonHovered] = colorHover;

		return *selected == index;
	}
}

ImGuiWindow& CreateScene() {
	ImGui_ImplDX11_NewFrame();
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
	ImGui::Begin(xorstr("##mainscenee"), nullptr, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar);

	auto& io = ImGui::GetIO();
	ImGui::SetWindowPos(ImVec2(0, 0), ImGuiCond_Always);
	ImGui::SetWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y), ImGuiCond_Always);

	return *ImGui::GetCurrentWindow();
}

static int Tab = 0;

VOID MenuAndDestroy(ImGuiWindow& window) {

	ColorChange();

	window.DrawList->PushClipRectFullScreen();
	ImGui::End();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar(2);

	ImGui::StyleColorsClassic();
	ImGuiStyle* style = &ImGui::GetStyle();


	if (ShowMenu) {


		ImGui::SetNextWindowSize({ 400, 600 });


		ImGui::Begin((""), 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar); {
			ImGui::Separator();
			ImGui::Spacing();

			ImGui::Tab(1, ("Aimbot"), &Tab);
			ImGui::Tab(2, ("Visuals"), &Tab);
			ImGui::Tab(3, ("Exploits"), &Tab);

			ImGui::Separator();

			if (Tab == 0)
			{
				ImGui::Separator;

				ImGui::Separator;
			}

			if (Tab == 1)
			{
				ImGui::Text("Aimbot Options");
				ImGui::Separator;

				ImGui::Checkbox("Memory Aimbot", &Settings::Aim);
				if (Settings::Aim)
				{
					ImGui::SliderFloat("Aimbot Smoothing", &Settings::smooth, 1.5f, 5.0f);

					ImGui::Checkbox("Show FOV Circle", &Settings::ShowFovCircle);
					if (Settings::ShowFovCircle)
					{
						ImGui::SliderFloat("FOV Value", &Settings::FovCircle_Value, 80.0f, 800.0f);
						ImGui::Checkbox("Show Crosshair", &Settings::Crosshair);
					}

				}

			}

			if (Tab == 2)
			{
				ImGui::Text("Visual Options");
				ImGui::Separator;

				ImGui::Checkbox("Box ESP", &Settings::Box);
				if (Settings::Box)
				{
					ImGui::Combo(xorstr("Visual Box Style"), &BoxModePosition, BoxModes, sizeof(BoxModes) / sizeof(*BoxModes));
				}
				ImGui::Checkbox("Draw Line To Aimbot Target", &Settings::TargetLine);
				ImGui::Checkbox("Draw Player Lines", &Settings::SnapLines);
				ImGui::Checkbox("Draw Player Skeleton ESP", &Settings::Skeleton);
			}

			if (Tab == 3)
			{
				ImGui::Text("Exploit Options");
				ImGui::Separator;

			}


			ImGui::End();
		}
	}
	

	ImGui::Render();
}



HRESULT present_hooked(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags)
{
	static float width = 0;
	static float height = 0;
	static HWND hWnd = 0;
	if (!device)
	{

		swapChain->GetDevice(__uuidof(device), reinterpret_cast<PVOID*>(&device));
		device->GetImmediateContext(&immediateContext);

		ID3D11Texture2D* renderTarget = nullptr;
		swapChain->GetBuffer(0, __uuidof(renderTarget), reinterpret_cast<PVOID*>(&renderTarget));
		device->CreateRenderTargetView(renderTarget, nullptr, &renderTargetView);
		renderTarget->Release();

		ID3D11Texture2D* backBuffer = 0;
		swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (PVOID*)&backBuffer);
		D3D11_TEXTURE2D_DESC backBufferDesc = { 0 };
		backBuffer->GetDesc(&backBufferDesc);

		HWND hWnd = LI_FN(FindWindowA)(xorstr("UnrealWindow"), xorstr("Fortnite  "));


		width = (float)backBufferDesc.Width;
		height = (float)backBufferDesc.Height;
		backBuffer->Release();

		ImGui::GetIO().Fonts->AddFontFromFileTTF(xorstr("C:\\Windows\\Fonts\\Bahnschrift.ttf"), 14.0f);

		ImGui_ImplDX11_Init(hWnd, device, immediateContext);
		ImGui_ImplDX11_CreateDeviceObjects();

	}
	immediateContext->OMSetRenderTargets(1, &renderTargetView, nullptr);

	auto& window = CreateScene();

	if (ShowMenu) {
		ImGuiIO& io = ImGui::GetIO();

		POINT p;
		SpoofCall(GetCursorPos, &p);
		io.MousePos.x = p.x;
		io.MousePos.y = p.y;



		if (InvadeKeySync(VK_LBUTTON)) {
			io.MouseDown[0] = true;
			io.MouseClicked[0] = true;
			io.MouseClickedPos[0].x = io.MousePos.x;
			io.MouseClickedPos[0].y = io.MousePos.y;
		}
		else {
			io.MouseDown[0] = false;
		}
	}


	EntitiyLoop();

	if (InvadeKeySync(VK_INSERT) & 1)
	{
		ShowMenu = !ShowMenu;
		Tab == 0;
	}

	MenuAndDestroy(window);

	return SpoofCall(present_original, swapChain, syncInterval, flags);
}



HRESULT resize_hooked(IDXGISwapChain* swapChain, UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat, UINT swapChainFlags) {
	ImGui_ImplDX11_Shutdown();
	renderTargetView->Release();
	immediateContext->Release();
	device->Release();
	device = nullptr;

	return SpoofCall(resize_original, swapChain, bufferCount, width, height, newFormat, swapChainFlags);
}

void RendererDefinesHook()
{
	Renderer_Defines::Width = LI_FN(GetSystemMetrics)(SM_CXSCREEN);
	Renderer_Defines::Height = LI_FN(GetSystemMetrics)(SM_CYSCREEN);
}


bool Initialize()
{
	RendererDefinesHook();

	UWorld = MemoryHelper::PatternScan("48 89 05 ? ? ? ? 48 8B 4B 78");
	UWorld = RVA(UWorld, 7);

	FreeFn = MemoryHelper::PatternScan(xorstr("E8 ? ? ? ? 0F 10 40 68"));

	ProjectWorldToScreen = MemoryHelper::PatternScan(xorstr("40 53 55 56 57 41 56 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 84 24 ? ? ? ? 33 DB 45 8A F1 49 8B F8 48 8B EA 48 8B F1 48 85 C9 0F 84"));

	GetNameByIndex = MemoryHelper::PatternScan(xorstr("48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 54 41 56 41 57 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 84 24 ? ? ? ? 8B 19 48 8B F2"));

	BoneMatrix = MemoryHelper::PatternScan(xorstr("E8 ? ? ? ? 0F 10 40 68"));
	BoneMatrix = RVA(BoneMatrix, 5);

	auto UObjectPtr = MemoryHelper::PatternScan(xorstr("48 8B 05 ? ? ? ? 48 8B 0C C8 48 8B 04 D1"));
	FN::objects = decltype(FN::objects)(RVA(UObjectPtr, 7));

	ObjectsAddresses::FOV = SpoofCall(FN::FindObject, (const char*)xorstr("FOV"));
	ObjectsAddresses::AddYawInput = SpoofCall(FN::FindObject, (const char*)xorstr("AddYawInput"));
	ObjectsAddresses::AddPitchInput = SpoofCall(FN::FindObject, (const char*)xorstr("AddPitchInput"));
	ObjectsAddresses::GetCameraLocation = SpoofCall(FN::FindObject, (const char*)xorstr("GetCameraLocation"));
	ObjectsAddresses::IsDead = SpoofCall(FN::FindObject, (const char*)xorstr("IsDead"));

	InvadeKeySync = (LPFN_MBA)LI_FN(GetProcAddress)(LI_FN(GetModuleHandleA)(xorstr("win32u.dll")), xorstr("NtUserGetAsyncKeyState"));


	auto level = D3D_FEATURE_LEVEL_11_0;
	DXGI_SWAP_CHAIN_DESC sd;
	{
		ZeroMemory(&sd, sizeof sd);
		sd.BufferCount = 1;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.OutputWindow = LI_FN(FindWindowA)(xorstr("UnrealWindow"), xorstr("Fortnite  "));
		sd.SampleDesc.Count = 1;
		sd.Windowed = TRUE;
		sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	}

	IDXGISwapChain* swap_chain = nullptr;
	ID3D11Device* device = nullptr;
	ID3D11DeviceContext* context = nullptr;

	LI_FN(D3D11CreateDeviceAndSwapChain)(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, &level, 1, D3D11_SDK_VERSION, &sd, &swap_chain, &device, nullptr, &context);

	auto* swap_chainvtable = reinterpret_cast<uintptr_t*>(swap_chain);
	swap_chainvtable = reinterpret_cast<uintptr_t*>(swap_chainvtable[0]);

	DWORD old_protect;
	present_original = reinterpret_cast<present_fn>(reinterpret_cast<DWORD_PTR*>(swap_chainvtable[8]));
	LI_FN(VirtualProtect)(swap_chainvtable, 0x2B8, PAGE_EXECUTE_READWRITE, &old_protect);
	swap_chainvtable[8] = reinterpret_cast<DWORD_PTR>(present_hooked);
	LI_FN(VirtualProtect)(swap_chainvtable, 0x2B8, old_protect, &old_protect);

	DWORD old_protect_resize;
	resize_original = reinterpret_cast<resize_fn>(reinterpret_cast<DWORD_PTR*>(swap_chainvtable[13]));
	LI_FN(VirtualProtect)(swap_chainvtable, 0x2B8, PAGE_EXECUTE_READWRITE, &old_protect_resize);
	swap_chainvtable[13] = reinterpret_cast<DWORD_PTR>(resize_hooked);
	LI_FN(VirtualProtect)(swap_chainvtable, 0x2B8, old_protect_resize, &old_protect_resize);
	return true;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:

		Initialize();
	}
	return TRUE;
}
