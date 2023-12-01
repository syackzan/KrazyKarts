// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKartMovementReplicationComp.h"

//allow replication header
#include "Net/UnrealNetwork.h"

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
	
	//GetLocalRole() is called on actor, GetOwnerRole() is called on component
	if(GetOwnerRole() == ROLE_AutonomousProxy)
	{
		FGoKartMove Move = MovementComponent->CreateMove(DeltaTime);
		MovementComponent->SimulateMove(Move);

		UnacknowledgedMoves.Add(Move);
		Server_SendMove(Move);
	}

	if(GetOwnerRole() == ROLE_Authority && Pawn->IsLocallyControlled())
	{
		FGoKartMove Move = MovementComponent->CreateMove(DeltaTime);
		Server_SendMove(Move);
	}

	if(GetOwnerRole() == ROLE_SimulatedProxy)
	{
		MovementComponent->SimulateMove(ServerState.LastMove);
	}
}

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

void UGoKartMovementReplicationComp::Server_SendMove_Implementation(FGoKartMove Move)
{	
	if(MovementComponent == nullptr)
		return;

	MovementComponent->SimulateMove(Move);

	ServerState.LastMove = Move;
	ServerState.ReplicatedTransform = GetOwner()->GetActorTransform();
	ServerState.Velocity = MovementComponent->GetVelocity();
};

bool UGoKartMovementReplicationComp::Server_SendMove_Validate(FGoKartMove Move)
{
	return true; //TODO: Make Better Validation
}

