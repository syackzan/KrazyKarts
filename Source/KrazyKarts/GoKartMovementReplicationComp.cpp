// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKartMovementReplicationComp.h"

//Helper Libraries
#include "Net/UnrealNetwork.h" //Enable Replication
#include "GameFramework/GameStateBase.h" //Used to GetServerWorldTimeSeconds()

// Sets default values for this component's properties
UGoKartMovementReplicationComp::UGoKartMovementReplicationComp()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicated(true);

	// ...
}

void UGoKartMovementReplicationComp::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	DOREPLIFETIME( UGoKartMovementReplicationComp, ServerState );
}


// Called when the game starts
void UGoKartMovementReplicationComp::BeginPlay()
{
	Super::BeginPlay();

	MovementComponent = GetOwner()->FindComponentByClass<UGoKartMovementComponent>();
	Pawn = Cast<APawn>(GetOwner());
	
}

// Called every frame
void UGoKartMovementReplicationComp::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if(MovementComponent == nullptr)
		return;

	FGoKartMove LastMove = MovementComponent->GetLastMove();

	//GetLocalRole() is called on actor, GetOwnerRole() is called on component
	if(GetOwnerRole() == ROLE_AutonomousProxy)
	{
		UnacknowledgedMoves.Add(LastMove);
		Server_SendMove(LastMove);
	}

	if(Pawn->IsLocallyControlled())
	{
		// FGoKartMove Move = MovementComponent->CreateMove(DeltaTime);
		// Server_SendMove(Move);
		UpdateServerState(LastMove);
	}

	if(GetOwnerRole() == ROLE_SimulatedProxy)
	{
		ClientTick(DeltaTime);
	}
}

void UGoKartMovementReplicationComp::UpdateServerState(const FGoKartMove& Move)
{
	ServerState.LastMove = Move;
	ServerState.ReplicatedTransform = GetOwner()->GetActorTransform();
	ServerState.Velocity = MovementComponent->GetVelocity();
}

void UGoKartMovementReplicationComp::ClientTick(float DeltaTime)
{
	ClientTimeSinceUpdate += DeltaTime;

	if(ClientTimeBetweenLastUpdates < KINDA_SMALL_NUMBER)
		return;

	if(MovementComponent == nullptr)
		return;

	float LerpRatio = ClientTimeSinceUpdate / ClientTimeBetweenLastUpdates;
	FHermiteCubicSpline Spline = CreateSpline();

	InterpolateLocation(Spline, LerpRatio);

	InterpolateVelocity(Spline, LerpRatio);

	InterpolateRotation(LerpRatio);
}

FHermiteCubicSpline UGoKartMovementReplicationComp::CreateSpline()
{
	FHermiteCubicSpline Spline;
	//Location Lerp
	Spline.TargetLocation = ServerState.ReplicatedTransform.GetLocation();
	Spline.StartLocation = ClientStartTransform.GetLocation();
	Spline.StartDerivative = ClientStartVelocity * VelocityToDerivative();
	Spline.TargetDerivative = ServerState.Velocity * VelocityToDerivative();
	return Spline;
};

void UGoKartMovementReplicationComp::InterpolateLocation(const FHermiteCubicSpline &Spline, float LerpRatio)
{
	FVector NewLocation = Spline.InterpolateLocation(LerpRatio);

	if(MeshOffsetRoot != nullptr)
	{
		MeshOffsetRoot->SetWorldLocation(NewLocation);
	}
}

void UGoKartMovementReplicationComp::InterpolateVelocity(const FHermiteCubicSpline &Spline, float LerpRatio)
{
	FVector NewDerivative = Spline.InterpolateDerivative(LerpRatio);
	FVector NewVelocity = NewDerivative / VelocityToDerivative(); 
	MovementComponent->SetVelocity(NewVelocity);
}

void UGoKartMovementReplicationComp::InterpolateRotation(float LerpRatio)
{
	//Get Target Rotation, Set Rotation, Create the New Rotation with Slerp & Set it
	FQuat TargetRotation = ServerState.ReplicatedTransform.GetRotation();
	FQuat StartRotation = ClientStartTransform.GetRotation();
	FQuat NewRotation = FQuat::Slerp(StartRotation, TargetRotation, LerpRatio);

	if(MeshOffsetRoot != nullptr)
	{
		MeshOffsetRoot->SetWorldRotation(NewRotation);
	}
}

float UGoKartMovementReplicationComp::VelocityToDerivative()
{
	//multiply by 100 to convert cms to meters
	return ClientTimeBetweenLastUpdates * 100;
};


void UGoKartMovementReplicationComp::ClearAcknowledgedMoves(FGoKartMove LastMove)
{
	TArray<FGoKartMove> NewMoves;

	for(const FGoKartMove& Move : UnacknowledgedMoves)
	{
		if(Move.Time > LastMove.Time)
		{
			NewMoves.Add(Move);
		}
	}

	UnacknowledgedMoves = NewMoves;
}

void UGoKartMovementReplicationComp::OnRep_ServerState()
{
	switch (GetOwnerRole())
	{
	case ROLE_AutonomousProxy:
		AutonomousProxy_OnRep_ServerState();
		break;
	case ROLE_SimulatedProxy:
		SimulatedProxy_OnRep_ServerState();
		break;
	default:
		break;
	}
}

void UGoKartMovementReplicationComp::AutonomousProxy_OnRep_ServerState()
{
	if(MovementComponent == nullptr)
		return;

	GetOwner()->SetActorTransform(ServerState.ReplicatedTransform);
	MovementComponent->SetVelocity(ServerState.Velocity);

	ClearAcknowledgedMoves(ServerState.LastMove);

	for(const FGoKartMove& Move : UnacknowledgedMoves)
	{
		MovementComponent->SimulateMove(Move);
	}

}

void UGoKartMovementReplicationComp::SimulatedProxy_OnRep_ServerState()
{	
	if(MovementComponent == nullptr) return;

	ClientTimeBetweenLastUpdates = ClientTimeSinceUpdate;
	ClientTimeSinceUpdate = 0;
	
	if(MeshOffsetRoot != nullptr)
	{
		ClientStartTransform.SetLocation(MeshOffsetRoot->GetComponentLocation());
		ClientStartTransform.SetRotation(MeshOffsetRoot->GetComponentQuat());
	}
	
	ClientStartVelocity = MovementComponent->GetVelocity();

	GetOwner()->SetActorTransform(ServerState.ReplicatedTransform);
}

void UGoKartMovementReplicationComp::Server_SendMove_Implementation(FGoKartMove Move)
{	
	if(MovementComponent == nullptr)
		return;

	ClientSimulatedTime += Move.DeltaTime;
	MovementComponent->SimulateMove(Move);

	UpdateServerState(Move);
};

bool UGoKartMovementReplicationComp::Server_SendMove_Validate(FGoKartMove Move)
{	
	float ProposedTime = ClientSimulatedTime + Move.DeltaTime;

	bool ClientNotRunningAhead = ProposedTime < GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
	
	if(!ClientNotRunningAhead)
	{
		UE_LOG(LogTemp, Error, TEXT("Client is running too fast"));
		return false;
	}

	if(!Move.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Received invalid move"));
		return false;
	}

	return true;
}

