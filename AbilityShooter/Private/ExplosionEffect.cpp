#include "AbilityShooter.h"
#include "AbilityShooterCharacter.h"
#include "ExplosionEffect.h"

AExplosionEffect::AExplosionEffect()
{
	explosionLightComponentName = TEXT("ExplosionLight");

	PrimaryActorTick.bCanEverTick = true;

	explosionLight = CreateDefaultSubobject<UPointLightComponent>(explosionLightComponentName);
	explosionLight->AttenuationRadius = 400.f;
	explosionLight->Intensity = 500.f;
	explosionLight->bUseInverseSquaredFalloff = false;
	explosionLight->LightColor = FColor(255, 158, 53);
	explosionLight->CastShadows = true;
	explosionLight->bVisible = true;

	explosionLightFadeOut = 0.2f;
}

void AExplosionEffect::BeginPlay()
{
	Super::BeginPlay();

	if (IsValid(explosionFX))
		UGameplayStatics::SpawnEmitterAtLocation(this, explosionFX, GetActorLocation(), GetActorRotation());

	if (IsValid(explosionSound))
		UGameplayStatics::PlaySoundAtLocation(this, explosionSound, GetActorLocation());

	if (IsValid(decal.material))
	{
		FRotator randomDecalRot = surfaceHit.ImpactNormal.Rotation();
		randomDecalRot.Roll = FMath::RandRange(-180.f, 180.f);

		UGameplayStatics::SpawnDecalAttached(decal.material, FVector(decal.size, decal.size, 1.0f),
			surfaceHit.GetComponent(), surfaceHit.BoneName,
			surfaceHit.ImpactPoint, randomDecalRot, EAttachLocation::KeepWorldPosition,
			decal.lifespan);
	}
}

void AExplosionEffect::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const float timeAlive = GetWorld()->TimeSeconds - CreationTime;
	const float timeRemaining = FMath::Max(0.f, explosionLightFadeOut - timeAlive);

	if (timeRemaining > 0.f)
	{
		const float fadeAlpha = 1.f - FMath::Square(timeRemaining / explosionLightFadeOut);

		UPointLightComponent* defLight = Cast<UPointLightComponent>(GetClass()->GetDefaultSubobjectByName(explosionLightComponentName));
		explosionLight->SetIntensity(defLight->Intensity * fadeAlpha);
	}
	else
		Destroy();
}