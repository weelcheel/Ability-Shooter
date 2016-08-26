#pragma once

#include "GameFramework/ProjectileMovementComponent.h"
#include "Projectile.generated.h"

USTRUCT()
struct FProjectileData
{
	GENERATED_USTRUCT_BODY()

	/** projectile class */
	UPROPERTY(EditDefaultsOnly, Category = Projectile)
	TSubclassOf<class AProjectile> type;

	/** life time */
	UPROPERTY(EditDefaultsOnly, Category = Projectile)
	float lifespan;

	/** damage at impact point */
	UPROPERTY(EditDefaultsOnly, Category = Projectile)
	int32 explosionDamage;

	/** radius of damage */
	UPROPERTY(EditDefaultsOnly, Category = Projectile)
	float explosionRadius;

	/** type of damage */
	UPROPERTY(EditDefaultsOnly, Category = Projectile)
	TSubclassOf<UDamageType> damageType;

	/** defaults */
	FProjectileData()
	{
		type = NULL;
		lifespan = 10.0f;
		explosionDamage = 100;
		explosionRadius = 300.0f;
		damageType = UDamageType::StaticClass();
	}
};

UCLASS(ABSTRACT, Blueprintable)
class AProjectile : public AActor
{
	GENERATED_BODY()

protected:

	/** movement component */
	UPROPERTY(VisibleDefaultsOnly, Category = Projectile)
	UProjectileMovementComponent* movement;

	/** collisions */
	UPROPERTY(VisibleDefaultsOnly, Category = Projectile)
	USphereComponent* collision;

	UPROPERTY(VisibleDefaultsOnly, Category = Projectile)
	UParticleSystemComponent* particleComp;

	/* effects for explosion */
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	TSubclassOf<class AExplosionEffect> explosionTemplate;

	/* cache the controller that launched this projectile */
	AController* controller;

	/* projectile data */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = ProjData)
	FProjectileData projConfig;

	/* has this projectile exploded? */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_Exploded)
	bool bExploded;

	/* initial setup */
	virtual void PostInitializeComponents() override;

	/* setup velocity */
	void InitVelocity(FVector& dir);

	/* handle hit */
	UFUNCTION()
	void OnImpact(const FHitResult& hitResult);

	/** [client] explosion happened */
	UFUNCTION()
	void OnRep_Exploded();

	/** trigger explosion */
	void Explode(const FHitResult& impact);

	/* blueprint hook for on explosion effects */
	UFUNCTION(BlueprintImplementableEvent, Category = Explosion)
	void OnExplode(const FHitResult& impact);

	/** shutdown projectile and prepare for destruction */
	void DisableAndDestroy();

	/** update velocity on client */
	virtual void PostNetReceiveVelocity(const FVector& newVelocity) override;

public:
	AProjectile();
};