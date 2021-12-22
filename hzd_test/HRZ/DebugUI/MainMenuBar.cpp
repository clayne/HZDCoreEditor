#pragma once

#include <objbase.h>
#include <imgui.h>

#include "../../RTTI/RTTIScanner.h"
#include "../../RTTI/RTTIIDAExporter.h"

#include "../PCore/Ref.h"
#include "../Core/WorldPosition.h"
#include "../Core/Vec3.h"
#include "../Core/GameModule.h"
#include "../Core/SlowMotionManager.h"
#include "../Core/Player.h"
#include "../Core/PlayerGame.h"
#include "../Core/CameraEntity.h"
#include "../Core/Application.h"
#include "../Core/DebugSettings.h"
#include "../Core/WorldState.h"
#include "../Core/Mover.h"
#include "../Core/StreamingManager.h"

#include "DebugUI.h"
#include "DebugUIWindow.h"
#include "EntityWindow.h"
#include "WeatherWindow.h"
#include "DemoWindow.h"
#include "LogWindow.h"
#include "FocusEditorWindow.h"
#include "EntitySpawnerWindow.h"
#include "MainMenuBar.h"

extern std::unordered_set<HRZ::RTTIRefObject *> AllResources;
extern std::unordered_set<HRZ::RTTIRefObject *> CachedAIFactions;
extern HRZ::SharedLock ResourceListLock;

namespace HRZ
{
DECL_RTTI(AIFaction);
DECL_RTTI(Spawnpoint);
DECL_RTTI(SpawnSetupBase);
}

namespace HRZ::DebugUI
{

void MainMenuBar::Render()
{
	UpdateFreecam();

	if (!ShouldInterceptInput())
		return;

	if (!ImGui::BeginMainMenuBar())
		return;

	// Empty space for MSI afterburner display
	if (ImGui::BeginMenu("                        ", false))
		ImGui::EndMenu();

	// "World" menu
	if (ImGui::BeginMenu("World", Application::IsInGame()))
	{
		DrawWorldMenu();
		ImGui::EndMenu();
	}

	// "Time" menu
	if (ImGui::BeginMenu("Time", Application::IsInGame()))
	{
		DrawTimeMenu();
		ImGui::EndMenu();
	}

	// "Cheats" menu
	if (ImGui::BeginMenu("Cheats", Application::IsInGame()))
	{
		DrawCheatsMenu();
		ImGui::EndMenu();
	}

	// "Saves" menu
	if (ImGui::BeginMenu("Saves", Application::IsInGame()))
	{
		DrawSavesMenu();
		ImGui::EndMenu();
	}

	// "Debug" menu
	if (ImGui::BeginMenu("Debug"))
	{
		DrawDebugMenu();
		ImGui::EndMenu();
	}

	ImGui::EndMainMenuBar();
}

bool MainMenuBar::Close()
{
	return false;
}

void MainMenuBar::DrawWorldMenu()
{
	if (ImGui::MenuItem("Weather Editor"))
		AddWindow(std::make_unique<WeatherWindow>());

	if (ImGui::MenuItem("Entity Spawner"))
		AddWindow(std::make_unique<EntitySpawnerWindow>());

	ImGui::Separator();

	if (ImGui::MenuItem("Player Entity"))
		AddWindow(std::make_unique<EntityWindow>(Player::GetLocalPlayer()->m_Entity));

	if (ImGui::MenuItem("Player Camera Entity"))
		AddWindow(std::make_unique<EntityWindow>(Player::GetLocalPlayer()->GetLastActivatedCamera()));

	if (ImGui::MenuItem("Player Focus Entity"))
		AddWindow(std::make_unique<FocusEditorWindow>());
}

void MainMenuBar::DrawTimeMenu()
{
	auto& gameModule = Application::Instance().m_GameModule;
	auto& worldState = gameModule->m_WorldState;

	float timeOfDay = worldState->m_TimeOfDay;
	int cycleCount = worldState->m_DayNightCycleCount;

	int hour = static_cast<int>(timeOfDay);
	int minute = static_cast<int>((timeOfDay - hour) * 60.0f);

	ImGui::Text("Days Passed: %u", worldState->m_DayNightCycleCount);
	ImGui::Text("Current Time: %02u:%02u", hour, minute);

	ImGui::Separator();

	// Day/night cycle
	if (static bool pauseGameLogic; ImGui::MenuItem("Pause Game Logic", nullptr, &pauseGameLogic))
	{
		if (pauseGameLogic)
			gameModule->Suspend();
		else
			gameModule->Resume();
	}

	ImGui::MenuItem("Pause Time of Day", nullptr, &worldState->m_PauseTimeOfDay);

	if (ImGui::MenuItem("Pause Day/Night Cycle", nullptr, !worldState->m_DayNightCycleEnabled))
		worldState->m_DayNightCycleEnabled = !worldState->m_DayNightCycleEnabled;

	ImGui::MenuItem("Time of Day", nullptr, nullptr, false);

	if (ImGui::SliderFloat("##timeofdaybar", &timeOfDay, 0.0f, 23.999f))
	{
		worldState->SetTimeOfDay(timeOfDay, 0.0f);
		worldState->m_DayNightCycleCount = cycleCount;
	}

	ImGui::Separator();

	// Timescale
	ImGui::MenuItem("Enable Timescale Override in Menus", nullptr, &m_TimescaleOverrideInMenus);
	ImGui::MenuItem("Enable Timescale Override", nullptr, &m_TimescaleOverride);
	ImGui::MenuItem("Timescale", nullptr, nullptr, false);

	auto modifyTimescale = [](float Scale, bool SameLine = true)
	{
		char temp[64];
		sprintf_s(temp, "%g##setTs%g", Scale, Scale);

		if (ImGui::Button(temp))
		{
			m_Timescale = Scale;
			m_TimescaleOverride = true;
		}

		if (SameLine)
			ImGui::SameLine();
	};

	if (ImGui::SliderFloat("##TimescaleDragFloat", &m_Timescale, 0.01f, 10.0f))
		m_TimescaleOverride = true;

	modifyTimescale(0.01f);
	modifyTimescale(0.25f);
	modifyTimescale(0.5f);
	modifyTimescale(1.0f);
	modifyTimescale(2.0f);
	modifyTimescale(5.0f);
	modifyTimescale(10.0f, false);
}

void MainMenuBar::DrawCheatsMenu()
{
	auto& debugSettings = Application::Instance().m_DebugSettings;

	if (ImGui::MenuItem("Enable Freefly Cam", nullptr, m_FreeCamMode == FreeCamMode::Free))
	{
		m_FreeCamMode = (m_FreeCamMode == FreeCamMode::Free) ? FreeCamMode::Off : FreeCamMode::Free;
		m_FreeCamPosition = Player::GetLocalPlayer()->GetLastActivatedCamera()->m_Orientation.Position;
	}

	if (ImGui::MenuItem("Enable Noclip", nullptr, m_FreeCamMode == FreeCamMode::Noclip))
	{
		m_FreeCamMode = (m_FreeCamMode == FreeCamMode::Noclip) ? FreeCamMode::Off : FreeCamMode::Noclip;
		m_FreeCamPosition = Player::GetLocalPlayer()->m_Entity->m_Orientation.Position;
	}

	if (ImGui::MenuItem("Enable Demigod Mode", nullptr, debugSettings->m_GodModeState == EGodMode::On))
		debugSettings->m_GodModeState = (debugSettings->m_GodModeState == EGodMode::On) ? EGodMode::Off : EGodMode::On;

	if (ImGui::MenuItem("Enable God Mode", nullptr, debugSettings->m_GodModeState == EGodMode::Invulnerable))
		debugSettings->m_GodModeState = (debugSettings->m_GodModeState == EGodMode::Invulnerable) ? EGodMode::Off : EGodMode::Invulnerable;

	ImGui::MenuItem("Enable Infinite Ammo", nullptr, &debugSettings->m_InfiniteAmmo);
	ImGui::MenuItem("Enable Infinite Ammo Reserves", nullptr, &debugSettings->m_InfiniteAmmoReserves);
	ImGui::MenuItem("Enable All Unlocks", nullptr, &debugSettings->m_UnlockAll);
	ImGui::MenuItem("Simulate Game Completed", nullptr, &debugSettings->m_SimulateGameFinished);

	ImGui::Separator();

	if (ImGui::BeginMenu("Teleport To"))
	{
		auto doTeleport = [](const WorldPosition Position)
		{
			auto player = Player::GetLocalPlayer();

			if (!player || !player->m_Entity)
				return;

			WorldTransform transform
			{
				.Position = Position,
				.Orientation = player->m_Entity->m_Orientation.Orientation,
			};

			player->m_Entity->PlaceOnWorldTransform(transform, false);
		};

		if (ImGui::MenuItem("Freefly Cam Position"))
			doTeleport(m_FreeCamPosition);
		if (ImGui::MenuItem("Naming Cliff"))
			doTeleport(WorldPosition(2258.91, -1097.40, 359.18));
		if (ImGui::MenuItem("Elizabet Sobeck's Ranch"))
			doTeleport(WorldPosition(5349.0, -2322.0, 120.0));
		if (ImGui::MenuItem("Climbing Testing Area 1"))
			doTeleport(WorldPosition(-2278.0, -2222.0, 219.0));
		if (ImGui::MenuItem("Climbing Testing Area 2"))
			doTeleport(WorldPosition(-2265.0, -2307.0, 224.0));
		if (ImGui::MenuItem("Terrain Slope Testing Area"))
			doTeleport(WorldPosition(-2277.0, -2541.0, 324.0));
		if (ImGui::MenuItem("Script Testing Area"))
			doTeleport(WorldPosition(-2523.0, -2220.0, 221.0));
		if (ImGui::MenuItem("DLC Testing Area"))
			doTeleport(WorldPosition(-4953.22, -4907.42, 258.15));

		ImGui::EndMenu();
	}

	ImGui::Separator();

	if (ImGui::BeginMenu("Set Player Faction"))
	{
		auto player = Player::GetLocalPlayer();

		for (auto refObject : CachedAIFactions)
		{
			if (!refObject->GetRTTI()->IsExactKindOf(RTTI_AIFaction))
				continue;

			String assetName;
			refObject->GetRTTI()->AsClass()->GetMemberValue(refObject, "Name", &assetName);

			if (ImGui::Selectable(assetName.c_str(), player->m_Entity->m_Faction == (HRZ::AIFaction *)refObject))
				player->m_Entity->SetFaction((HRZ::AIFaction *)refObject);
		}

		ImGui::EndMenu();
	}
}

void MainMenuBar::DrawSavesMenu()
{
	auto doSave = [](uint8_t SaveType)
	{
		Offsets::Call<0x1269560, void(*)(uint8_t SaveType, bool Unknown, const class AIMarker *)>(SaveType, false, nullptr);
	};

	if (ImGui::MenuItem("Force Quicksave"))
	{
		// RestartOnSpawned determines if the player is moved to their last position or moved to a campfire
		auto playerGame = RTTI::Cast<PlayerGame>(Player::GetLocalPlayer());
		playerGame->m_RestartOnSpawned = true;

		doSave(2);
	}

	if (ImGui::MenuItem("Force Autosave"))
		doSave(4);

	if (ImGui::MenuItem("Force Manual Save"))
		doSave(1);

	if (ImGui::MenuItem("Force NG+ Save"))
		doSave(8);

	ImGui::Separator();

	if (ImGui::MenuItem("Load Previous Save"))
		Offsets::Call<0x117C3F0, void(*)(float)>(0.0f);
}

void MainMenuBar::DrawDebugMenu()
{
	if (auto debugSettings = Application::Instance().m_DebugSettings; debugSettings)
	{
		if (ImGui::MenuItem("Enable Damage Logging"))
			Offsets::CallID<"ToggleDamageLogging", void(*)(void *, bool)>(nullptr, true);
		ImGui::MenuItem("Enable Player Cover", "", &debugSettings->m_PlayerCoverEnabled);
		ImGui::MenuItem("Show Debug Coordinates", "", &debugSettings->m_DrawDebugCoordinates);
		ImGui::MenuItem("Disable Inactivity Check", "", &debugSettings->m_DisableInactivityCheck);
	}

	ImGui::Separator();

	if (ImGui::MenuItem("Show Log Window"))
		AddWindow(std::make_unique<LogWindow>());

	if (ImGui::MenuItem("Show ImGui Demo Window"))
		AddWindow(std::make_unique<DemoWindow>());

	ImGui::Separator();

	if (ImGui::MenuItem("Dump RTTI Typeinfo"))
		RTTIScanner::ExportAll("C:\\hzd_rtti_export", "HZD");

	if (ImGui::MenuItem("Dump Fullgame Typeinfo"))
	{
		RTTIIDAExporter idaExporter(RTTIScanner::GetAllTypes(), "HZD");
		idaExporter.ExportFullgameTypes("C:\\hzd_rtti_export");
	}

	ImGui::Separator();
	ImGui::MenuItem("", nullptr, nullptr, false);
	ImGui::MenuItem("", nullptr, nullptr, false);
	ImGui::MenuItem("", nullptr, nullptr, false);
	ImGui::Separator();

	if (ImGui::MenuItem("Terminate Process"))
		TerminateProcess(GetCurrentProcess(), 0);
}

void MainMenuBar::UpdateFreecam()
{
	if (m_FreeCamMode == FreeCamMode::Off)
		return;

	auto player = Player::GetLocalPlayer();

	if (!player)
		return;

	auto camera = player->GetLastActivatedCamera();

	if (!camera)
		return;

	auto& io = ImGui::GetIO();

	// Set up the camera's rotation matrix
	RotMatrix cameraMatrix;
	float yaw = 0.0f;
	float pitch = 0.0f;

	if (m_FreeCamMode == FreeCamMode::Free)
	{
		// Convert mouse X/Y to yaw/pitch angles in radians
		static float degreesX = 0.0f;
		static float degreesY = 0.0f;

		if (ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0.0f))
		{
			degreesX = fmodf(degreesX + io.MouseDelta.x, 360.0f);
			degreesY = fmodf(degreesY + io.MouseDelta.y, 360.0f);
		}

		yaw = degreesX * (3.14159f / 180.0f);
		pitch = degreesY * (3.14159f / 180.0f);

		cameraMatrix = RotMatrix(yaw, pitch, 0.0f);
	}
	else if (m_FreeCamMode == FreeCamMode::Noclip)
	{
		std::scoped_lock lock(camera->m_DataLock);

		// Convert matrix components to angles
		cameraMatrix = camera->m_Orientation.Orientation;
		cameraMatrix.Decompose(&yaw, &pitch, nullptr);
	}

	// Scale camera velocity based on delta time
	float speed = io.DeltaTime * 5.0f;

	if (io.KeysDown[VK_SHIFT])
		speed *= 10.0f;
	else if (io.KeysDown[VK_CONTROL])
		speed /= 5.0f;

	// WSAD keys for movement
	Vec3 moveDirection(sin(yaw) * cos(pitch), cos(yaw) * cos(pitch), -sin(pitch));

	if (io.KeysDown['W'])
		m_FreeCamPosition += moveDirection * speed;

	if (io.KeysDown['S'])
		m_FreeCamPosition -= moveDirection * speed;

	if (io.KeysDown['A'])
		m_FreeCamPosition -= moveDirection.CrossProduct(Vec3(0, 0, 1)) * speed;

	if (io.KeysDown['D'])
		m_FreeCamPosition += moveDirection.CrossProduct(Vec3(0, 0, 1)) * speed;

	WorldTransform newTransform
	{
		.Position = m_FreeCamPosition,
		.Orientation = cameraMatrix,
	};

	if (m_FreeCamMode == FreeCamMode::Free)
	{
		std::scoped_lock lock(camera->m_DataLock);

		camera->m_PreviousOrientation = newTransform;
		camera->m_Orientation = newTransform;
		camera->m_Flags |= Entity::WorldTransformChanged;
		//CallOffset<0x0BB41A0, void(*)(CameraEntity *, WorldTransform&)>(camera, cameraTransform);
	}
	else if (m_FreeCamMode == FreeCamMode::Noclip)
	{
		player->m_Entity->m_Mover->MoveToWorldTransform(newTransform, 0.01f, false);
	}
}

}