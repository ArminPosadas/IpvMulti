#include "IpvmultiCharacter.h"
#include "IpvmultiCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"
#include "ThirdPersonMPProjectile.h"
#include "Components/SkeletalMeshComponent.h"
#include "Blueprint/UserWidget.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AIpvmultiCharacter

AIpvmultiCharacter::AIpvmultiCharacter():
CreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionComplete))
{
    // Set size for collision capsule
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
        
    // Don't rotate when the controller rotates. Let that just affect the camera.
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    // Configure character movement
    GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...    
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

    // Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
    // instead of recompiling to adjust them
    GetCharacterMovement()->JumpZVelocity = 700.f;
    GetCharacterMovement()->AirControl = 0.35f;
    GetCharacterMovement()->MaxWalkSpeed = 500.f;
    GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
    GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
    GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

    // Create a camera boom (pulls in towards the player if there is a collision)
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character    
    CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

    // Create a follow camera
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
    FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

    // Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
    // are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
    
    //Initialize the player's Health
    MaxHealth = 100.0f;
    CurrentHealth = MaxHealth;

    //Initialize ammo
    MaxAmmo = 5;
    CurrentAmmo = MaxAmmo;

    //Initialize projectile class
    ProjectileClass = AThirdPersonMPProjectile::StaticClass();
    //Initialize fire rate
    FireRate = 0.25f;
    bIsFiringWeapon = false;

    IOnlineSubsystem* OnLineSubsystem = IOnlineSubsystem::Get();
    if (OnlineSessionInterface)
    {
        OnlineSessionInterface = OnLineSubsystem->GetSessionInterface();
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Purple,
                FString::Printf(TEXT("Found Online Subsystem %s"), *OnLineSubsystem->GetSubsystemName().ToString()));
        }
    }
}

void AIpvmultiCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(AIpvmultiCharacter, CurrentHealth);
    DOREPLIFETIME(AIpvmultiCharacter, CurrentAmmo);
    DOREPLIFETIME(AIpvmultiCharacter, bIsRagdoll);
}

//////////////////////////////////////////////////////////////////////////
// Input

void AIpvmultiCharacter::NotifyControllerChanged()
{
    Super::NotifyControllerChanged();

    // Add Input Mapping Context
    if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }
}

void AIpvmultiCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    // Set up action bindings
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
        
        // Jumping
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

        // Moving
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AIpvmultiCharacter::Move);

        // Looking
        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AIpvmultiCharacter::Look);

        // Handle firing projectiles
        EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Triggered, this, &AIpvmultiCharacter::StartFire);
    }
    else
    {
        UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
    }
}

void AIpvmultiCharacter::SetCurrentHealth(float healthValue)
{
    if (GetLocalRole() == ROLE_Authority)
    {
        CurrentHealth = FMath::Clamp(healthValue, 0.f, MaxHealth);
        OnHealthUpdate();
    }
}

float AIpvmultiCharacter::TakeDamage(float DamageTaken, struct FDamageEvent const& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    float damageApplied = CurrentHealth - DamageTaken;
    SetCurrentHealth(damageApplied);
    return damageApplied;
}

void AIpvmultiCharacter::StartFire()
{
    if (!bIsFiringWeapon && CurrentAmmo > 0)
    {
        bIsFiringWeapon = true;
        UWorld* World = GetWorld();
        World->GetTimerManager().SetTimer(FiringTimer, this, &AIpvmultiCharacter::StopFire, FireRate, false);
        HandleFire();
    }
}

void AIpvmultiCharacter::StopFire()
{
    bIsFiringWeapon = false;
}

void AIpvmultiCharacter::HandleFire_Implementation()
{
    if (CurrentAmmo <= 0) return;

    CurrentAmmo--;
    OnAmmoUpdated();

    FVector spawnLocation = GetActorLocation() + ( GetActorRotation().Vector()  * 100.0f ) + (GetActorUpVector() * 50.0f);
    FRotator spawnRotation = GetActorRotation();
     
    FActorSpawnParameters spawnParameters;
    spawnParameters.Instigator = GetInstigator();
    spawnParameters.Owner = this;
     
    AThirdPersonMPProjectile* spawnedProjectile = GetWorld()->SpawnActor<AThirdPersonMPProjectile>(spawnLocation, spawnRotation, spawnParameters);
}

void AIpvmultiCharacter::OnRep_CurrentHealth()
{
    OnHealthUpdate();
}

void AIpvmultiCharacter::OnRep_CurrentAmmo()
{
    OnAmmoUpdated();
}

void AIpvmultiCharacter::AddAmmo(int32 Amount)
{
    if (GetLocalRole() == ROLE_Authority)
    {
        CurrentAmmo = MaxAmmo;
        OnAmmoUpdated();
    }
}

void AIpvmultiCharacter::StartRespawnTimer()
{
    if (GetLocalRole() == ROLE_Authority)
    {
        RespawnTimeRemaining = RespawnDuration;
        
        // Clear any existing timer
        GetWorld()->GetTimerManager().ClearTimer(RespawnTimerHandle);
        
        // Set up the actual respawn timer
        GetWorld()->GetTimerManager().SetTimer(
            RespawnTimerHandle,
            this,
            &AIpvmultiCharacter::Respawn,
            RespawnDuration,
            false
        );
    }
}

float AIpvmultiCharacter::GetRemainingRespawnTime() const
{
    if (GetWorld()->GetTimerManager().IsTimerActive(RespawnTimerHandle))
    {
        return GetWorld()->GetTimerManager().GetTimerRemaining(RespawnTimerHandle);
    }
    return 0.f;
}

void AIpvmultiCharacter::OnHealthUpdate_Implementation()
{
    bReplicates = true;
    // Client-specific functionality
    if (IsLocallyControlled())
    {
        FString healthMessage = FString::Printf(TEXT("You now have %f health remaining."), CurrentHealth);
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, healthMessage);
     
        if (CurrentHealth <= 0)
        {
            FString deathMessage = FString::Printf(TEXT("You have been killed."));
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, deathMessage);
            
            StartRagdoll();
            HideUI();
            ShowGameOverScreen();
        }
    }
    
    // Server-specific functionality
    if (GetLocalRole() == ROLE_Authority)
    {
        FString healthMessage = FString::Printf(TEXT("%s now has %f health remaining."), *GetFName().ToString(), CurrentHealth);
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, healthMessage);

        if (CurrentHealth <= 0)
        {
            DisableInput(nullptr);
            DisableCharacterCollision();
            StartRespawnTimer(); // Start the respawn timer
        }
    }
}

void AIpvmultiCharacter::Move(const FInputActionValue& Value)
{
    // input is a Vector2D
    FVector2D MovementVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        // find out which way is forward
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);

        // get forward vector
        const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    
        // get right vector 
        const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        // add movement 
        AddMovementInput(ForwardDirection, MovementVector.Y);
        AddMovementInput(RightDirection, MovementVector.X);
    }
}

void AIpvmultiCharacter::Look(const FInputActionValue& Value)
{
    // input is a Vector2D
    FVector2D LookAxisVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        // add yaw and pitch input to controller
        AddControllerYawInput(LookAxisVector.X);
        AddControllerPitchInput(LookAxisVector.Y);
    }
}

void AIpvmultiCharacter::StartRagdoll()
{
    // On server, call the server RPC
    if (GetLocalRole() == ROLE_Authority)
    {
        bIsRagdoll = true;
        OnRep_IsRagdoll(); // Call locally on server
    }
    else // On client, ask server to activate ragdoll
    {
        ServerStartRagdoll();
    }
}

void AIpvmultiCharacter::ShowGameOverScreen()
{
    if (GameOverWidgetClass && IsLocallyControlled())
    {
        // Create and show the game over widget
        GameOverWidget = CreateWidget<UUserWidget>(GetWorld(), GameOverWidgetClass);
        if (GameOverWidget)
        {
            GameOverWidget->AddToViewport();
            
            // Show cursor and enable UI input
            APlayerController* PC = Cast<APlayerController>(GetController());
            if (PC)
            {
                PC->bShowMouseCursor = true;
                PC->SetInputMode(FInputModeUIOnly());
            }
        }
    }
}

void AIpvmultiCharacter::DisableCharacterCollision()
{
    bReplicates = true;
    // Disable capsule collision
    UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
    if (CapsuleComp)
    {
        CapsuleComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        CapsuleComp->SetCollisionResponseToAllChannels(ECR_Ignore);
    }

    // Disable mesh collision (except for physics)
    USkeletalMeshComponent* MeshComp = GetMesh();
    if (MeshComp)
    {
        MeshComp->SetCollisionResponseToAllChannels(ECR_Ignore);
        MeshComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
        MeshComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
        MeshComp->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
    }

    // Disable character movement
    UCharacterMovementComponent* MovementComp = GetCharacterMovement();
    if (MovementComp)
    {
        MovementComp->StopMovementImmediately();
        MovementComp->DisableMovement();
    }
}

void AIpvmultiCharacter::ServerStartRagdoll_Implementation()
{
    bIsRagdoll = true;
}

void AIpvmultiCharacter::OnRep_IsRagdoll()
{
    USkeletalMeshComponent* MeshComp = GetMesh();
    if (!MeshComp) return;
    
    if (bIsRagdoll)
    {
        // Enable physics simulation on the mesh
        MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        MeshComp->SetSimulatePhysics(true);
        MeshComp->SetAllBodiesSimulatePhysics(true);
        MeshComp->WakeAllRigidBodies();
        
        DisableCharacterCollision();
    }
    else
    {
        // Disable physics simulation
        MeshComp->SetSimulatePhysics(false);
        MeshComp->SetAllBodiesSimulatePhysics(false);
        MeshComp->PutAllRigidBodiesToSleep();
        
        // Reset mesh position relative to capsule
        MeshComp->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetIncludingScale);
        MeshComp->SetRelativeLocation(FVector(0, 0, -GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
        MeshComp->SetRelativeRotation(FRotator(0, -90.f, 0));
        
        // Force physics state update
        MeshComp->SetPhysicsLinearVelocity(FVector::ZeroVector);
        MeshComp->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
    }
}

void AIpvmultiCharacter::HideUI()
{
    if (!IsLocallyControlled()) return;
}

void AIpvmultiCharacter::Respawn()
{
    GetWorld()->GetTimerManager().ClearTimer(TimerUpdateHandle);
    
    // Hide the timer display
    if (IsLocallyControlled())
    {
        HideRespawnTimer();
    }
    // Remove widget locally first
    if (IsLocallyControlled() && GameOverWidget)
    {
        GameOverWidget->RemoveFromParent();
        GameOverWidget = nullptr;
        
        // Reset input mode
        if (APlayerController* PC = Cast<APlayerController>(GetController()))
        {
            PC->bShowMouseCursor = false;
            PC->SetInputMode(FInputModeGameOnly());
        }
    }
    
    // Then tell server to respawn us
    if (GetLocalRole() < ROLE_Authority)
    {
        ServerRespawn();
    }
    else
    {
        ServerRespawn_Implementation();
    }
}

void AIpvmultiCharacter::UpdateTimerDisplay()
{
    if (GetLocalRole() == ROLE_Authority)
    {
        RespawnTimeRemaining = GetWorld()->GetTimerManager().GetTimerRemaining(RespawnTimerHandle);
        
        // Update locally
        if (IsLocallyControlled())
        {
            UpdateRespawnTimer(RespawnTimeRemaining);
        }
    }
}

void AIpvmultiCharacter::CreateGameSession()
{
    if (!OnlineSessionInterface.IsValid()) return;
    FNamedOnlineSession* ExistingSession = OnlineSessionInterface -> GetNamedSession(NAME_GameSession);
    if (ExistingSession)
    {
        OnlineSessionInterface->DestroySession(NAME_GameSession);
    }
    //Delegate-List
    OnlineSessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);
    //CreateSession
    TSharedPtr<FOnlineSessionSettings> SessionSettings = MakeShareable(new FOnlineSessionSettings());
    SessionSettings->bIsLANMatch = false;
    SessionSettings->NumPublicConnections = 4;
    SessionSettings->bAllowJoinInProgress = true;
    SessionSettings->bAllowJoinViaPresence = true;
    SessionSettings->bShouldAdvertise = true;
    SessionSettings->bUsesPresence = true;

    const ULocalPlayer* LocalPlayer= GetWorld()->GetFirstLocalPlayerFromController();

    OnlineSessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *SessionSettings);
}

void AIpvmultiCharacter::OnCreateSessionComplete(FName SessionName, bool bWasSuccess)
{
    if(bWasSuccess)
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                -1,
                15.f,
                FColor::Blue,
                FString::Printf(TEXT("Created Session %s"), *SessionName.ToString())
            );
        }
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(
            -1,
            15.f,
            FColor::Red,
            FString(TEXT("Create Session Failed"))
        );
    }
}

void AIpvmultiCharacter::ServerRespawn_Implementation()
{
    // Reset health
    CurrentHealth = MaxHealth;
    OnHealthUpdate();
    
    // Reset ammo
    CurrentAmmo = MaxAmmo;
    OnAmmoUpdated();
    
    // Reset ragdoll state FIRST
    bIsRagdoll = false;
    OnRep_IsRagdoll(); // Force immediate update
    
    // Reset physics state before enabling collision
    USkeletalMeshComponent* MeshComp = GetMesh();
    if (MeshComp)
    {
        MeshComp->SetSimulatePhysics(false);
        MeshComp->SetAllBodiesSimulatePhysics(false);
        MeshComp->PutAllRigidBodiesToSleep();
    }
    
    // Enable input and movement
    EnableInput(Cast<APlayerController>(GetController()));
    
    // Re-enable collision
    UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
    if (CapsuleComp)
    {
        CapsuleComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        CapsuleComp->SetCollisionResponseToAllChannels(ECR_Block);
    }
    
    // Re-enable character movement
    UCharacterMovementComponent* MovementComp = GetCharacterMovement();
    if (MovementComp)
    {
        MovementComp->SetMovementMode(EMovementMode::MOVE_Walking);
        MovementComp->StopMovementImmediately();
        MovementComp->ClearAccumulatedForces();
    }
    
    // Reset position to spawn point
    SetActorLocation(GetActorLocation());
    
    // Force widget cleanup on clients
    if (GameOverWidget)
    {
        GameOverWidget->RemoveFromParent();
        GameOverWidget = nullptr;
    }
    
    // Notify clients to clean up their widgets
    ClientRemoveWidget();
    
    // Force network update
    ForceNetUpdate();
}

void AIpvmultiCharacter::ClientRemoveWidget_Implementation()
{
    if (GameOverWidget && IsLocallyControlled())
    {
        GameOverWidget->RemoveFromParent();
        GameOverWidget = nullptr;
        
        // Reset input mode
        if (APlayerController* PC = Cast<APlayerController>(GetController()))
        {
            PC->bShowMouseCursor = false;
            PC->SetInputMode(FInputModeGameOnly());
        }
    }
}
