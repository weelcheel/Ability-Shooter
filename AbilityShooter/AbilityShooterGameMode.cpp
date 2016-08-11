// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "AbilityShooter.h"
#include "AbilityShooterGameMode.h"
#include "AbilityShooterCharacter.h"

AAbilityShooterGameMode::AAbilityShooterGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}

void AAbilityShooterGameMode::ShooterKilled(AController* Killer, AController* KilledPlayer, APawn* KilledPawn, const UDamageType* DamageType)
{
	//@TODO: notify player states about Shooter deaths
}
