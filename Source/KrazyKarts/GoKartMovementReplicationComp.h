// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GoKartMovementComponent.h"
#include "GoKartMovementReplicationComp.generated.h"

//More Important struct that will need to be passed around to differenct .cpp files
//This is why it used the USTRUCT() property macro
USTRUCT()
struct FGoKartState
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FTransform ReplicatedTransform;

	UPROPERTY()
	FVector Velocity;

	UPROPERTY()
	FGoKartMove LastMove;
};

//SimpleStruct that will only function in this .cpp folder
struct FHermiteCubicSpline 
{
	FVector StartLocation, StartDerivative, TargetLocation, TargetDerivative;

	FVector InterpolateLocation(float LerpRatio) const
	{
		return FMath::CubicInterp(StartLocation, StartDerivative, TargetLocation, TargetDerivative, LerpRatio);
	};
	FVector InterpolateDerivative(float LerpRatio) const
	{
		return FMath::CubicInterpDerivative(StartLocation, StartDerivative, TargetLocation, TargetDerivative, LerpRatio);
	};
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KRAZYKARTS_API UGoKartMovementReplicationComp : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGoKartMovementReplicationComp();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SendMove(FGoKartMove Move);

private:
	void UpdateServerState(const FGoKartMove& Move);

	void ClearAcknowledgedMoves(FGoKartMove LastMove);

	UPROPERTY(ReplicatedUsing=OnRep_ServerState)
	FGoKartState ServerState;

	UFUNCTION()
	void OnRep_ServerState();
	void AutonomousProxy_OnRep_ServerState();
	void SimulatedProxy_OnRep_ServerState();

	void ClientTick(float DeltaTime);
	FHermiteCubicSpline CreateSpline();
	void InterpolateLocation(const FHermiteCubicSpline &Spline, float LerpRatio);
	void InterpolateVelocity(const FHermiteCubicSpline &Spline, float LerpRatio);
	void InterpolateRotation(float LerpRatio);
	float VelocityToDerivative();

	TArray<FGoKartMove> UnacknowledgedMoves;

	float ClientTimeSinceUpdate;
	float ClientTimeBetweenLastUpdates;
	FTransform ClientStartTransform;
	FVector ClientStartVelocity;

	UPROPERTY()
	UGoKartMovementComponent* MovementComponent;

	UPROPERTY()
	APawn* Pawn;
};
