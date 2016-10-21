#include "AbilityShooter.h"
#include "AbilityShooterCharacter.h"
#include "Projectile.h"
#include "Particles/ParticleSystemComponent.h"
#include "ExplosionEffect.h"
#include "UnrealNetwork.h"

AProjectile::AProjectile()
{
	collision = CreateDefaultSubobject<USphereComponent>(TEXT("sphereComp"));
	collision->InitSphereRadius(5.f);
	collision->AlwaysLoadOnClient = true;
	collision->AlwaysLoadOnServer = true;
	collision->bTraceComplexOnMove = true;
	collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	collision->SetCollisionObjectType(COLLISION_PROJECTILE);
	collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	RootComponent = collision;

	particleComp = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("particleComp"));
	particleComp->SetupAttachment(RootComponent);
	particleComp->bAutoActivate = false;
	particleComp->bAutoDestroy = false;

	movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("projectileMovement"));
	movement->UpdatedComponent = collision;
	movement->InitialSpeed = 2000.f;
	movement->MaxSpeed = 2000.f;
	movement->bRotationFollowsVelocity = true;
	movement->ProjectileGravityScale = 0.f;
	movement->bInitialVelocityInLocalSpace = false;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	bReplicates = true;
	bReplicateMovement = true;
}

void AProjectile::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	movement->OnProjectileStop.AddDynamic(this, &AProjectile::OnImpact);
	collision->MoveIgnoreActors.Add(Instigator);

	SetLifeSpan(projConfig.lifespan);
	controller = GetInstigatorController();
}

void AProjectile::InitVelocity(FVector& dir)
{
	if (IsValid(movement))
		movement->Velocity = dir * movement->InitialSpeed;
}

void AProjectile::OnImpact(const FHitResult& hitResult)
{
	if (Role == ROLE_Authority && !bExploded)
	{
		if (!projConfig.bExplodeOnAnyImpact)
		{
			AAbilityShooterCharacter* character = Cast<AAbilityShooterCharacter>(hitResult.GetActor());
			if (!IsValid(character) || hitResult.GetActor() == Instigator)
				return;
		}

		Explode(hitResult);
		DisableAndDestroy();
	}
}

void AProjectile::Explode(const FHitResult& impact)
{
	OnExplode(impact);

	if (particleComp)
		particleComp->Deactivate();

	// effects and damage origin shouldn't be placed inside mesh at impact point
	const FVector nudgedImpactLocation = impact.ImpactPoint + impact.ImpactNormal * 10.0f;

	if (projConfig.explosionDamage > 0.f && IsValid(projConfig.damageType))
	{
		if (projConfig.explosionRadius > 0.f)
			UGameplayStatics::ApplyRadialDamage(this, projConfig.explosionDamage, nudgedImpactLocation, projConfig.explosionRadius, projConfig.damageType, TArray<AActor*>(), this, controller);
		else
			UGameplayStatics::ApplyDamage(impact.GetActor(), projConfig.explosionDamage, controller, this, projConfig.damageType);
	}
		

	if (IsValid(explosionTemplate))
	{
		const FTransform spawnTransform(impact.ImpactNormal.Rotation(), nudgedImpactLocation);
		AExplosionEffect* effectActor = GetWorld()->SpawnActorDeferred<AExplosionEffect>(explosionTemplate, spawnTransform);
		if (IsValid(effectActor))
		{
			effectActor->surfaceHit = impact;
			UGameplayStatics::FinishSpawningActor(effectActor, spawnTransform);
		}
	}

	bExploded = true;
}

void AProjectile::DisableAndDestroy()
{
	UAudioComponent* projAudioComp = FindComponentByClass<UAudioComponent>();
	if (IsValid(projAudioComp) && projAudioComp->IsPlaying())
		projAudioComp->FadeOut(0.1f, 0.f);

	movement->StopMovementImmediately();

	SetLifeSpan(0.2f);
}

void AProjectile::OnRep_Exploded()
{
	FVector dir = GetActorForwardVector();

	const FVector startTrace = GetActorLocation() - dir * 200.f;
	const FVector endTrace = GetActorLocation() + dir * 150.f;
	FHitResult impact;

	if (!GetWorld()->LineTraceSingleByChannel(impact, startTrace, endTrace, COLLISION_PROJECTILE, FCollisionQueryParams(TEXT("ProjClient"), true, Instigator)))
	{
		impact.ImpactPoint = GetActorLocation();
		impact.ImpactNormal = -dir;
	}

	Explode(impact);
}

void AProjectile::PostNetReceiveVelocity(const FVector& newVelocity)
{
	if (IsValid(movement))
		movement->Velocity = newVelocity;
}

void AProjectile::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AProjectile, bExploded);
}