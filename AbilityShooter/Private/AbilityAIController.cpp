#include "AbilityShooter.h"
#include "AbilityAIController.h"
#include "AbilityShooterBot.h"
#include "ASPlayerState.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "EquipmentItem.h"
#include "AbilityShooterGameMode.h"
#include "Effect.h"
#include "Ability.h"
#include "BulletGunWeapon.h"

AAbilityAIController::AAbilityAIController()
{
	BlackboardComp = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackBoardComp"));
	BrainComponent = BehaviorComp = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorComp"));
	bWantsPlayerState = true;
}

void AAbilityAIController::Possess(APawn* InPawn)
{
	Super::Possess(InPawn);

	AAbilityShooterBot* Bot = Cast<AAbilityShooterBot>(InPawn);

	// start behavior
	if (Bot && Bot->BotBehavior)
	{
		if (Bot->BotBehavior->BlackboardAsset)
		{
			BlackboardComp->InitializeBlackboard(*Bot->BotBehavior->BlackboardAsset);
		}

		EnemyKeyID = BlackboardComp->GetKeyID("Enemy");
		NeedNewEquipmentKeyID = BlackboardComp->GetKeyID("NeedNewEquipment");

		BehaviorComp->StartTree(*(Bot->BotBehavior));
	}
}

void AAbilityAIController::UnPossess()
{
	Super::UnPossess();

	BehaviorComp->StopTree();
}

void AAbilityAIController::BeginInactiveState()
{
	Super::BeginInactiveState();

	AAbilityShooterGameMode* game = GetWorld()->GetAuthGameMode<AAbilityShooterGameMode>();

	const float MinRespawnDelay = (IsValid(game)) ? game->GetRespawnTime(this) : 1.0f;

	GetWorldTimerManager().SetTimer(TimerHandle_Respawn, this, &AAbilityAIController::Respawn, MinRespawnDelay);
}

void AAbilityAIController::Respawn()
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

				for (AAbility* ability : persistentAbilities)
				{
					if (!IsValid(ability))
						continue;

					ownedCharacter->AddExistingAbility(ability);
				}

				persistentEffects.Empty();
				persistentAbilities.Empty();
			}
		}
	}
}

void AAbilityAIController::FindClosestEnemy()
{
	APawn* MyBot = GetPawn();
	if (MyBot == NULL)
	{
		return;
	}

	const FVector MyLoc = MyBot->GetActorLocation();
	float BestDistSq = MAX_FLT;
	AAbilityShooterCharacter* BestPawn = NULL;

	for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
	{
		AAbilityShooterCharacter* TestPawn = Cast<AAbilityShooterCharacter>(*It);
		if (TestPawn && TestPawn->IsAlive() && TestPawn->IsEnemyFor(this))
		{
			const float DistSq = (TestPawn->GetActorLocation() - MyLoc).SizeSquared();
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				BestPawn = TestPawn;
			}
		}
	}

	if (BestPawn)
	{
		SetTarget(BestPawn);
	}
}

bool AAbilityAIController::FindClosestEnemyWithLOS(AAbilityShooterCharacter* ExcludeEnemy)
{
	bool bGotEnemy = false;
	APawn* MyBot = GetPawn();
	if (MyBot != NULL)
	{
		const FVector MyLoc = MyBot->GetActorLocation();
		float BestDistSq = MAX_FLT;
		AAbilityShooterCharacter* BestPawn = NULL;

		for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
		{
			AAbilityShooterCharacter* TestPawn = Cast<AAbilityShooterCharacter>(*It);
			if (TestPawn && TestPawn != ExcludeEnemy && TestPawn->IsAlive() && TestPawn->IsEnemyFor(this))
			{
				if (HasEquipmentLOSToTarget(TestPawn, true))
				{
					const float DistSq = (TestPawn->GetActorLocation() - MyLoc).SizeSquared();
					if (DistSq < BestDistSq)
					{
						BestDistSq = DistSq;
						BestPawn = TestPawn;
					}
				}
			}
		}
		if (BestPawn)
		{
			SetTarget(BestPawn);
			bGotEnemy = true;
		}
	}
	return bGotEnemy;
}

bool AAbilityAIController::HasEquipmentLOSToTarget(AActor* InEnemyActor, const bool bAnyEnemy)
{
	static FName LosTag = FName(TEXT("AIWeaponLosTrace"));

	AAbilityShooterBot* MyBot = Cast<AAbilityShooterBot>(GetPawn());

	bool bHasLOS = false;
	// Perform trace to retrieve hit info
	FCollisionQueryParams TraceParams(LosTag, true, GetPawn());
	TraceParams.bTraceAsyncScene = true;

	TraceParams.bReturnPhysicalMaterial = true;
	FVector StartLocation = MyBot->GetActorLocation();
	StartLocation.Z += GetPawn()->BaseEyeHeight; //look from eyes

	FHitResult Hit(ForceInit);
	const FVector EndLocation = InEnemyActor->GetActorLocation();
	GetWorld()->LineTraceSingleByChannel(Hit, StartLocation, EndLocation, COLLISION_WEAPON, TraceParams);
	if (Hit.bBlockingHit == true)
	{
		// Theres a blocking hit - check if its our enemy actor
		AActor* HitActor = Hit.GetActor();
		if (Hit.GetActor() != NULL)
		{
			if (HitActor == InEnemyActor)
			{
				bHasLOS = true;
			}
			else if (bAnyEnemy == true)
			{
				// Its not our actor, maybe its still an enemy ?
				AAbilityShooterCharacter* HitChar = Cast<AAbilityShooterCharacter>(HitActor);
				if (HitChar != NULL && HitChar->IsEnemyFor(this))
				{
					bHasLOS = true;
				}
			}
		}
	}

	return bHasLOS;
}

void AAbilityAIController::AttackTarget()
{
	AAbilityShooterBot* MyBot = Cast<AAbilityShooterBot>(GetPawn());
	AEquipmentItem* MyWeapon = MyBot ? MyBot->GetCurrentEquipment() : NULL;
	if (MyWeapon == NULL)
	{
		return;
	}

	bool bCanShoot = false;
	AAbilityShooterCharacter* Enemy = GetTarget();
	if (Enemy && (Enemy->IsAlive()) && (MyWeapon->CanUse()))
	{
		bCanShoot = HasEquipmentLOSToTarget(Enemy, false);
	}

	if (bCanShoot)
	{
		if (MyWeapon->IsA(ABulletGunWeapon::StaticClass()))
			MyBot->AlternateUseStart();

		MyBot->PrimaryUseStart();
	}
	else
	{
		if (MyWeapon->IsA(ABulletGunWeapon::StaticClass()))
			MyBot->AlternateUseStop();

		MyBot->PrimaryUseStop();
	}
}

void AAbilityAIController::CheckCanUseEquipment(const class AEquipmentItem* currentEquip)
{
	if (BlackboardComp && currentEquip)
	{
		BlackboardComp->SetValue<UBlackboardKeyType_Bool>(NeedNewEquipmentKeyID, currentEquip->CanUse());
	}
}

void AAbilityAIController::SetTarget(class APawn* InPawn)
{
	if (BlackboardComp)
	{
		BlackboardComp->SetValue<UBlackboardKeyType_Object>(EnemyKeyID, InPawn);
		SetFocus(InPawn);
	}
}

class AAbilityShooterCharacter* AAbilityAIController::GetTarget() const
{
	if (BlackboardComp)
	{
		return Cast<AAbilityShooterCharacter>(BlackboardComp->GetValue<UBlackboardKeyType_Object>(EnemyKeyID));
	}

	return NULL;
}

void AAbilityAIController::UpdateControlRotation(float DeltaTime, bool bUpdatePawn)
{
	// Look toward focus
	FVector FocalPoint = GetFocalPoint();
	if (!FocalPoint.IsZero() && GetPawn())
	{
		FVector Direction = FocalPoint - GetPawn()->GetActorLocation();
		FRotator NewControlRotation = Direction.Rotation();

		NewControlRotation.Yaw = FRotator::ClampAxis(NewControlRotation.Yaw);

		SetControlRotation(NewControlRotation);

		APawn* const P = GetPawn();
		if (P && bUpdatePawn)
		{
			P->FaceRotation(NewControlRotation, DeltaTime);
		}

		AASPlayerState* ps = Cast<AASPlayerState>(PlayerState);
		if (IsValid(ps))
			ps->viewRotation = NewControlRotation;
	}
}

void AAbilityAIController::GameHasEnded(AActor* EndGameFocus, bool bIsWinner)
{
	// Stop the behaviour tree/logic
	BehaviorComp->StopTree();

	// Stop any movement we already have
	StopMovement();

	// Cancel the repsawn timer
	GetWorldTimerManager().ClearTimer(TimerHandle_Respawn);

	// Clear any enemy
	SetTarget(NULL);

	// Finally stop firing
	AAbilityShooterBot* MyBot = Cast<AAbilityShooterBot>(GetPawn());
	AEquipmentItem* MyWeapon = MyBot ? MyBot->GetCurrentEquipment() : NULL;
	if (MyWeapon == NULL)
	{
		return;
	}
	MyBot->PrimaryUseStop();
}