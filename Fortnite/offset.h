#pragma once
#include <Windows.h>

namespace StaticOffsets {

	uintptr_t OwningGameInstance = 0x1b8; //
	uintptr_t LocalPlayers = 0x38; // Usually The Same
	uintptr_t PlayerController = 0x30; // Usually The Same
	uintptr_t PlayerCameraManager = 0x340; //
	uintptr_t AcknowledgedPawn = 0x330; //
	uintptr_t PrimaryPickupItemEntry = 0x330; //
	uintptr_t ItemDefinition = 0x18; // Usually The Same
	uintptr_t DisplayName = 0x90; //
	uintptr_t Tier = 0x18; //
	uintptr_t WeaponData = 0x3f8; //
	uintptr_t LastFireTime = 0xab0; //
	uintptr_t LastFireTimeVerified = 0xab4; //
	uintptr_t LastFireAbilityTime = 0xE28; //
	uintptr_t CurrentWeapon = 0x8F8; //
	uintptr_t bADSWhileNotOnGround = 0x4df6; //
	uintptr_t Levels = 0x170; //
	uintptr_t PersistentLevel = 0x30; // Usually The Same
	uintptr_t AActors = 0x98; // Usually The Same
	uintptr_t ActorCount = 0xA0; // Usually The Same
	uintptr_t RootComponent = 0x190; //
	uintptr_t FireStartLoc = 0xa20; //
	uintptr_t RelativeLocation = 0x128; //
	uintptr_t RelativeRotation = 0x140; //
	uintptr_t PlayerState = 0x2a8; //
	uintptr_t Mesh = 0x310; //
	uintptr_t TeamIndex = 0x10A0; //
}