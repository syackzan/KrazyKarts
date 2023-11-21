// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKart.h"
#include "Components/InputComponent.h"

// //Enhanced Input Component
// #include "EnhancedInputComponent.h"
// #include "EnhancedInputSubsystems.h"

// Sets default values
AGoKart::AGoKart()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AGoKart::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AGoKart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector Force = GetActorForwardVector() * MaxDrivingForce * Throttle;

	Force += GetAirResistance();
	Force += GetRollingResistance();

	FVector Acceleration = Force / Mass;

	Velocity = Velocity + Acceleration * DeltaTime;

	UpdateLocationFromVelocity(DeltaTime);

	ApplyRotation(DeltaTime);
}

void AGoKart::UpdateLocationFromVelocity(float DeltaTime)
{
	FVector Translation = Velocity * DeltaTime * 100;
	FHitResult HitResult;
	AddActorWorldOffset(Translation, true, &HitResult);

	if(HitResult.IsValidBlockingHit())
	{
		Velocity = GetActorForwardVector() * 0;
	}
};

void AGoKart::ApplyRotation(float DeltaTime)
{
	float DeltaLocation = FVector::DotProduct(GetActorForwardVector(), Velocity) * DeltaTime;
	float RotationAngle = DeltaLocation / MinTurningRadius * SteeringThrow;
	FQuat RotationDelta(GetActorUpVector(), RotationAngle);

	Velocity = RotationDelta.RotateVector(Velocity);

	AddActorWorldRotation(RotationDelta);
}

// Called to bind functionality to input
void AGoKart::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Moving
	//EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMyProjectCharacter::MoveForward);

	PlayerInputComponent->BindAxis("MoveForward", this, &AGoKart::Server_MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGoKart::Server_MoveRight);
}

FVector AGoKart::GetAirResistance()
{
	return - Velocity.GetSafeNormal() * Velocity.SizeSquared() * DragCoefficient;
}

FVector AGoKart::GetRollingResistance()
{
	float AccelerationDueToGravity = -GetWorld()->GetGravityZ() / 100;
	float NormalForce = Mass * AccelerationDueToGravity;
	return - Velocity.GetSafeNormal() * RollingResistanceCoefficient * NormalForce;
};

// void AGoKart::Move(const FInputActionValue &Value)
// {
// 	// input is a Vector2D
// 	FVector2D MovementVector = Value.Get<FVector2D>();

// 	if (Controller != nullptr)
// 	{
// 		// find out which way is forward
// 		const FRotator Rotation = Controller->GetControlRotation();
// 		const FRotator YawRotation(0, Rotation.Yaw, 0);

// 		// get forward vector
// 		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

// 		// get right vector
// 		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

// 		// add movement
// 		AddMovementInput(ForwardDirection, MovementVector.Y);
// 		AddMovementInput(RightDirection, MovementVector.X);
// 	}
// }

void AGoKart::Server_MoveForward_Implementation(float Value)
{
	//Set Velocity if we want to have a uniform drive experience
	//Velocity = GetActorForwardVector() * 20 * Value; 

	Throttle = Value;
};

bool AGoKart::Server_MoveForward_Validate(float Value)
{
	return FMath::Abs(Value) <= 1;
}

void AGoKart::Server_MoveRight_Implementation(float Value)
{
	SteeringThrow = Value;
}

bool AGoKart::Server_MoveRight_Validate(float Value)
{
	return FMath::Abs(Value) <= 1;
}
