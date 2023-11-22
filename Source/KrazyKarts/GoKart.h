// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GoKart.generated.h"

UCLASS()
class KRAZYKARTS_API AGoKart : public APawn
{
	GENERATED_BODY()

	/** Move Input Action */
	// UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	// class UInputAction* MoveAction;

public:
	// Sets default values for this pawn's properties
	AGoKart();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	//Setup Player Input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Called for movement input */
	//void Move(const struct FInputActionValue& Value);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void MoveForward(float Value);

	void MoveRight(float Value);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_MoveForward(float Value);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_MoveRight(float Value);

private:

	UPROPERTY(Replicated)
	FVector Velocity;

	//Example for replicating with function
	UPROPERTY(ReplicatedUsing=OnRep_ReplicatedTransform)
	FTransform ReplicatedTransform;

	UFUNCTION()
	void OnRep_ReplicatedTransform();

	UPROPERTY(Replicated)
	float Throttle;

	UPROPERTY(Replicated)
	float SteeringThrow;

	//Force applied to car when throttle is fully down {N}
	UPROPERTY(EditAnywhere)
	float MaxDrivingForce = 10000;

	//Minimum radius of car turning circle at full lock (m)
	UPROPERTY(EditAnywhere)
	float MinTurningRadius = 10;

	//Higher means more drag
	UPROPERTY(EditAnywhere)
	float DragCoefficient = 16;

	//Higher means more rollling resistance
	UPROPERTY(EditAnywhere)
	float RollingResistanceCoefficient = 0.015;

	//Mass of the car {unit is kg}
	UPROPERTY(EditAnywhere)
	float Mass = 1000;

	void UpdateLocationFromVelocity(float DeltaTime);

	void ApplyRotation(float DeltaTime);

	FVector GetAirResistance();
	FVector GetRollingResistance();

};
