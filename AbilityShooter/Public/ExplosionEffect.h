#pragma once

#include "Sound/SoundCue.h"
#include "AbilityShooterTypes.h"
#include "ExplosionEffect.generated.h"

UCLASS(ABSTRACT, Blueprintable)
class AExplosionEffect : public AActor
{
	GENERATED_BODY()

protected:

	/** explosion FX */
	UPROPERTY(EditDefaultsOnly, Category = Effect)
	UParticleSystem* explosionFX;

	/** explosion light */
	UPROPERTY(VisibleDefaultsOnly, Category = Effect)
	UPointLightComponent* explosionLight;

	/** Point light component name */
	FName explosionLightComponentName;

	/** spawn explosion */
	virtual void BeginPlay() override;

	/** update fading light */
	virtual void Tick(float DeltaSeconds) override;

public:

	/** how long keep explosion light on? */
	UPROPERTY(EditDefaultsOnly, Category = Effect)
	float explosionLightFadeOut;

	/** explosion sound */
	UPROPERTY(EditDefaultsOnly, Category = Effect)
	USoundCue* explosionSound;

	/** explosion decals */
	UPROPERTY(EditDefaultsOnly, Category = Effect)
	FDecalData decal;

	/** surface data for spawning */
	UPROPERTY(BlueprintReadOnly, Category = Surface)
	FHitResult surfaceHit;

	AExplosionEffect();

};