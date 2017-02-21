#include "AbilityShooter.h"
#include "AbilityShooterBot.h"
#include "AbilityAIController.h"
#include "AbilityShooterMovementComponent.h"

AAbilityShooterBot::AAbilityShooterBot(const FObjectInitializer& objectInitializer)
:Super(objectInitializer.SetDefaultSubobjectClass<UAbilityShooterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	AIControllerClass = AAbilityAIController::StaticClass();

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	GetMesh()->SetCollisionObjectType(ECC_Pawn);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(COLLISION_PROJECTILE, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Overlap);
	GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_PROJECTILE, ECR_Block);
	GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Ignore);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	statsManager = CreateDefaultSubobject<UStatsManager>(TEXT("botStatsManager"));
	statsManager->SetOwningCharacter(this);

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)

	health = 100.f;
	maxAbilityCount = 7;

	aboveHeadWidget->SetupAttachment(RootComponent, TEXT("hpbar"));
	aboveHeadWidget->SetWidgetClass(aboveHeadWidgetClass);
	aboveHeadWidget->SetDrawSize(FVector2D(0.f, 0.f));

	//create the headshot collision capsule
	headshotComponent->SetupAttachment(GetMesh(), TEXT("headshot"));
	headshotComponent->ComponentTags.Add(TEXT("headshot"));
	headshotComponent->SetCapsuleHalfHeight(20.f);
	headshotComponent->SetCapsuleRadius(15.f);
	headshotComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	headshotComponent->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Block);

	deathRecapListTimeout = 10.f;

	NetCullDistanceSquared = 2500000000.f;
}

void AAbilityShooterBot::FaceRotation(FRotator NewRotation, float DeltaTime)
{
	FRotator CurrentRotation = FMath::RInterpTo(GetActorRotation(), NewRotation, DeltaTime, 8.0f);

	Super::FaceRotation(CurrentRotation, DeltaTime);
}