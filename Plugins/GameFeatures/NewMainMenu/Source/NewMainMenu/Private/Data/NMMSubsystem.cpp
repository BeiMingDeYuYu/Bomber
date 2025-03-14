﻿// Copyright (c) Yevhenii Selivanov

#include "Data/NMMSubsystem.h"
//---
#include "NMMUtils.h"
#include "Components/NMMSpotComponent.h"
#include "Data/NMMDataAsset.h"
#include "UtilityLibraries/MyBlueprintFunctionLibrary.h"
//---
#include UE_INLINE_GENERATED_CPP_BY_NAME(NMMSubsystem)

// Returns this Subsystem, is checked and wil crash if can't be obtained
UNMMSubsystem& UNMMSubsystem::Get(const UObject* OptionalWorldContext/* = nullptr*/)
{
	UNMMSubsystem* ThisSubsystem = UNMMUtils::GetNewMainMenuSubsystem(OptionalWorldContext);
	checkf(ThisSubsystem, TEXT("%s: 'SoundsSubsystem' is null"), *FString(__FUNCTION__));
	return *ThisSubsystem;
}

// Returns the data asset that contains all the assets and tweaks of New Main Menu game feature
const UNMMDataAsset* UNMMSubsystem::GetNewMainMenuDataAsset() const
{
	return NewMainMenuDataAssetInternal.LoadSynchronous();
}

// Add new Main-Menu spot, so it can be obtained by other objects
void UNMMSubsystem::AddNewMainMenuSpot(UNMMSpotComponent* NewMainMenuSpotComponent)
{
	if (ensureMsgf(NewMainMenuSpotComponent, TEXT("%s: 'NewMainMenuSpotComponent' is null"), *FString(__FUNCTION__)))
	{
		MainMenuSpotsInternal.AddUnique(NewMainMenuSpotComponent);
	}
}

// Removes Main-Menu spot if should not be available by other objects anymore
void UNMMSubsystem::RemoveMainMenuSpot(UNMMSpotComponent* MainMenuSpotComponent)
{
	if (ensureMsgf(MainMenuSpotComponent, TEXT("%s: 'MainMenuSpotComponent' is null"), *FString(__FUNCTION__)))
	{
		MainMenuSpotsInternal.RemoveSwap(MainMenuSpotComponent);
	}
}

// Returns currently selected Main-Menu spot
UNMMSpotComponent* UNMMSubsystem::GetActiveMainMenuSpotComponent() const
{
	for (UNMMSpotComponent* MainMenuSpotComponent : MainMenuSpotsInternal)
	{
		if (MainMenuSpotComponent && MainMenuSpotComponent->GetCinematicRow().RowIndex == ActiveMainMenuSpotIdx)
		{
			return MainMenuSpotComponent;
		}
	}

	return nullptr;
}

// Returns Main-Menu spots by given level type
void UNMMSubsystem::GetMainMenuSpotsByLevelType(TArray<UNMMSpotComponent*>& OutSpots, ELevelType LevelType) const
{
	for (UNMMSpotComponent* MainMenuSpotComponent : MainMenuSpotsInternal)
	{
		if (MainMenuSpotComponent
			&& MainMenuSpotComponent->GetCinematicRow().LevelType == LevelType)
		{
			OutSpots.AddUnique(MainMenuSpotComponent);
		}
	}

	// Sort the array based on the RowIndex
	OutSpots.Sort([](const UNMMSpotComponent& A, const UNMMSpotComponent& B)
	{
		return A.GetCinematicRow().RowIndex < B.GetCinematicRow().RowIndex;
	});
}

// Goes to another Spot to show another player character on current level
UNMMSpotComponent* UNMMSubsystem::MoveMainMenuSpot(int32 Incrementer)
{
	// Get all rows of current level type
	const ELevelType CurrentLevelType = UMyBlueprintFunctionLibrary::GetLevelType();
	TArray<UNMMSpotComponent*> CurrentLevelTypeSpots;
	GetMainMenuSpotsByLevelType(/*out*/CurrentLevelTypeSpots, CurrentLevelType);

	// Extract the row indices, so we can track the bounds
	TArray<int32> SpotRowIndices;
	for (const UNMMSpotComponent* SpotIt : CurrentLevelTypeSpots)
	{
		SpotRowIndices.AddUnique(SpotIt->GetCinematicRow().RowIndex);
	}

	const bool bFoundActiveIdx = SpotRowIndices.Contains(ActiveMainMenuSpotIdx);
	if (!ensureMsgf(bFoundActiveIdx, TEXT("%s: 'ActiveMainMenuSpotIdx' is not found in the 'SpotRowIndices'"), *FString(__FUNCTION__)))
	{
		// Most likely the level is switched that could be not supported yet
		return nullptr;
	}

	// Stop the current spot
	const int32 ActiveSpotPosition = SpotRowIndices.IndexOfByKey(ActiveMainMenuSpotIdx);
	checkf(CurrentLevelTypeSpots.IsValidIndex(ActiveSpotPosition), TEXT("ERROR: [%i] %s:\n'CurrentLevelTypeSpots array has to have ActiveSpotPosition index since it's the same size as SpotRowIndices array!"), __LINE__, *FString(__FUNCTION__))
	UNMMSpotComponent* CurrentSpot = CurrentLevelTypeSpots[ActiveSpotPosition];
	checkf(CurrentSpot, TEXT("ERROR: [%i] %s:\n'CurrentSpot' can't be null since CurrentLevelTypeSpots array does not contain nulls!"), __LINE__, *FString(__FUNCTION__))
	CurrentSpot->StopMasterSequence();

	// Find the new index based on the incrementer
	// If there is no next spot in array, it will take the first one with its index and vise versa for decrementing
	const int32 NewSpotIndex = (ActiveSpotPosition + Incrementer + SpotRowIndices.Num()) % SpotRowIndices.Num();
	ActiveMainMenuSpotIdx = SpotRowIndices[NewSpotIndex];

	// Play the new spot
	checkf(CurrentLevelTypeSpots.IsValidIndex(NewSpotIndex), TEXT("ERROR: [%i] %s:\n'CurrentLevelTypeSpots array has to have NewSpotIndex since it's the same size as SpotRowIndices array!"), __LINE__, *FString(__FUNCTION__));
	UNMMSpotComponent* NewSpot = CurrentLevelTypeSpots[NewSpotIndex];
	checkf(NewSpot, TEXT("ERROR: [%i] %s:\n'NewSpot' can't be null since CurrentLevelTypeSpots array does not contain nulls!"), __LINE__, *FString(__FUNCTION__));
	NewSpot->SetCinematicState(ENMMCinematicState::IdlePart);

	return NewSpot;
}

// Clears all transient data contained in this subsystem
void UNMMSubsystem::Deinitialize()
{
	NewMainMenuDataAssetInternal.Reset();
	MainMenuSpotsInternal.Empty();

	Super::Deinitialize();
}
