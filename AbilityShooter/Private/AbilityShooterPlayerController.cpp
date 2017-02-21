#include "AbilityShooter.h"
#include "AbilityShooterPlayerController.h"
#include "ASPlayerState.h"
#include "AbilityShooterGameMode.h"
#include "Effect.h"
#include "Ability.h"
#include "Engine/ActorChannel.h"
#include "PlayerHUD.h"
#include "StatsManager.h"
#include "UnrealNetwork.h"
#include "StoreItem.h"
#include "ShooterItem.h"
#include "Outfit.h"
#include "EquipmentItem.h"
#include "AbilityShooterGameState.h"

AAbilityShooterPlayerController::AAbilityShooterPlayerController()
{
	statsManager = CreateDefaultSubobject<UStatsManager>(TEXT("statsManager"));
	deathSpectatorCounter = -1;
}

void AAbilityShooterPlayerController::SetRespawnTimer(float respawnTime)
{
	if (Role < ROLE_Authority)
		return;

	AASPlayerState* ps = Cast<AASPlayerState>(PlayerState);
	if (IsValid(ps))
	{
		//set the server version of the respawn timer that actually restarts the player
		GetWorldTimerManager().SetTimer(ps->respawnTimer, this, &AAbilityShooterPlayerController::HandleRespawnTimer, respawnTime);

		//now set the visual timer for the clients
		ps->SetRespawnTimer(respawnTime);
	}
}

void AAbilityShooterPlayerController::HandleRespawnTimer()
{
	if (Role == ROLE_Authority)
	{
		AAbilityShooterGameMode* gm = GetWorld()->GetAuthGameMode<AAbilityShooterGameMode>();
		if (IsValid(gm))
		{
			gm->RestartPlayer(this);

			AAbilityShooterCharacter* ownedCharacter = Cast<AAbilityShooterCharacter>(GetCharacter());
			if (IsValid(ownedCharacter))
			{
				for (int32 i = 0; i < persistentEffects.Num(); i++)
				{
					if (persistentEffects[i].persistentTime >= 0.f)
					{
						persistentEffects[i].duration -= GetWorld()->TimeSeconds - persistentEffects[i].persistentTime;
						if (persistentEffects[i].duration <= 0.f)
							continue;
					}

					ownedCharacter->ApplyEffect(nullptr, persistentEffects[i]);
				}

				for (int32 i = 0; i < persistentAbilities.Num(); i++)
				{
					if (!IsValid(persistentAbilities[i]))
						continue;

					ownedCharacter->AddExistingAbility(persistentAbilities[i], i > 2);
				}

				ownedCharacter->EquipOutfit(persistentOutfit, true);

				persistentEffects.Empty();
				persistentAbilities.Empty();
				persistentOutfit = nullptr;
			}
		}
	}

	deathSpectatorCounter = -1;
}

void AAbilityShooterPlayerController::UpdateClientQuests_Implementation()
{
	APlayerHUD* hud = Cast<APlayerHUD>(GetHUD());
	if (IsValid(hud))
	{
		hud->OnQuestsUpdated();
	}
}

void AAbilityShooterPlayerController::GiveQuest(UQuest* newQuest)
{
	//don't run if there's no valid quest or we're not on the server
	if (!IsValid(newQuest) || Role < ROLE_Authority)
		return;

	newQuest->SetPlayerOwner(this);
	quests.AddUnique(newQuest);

	UpdateClientQuests();
}

void AAbilityShooterPlayerController::CompleteQuest(UQuest* quest)
{
	if (Role < ROLE_Authority || !IsValid(quest))
		return;

	OnClientQuestCompleted(quest->GetKey());

	quests.Remove(quest);
	UpdateClientQuests();
}

void AAbilityShooterPlayerController::GiveQuestCurrentObjectiveCheckpoint(const FString& questKey)
{
	if (Role < ROLE_Authority)
		return;

	for (UQuest* quest : quests)
	{
		if (IsValid(quest) && quest->GetKey() == questKey)
		{
			quest->IncrementCurrentObjectiveCheckpoint();
		}
	}

	UpdateClientQuests();
}

void AAbilityShooterPlayerController::GiveQuestNewObjective(const FString& questKey, FQuestObjective& newObjective)
{
	if (Role < ROLE_Authority)
		return;

	for (UQuest* quest : quests)
	{
		if (IsValid(quest) && quest->GetKey() == questKey)
		{
			quest->AddNewObjective(newObjective);
		}
	}

	UpdateClientQuests();
}

void AAbilityShooterPlayerController::OnClientQuestCompleted_Implementation(const FString& questKey)
{
	UQuest* quest = nullptr;
	for (UQuest* q : quests)
	{
		if (IsValid(q) && q->GetKey() == questKey)
			quest = q;
	}

	if (!IsValid(quest))
		return;

	APlayerHUD* hud = Cast<APlayerHUD>(GetHUD());
	if (IsValid(hud))
	{
		hud->OnQuestCompleted(quest);
	}
}

bool AAbilityShooterPlayerController::ReplicateSubobjects(class UActorChannel *Channel, class FOutBunch *Bunch, FReplicationFlags *RepFlags)
{
	check(Channel && Bunch && RepFlags);

	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	for (int32 i = 0; i < quests.Num(); i++)
	{
		if (quests[i])
			bWroteSomething |= Channel->ReplicateSubobject(quests[i], *Bunch, *RepFlags);
	}

	return bWroteSomething;
}

void AAbilityShooterPlayerController::GetKeysForAction(FName actionName, TArray<FInputActionKeyMapping>& bindings)
{
	bindings = PlayerInput->GetKeysForAction(actionName);
}

void AAbilityShooterPlayerController::Possess(APawn* InPawn)
{
	currentCharacter = Cast<AAbilityShooterCharacter>(InPawn);

	Super::Possess(InPawn);
}

bool AAbilityShooterPlayerController::ServerUpgradeOutfit_Validate(uint8 tree, uint8 row, uint8 col)
{
	return true;
}

void AAbilityShooterPlayerController::ServerUpgradeOutfit_Implementation(uint8 tree, uint8 row, uint8 col)
{
	AAbilityShooterCharacter* ownedCharacter = Cast<AAbilityShooterCharacter>(GetCharacter());
	if (IsValid(ownedCharacter))
	{
		ownedCharacter->UpgradeOutfit(tree, row, col);
	}
}

bool AAbilityShooterPlayerController::ServerIngameStorePurchase_Validate(FStoreItem purchaseItem)
{
	return true;
}

void AAbilityShooterPlayerController::ServerIngameStorePurchase_Implementation(FStoreItem purchaseItem)
{
	AASPlayerState* ps = Cast<AASPlayerState>(PlayerState);
	if (IsValid(ps))
	{
		if (ps->cash - purchaseItem.cost < 0)
			return;
		else
			ps->cash -= purchaseItem.cost;
	}

	if (IsValid(currentCharacter))
	{
		AShooterItem* newItem = GetWorld()->SpawnActor<AShooterItem>(purchaseItem.itemType);
		if (IsValid(newItem))
		{
			if (newItem->IsA(AOutfit::StaticClass()))
			{
				currentCharacter->EquipOutfit(Cast<AOutfit>(newItem));
			}
			else if (newItem->IsA(AEquipmentItem::StaticClass()))
			{
				currentCharacter->AddEquipment(Cast<AEquipmentItem>(newItem));
			}
			else if (newItem->IsA(AAbility::StaticClass()))
			{
				AAbility* abItem = Cast<AAbility>(newItem);
				TSubclassOf<AAbility> abType = abItem->GetClass();

				newItem->Destroy(true);
				currentCharacter->AddAbility(abType);
			}
		}
	}
}

void AAbilityShooterPlayerController::SetControlRotation(const FRotator& NewRotation)
{
	Super::SetControlRotation(NewRotation);

	AASPlayerState* ps = Cast<AASPlayerState>(PlayerState);
	if (IsValid(ps))
		ps->viewRotation = NewRotation;
}

void AAbilityShooterPlayerController::ClientReceiveKillfeedMessage_Implementation(const FString& message)
{
	APlayerHUD* hud = Cast<APlayerHUD>(GetHUD());
	if (IsValid(hud))
	{
		hud->ReceiveKillfeedMessage(message);
	}
}

void AAbilityShooterPlayerController::OnShooterDeath_Implementation(const FShooterDamage& damage)
{
	//let the HUD know the Shooter died to show death recap and spectator controls
	APlayerHUD* hud = Cast<APlayerHUD>(GetHUD());
	if (IsValid(hud))
	{
		hud->OnCharacterDied(damage);
	}
}

void AAbilityShooterPlayerController::DeathSpectateNextShooter(bool bIterateForward)
{
	AAbilityShooterCharacter* ownedCharacter = Cast<AAbilityShooterCharacter>(GetCharacter());
	if (IsValid(ownedCharacter) && ownedCharacter->IsAlive())
		return;

	AAbilityShooterGameState* gs = Cast<AAbilityShooterGameState>(GetWorld()->GameState);
	if (!IsValid(gs))
		return;

	AASPlayerState* nextSpectate = nullptr;
	AASPlayerState* thisState = Cast<AASPlayerState>(PlayerState);
	if (!IsValid(thisState))
		return;

	int bHitArrayBound = 0;
	while (nextSpectate == nullptr || nextSpectate == thisState)
	{
		if (bIterateForward)
		{
			if (deathSpectatorCounter + 1 >= gs->PlayerArray.Num())
			{
				deathSpectatorCounter = gs->PlayerArray.Num() - 1;
				bHitArrayBound++;
			}
			else
			{
				deathSpectatorCounter++;
				bHitArrayBound = 0;
			}
		}
		else
		{
			if (deathSpectatorCounter - 1 < 0)
			{
				deathSpectatorCounter = 0;
				bHitArrayBound++;
			}
			else
			{
				deathSpectatorCounter--;
				bHitArrayBound = 0;
			}
		}

		nextSpectate = Cast<AASPlayerState>(gs->PlayerArray[deathSpectatorCounter]);

		if (gs->bIsTeamGame && IsValid(nextSpectate) && thisState->GetTeamIndex() != nextSpectate->GetTeamIndex())
			nextSpectate = nullptr;

		if (bHitArrayBound >= 2) //prevent infinite loops, SAVE LIVES NOW
			return;
	}

	if (IsValid(nextSpectate) && IsValid(nextSpectate->currentShooter))
	{
		SetViewTarget(nextSpectate->currentShooter);
	}
}

void AAbilityShooterPlayerController::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAbilityShooterPlayerController, quests);
	DOREPLIFETIME_CONDITION(AAbilityShooterPlayerController, storeItems, COND_OwnerOnly);
}