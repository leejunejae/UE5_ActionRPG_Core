// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Enemies/EnemyBaseAIController.h"
#include "NavigationSystem.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Kismet/KismetMathLibrary.h"

#include "Core/Subsystems/WorldSystem/RouteDataSubsystem.h"
#include "Characters/Enemies/EnemyBase.h"

#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Damage.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "AI/CombatDecisionComponent.h"
#include "Combat/Components/HitReactionComponent.h"

#include "Utils/CoreLog.h"
#include "Utils/CustomMathUtility.h"

using namespace CustomMath::Spatial;

AEnemyBaseAIController::AEnemyBaseAIController()
{
	PrimaryActorTick.bCanEverTick = true;

    CombatDecisionComponent = CreateDefaultSubobject<UCombatDecisionComponent>("CombatDecisionComponent");
	
    AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>("PerceptionComponent");
	SetPerceptionComponent(*AIPerceptionComponent);

	AISenseConfigSight = CreateDefaultSubobject<UAISenseConfig_Sight>("SenseSight");
	AISenseConfigSight->DetectionByAffiliation.bDetectEnemies = true;
	AISenseConfigSight->DetectionByAffiliation.bDetectFriendlies = true;
	AISenseConfigSight->DetectionByAffiliation.bDetectNeutrals = true;
	AISenseConfigSight->SightRadius = 1000.0f;
	AISenseConfigSight->LoseSightRadius = 1200.0f;
	AISenseConfigSight->PeripheralVisionAngleDegrees = 70.0f;
	AISenseConfigSight->SetMaxAge(3.0f);
	AISenseConfigSight->AutoSuccessRangeFromLastSeenLocation = 400.0f;

	AISenseConfigHearing = CreateDefaultSubobject<UAISenseConfig_Hearing>("SenseHearing");
	AISenseConfigHearing->DetectionByAffiliation.bDetectEnemies = true;
	AISenseConfigHearing->DetectionByAffiliation.bDetectFriendlies = true;
	AISenseConfigHearing->DetectionByAffiliation.bDetectNeutrals = true;
	AISenseConfigHearing->HearingRange = 1000.0f;
	AISenseConfigHearing->HearingRange = 1200.0f;
	AISenseConfigHearing->SetMaxAge(3.0f);
	
	AISenseConfigDamage = CreateDefaultSubobject<UAISenseConfig_Damage>("SenseDamage");

	AIPerceptionComponent->ConfigureSense(*AISenseConfigSight);
	AIPerceptionComponent->ConfigureSense(*AISenseConfigDamage);
	AIPerceptionComponent->ConfigureSense(*AISenseConfigHearing);
	AIPerceptionComponent->SetDominantSense(AISenseConfigSight->GetSenseImplementation());
}

void AEnemyBaseAIController::BeginPlay()
{
	Super::BeginPlay();

	if (AIPerceptionComponent)
	{
		AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AEnemyBaseAIController::OnTargetPerceptionUpdated_Delegate);
		AIPerceptionComponent->OnTargetPerceptionForgotten.AddDynamic(this, &AEnemyBaseAIController::OnTargetPerceptionForgotten_Delegate);
	}
}

void AEnemyBaseAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AEnemyBaseAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

    if (AEnemyBase* Enemy = Cast<AEnemyBase>(InPawn))
    {
        if (UHitReactionComponent* HitReaction = Enemy->GetHitReactionComponent())
        {
            HitReaction->HitStartDelegate.AddUObject(
                this, &AEnemyBaseAIController::OnStaggeredStarted);
            HitReaction->HitEndDelegate.AddUObject(
                this, &AEnemyBaseAIController::OnStaggeredEnded);
        }
    }
}

void AEnemyBaseAIController::OnUnPossess()
{
    StopHostileMonitoring();
    HostileMap.Empty();
    PrimaryTarget = nullptr;

    Super::OnUnPossess();
}

void AEnemyBaseAIController::SetControllerData(UBehaviorTree* BehaviorTree, UBlackboardData* BlackboardData)
{
	CachedBT = BehaviorTree;
	CachedBB = BlackboardData;

	UBlackboardComponent* BlackboardComp = Blackboard.Get();

	if (!UseBlackboard(CachedBB, BlackboardComp))
	{
		UE_LOG(Log_AI, Error, TEXT("Failed Using BlackBoard"));
		return;
	}

	AEnemyBase* Enemy = Cast<AEnemyBase>(GetPawn());
	if (!Enemy) return;
	
	URouteDataSubsystem* RouteSys = GetWorld()->GetSubsystem<URouteDataSubsystem>();
	if (!RouteSys) return;

	const FGuid RouteGuid = Enemy->GetDefaultRouteGuid();
	APatrolRoute* Route = RouteSys->FindRoute(RouteGuid);

	if (!Route)
	{
		UE_LOG(Log_AI, Error, TEXT("Failed Find Route"));
	}

	Blackboard->SetValueAsObject(TEXT("PatrolRoute"), Route);


	if (!RunBehaviorTree(CachedBT))
	{
		UE_LOG(Log_AI, Error, TEXT("Failed Running BehaviorTree"));
	}
}

void AEnemyBaseAIController::OnTargetPerceptionUpdated_Delegate(AActor* Actor, FAIStimulus Stimulus)
{
    if (!IsValid(Actor))
        return;

	const ETeamAttitude::Type Attitude = GetTeamAttitudeTowards(*Actor);
	UE_LOG(Log_AI, Log, TEXT("[EnemyBaseAIController] %s Perception Updated By Actor : %s"), *GetNameSafe(GetPawn()), *GetNameSafe(Actor));
	
	switch (Attitude)
	{
	case ETeamAttitude::Hostile:
	{
		if (!Stimulus.WasSuccessfullySensed())
			return;

        FThreatEntry& Entry = HostileMap.FindOrAdd(Actor);
        Entry.Target = Actor;

        UpdateEntryFromStimulus(Entry, Actor, Stimulus);

		if (!bHostileMonitoring) StartHostileMonitoring();

		//Blackboard->SetValueAsObject(Key_Target, Actor);

		UE_LOG(Log_AI, Log, TEXT("[EnemyBaseAIController] %s Sets Target Actor : %s"), *GetNameSafe(GetPawn()), *GetNameSafe(Actor));
		break;
	}
	}
}

void AEnemyBaseAIController::OnTargetPerceptionForgotten_Delegate(AActor* Actor)
{
    if (!IsValid(Actor))
        return;

    const ETeamAttitude::Type Attitude = GetTeamAttitudeTowards(*Actor);

    UE_LOG(Log_AI, Log, TEXT("[EnemyBaseAIController] %s Generate Forget Event"), *Actor->GetName());

    switch (Attitude)
    {
    case ETeamAttitude::Hostile:
    {
        if (FThreatEntry* Entry = HostileMap.Find(Actor))
        {
            UE_LOG(Log_AI, Log, TEXT("[EnemyBaseAIController] Update %s Forget Data"), *Actor->GetName());
            const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
            Entry->bForgottened = true;
            Entry->ForgottenTime = Now;
            Entry->bCurrentlySensed = false;
            // ⚠️ Forgotten은 LOS가 아니라 “기억 만료”임. bHasLOS는 Monitor에서 Trace로 갱신.
        }
        break;
    }
    }
}

void AEnemyBaseAIController::StartHostileMonitoring()
{
	if (bHostileMonitoring) return;

	bHostileMonitoring = true;

	GetWorldTimerManager().SetTimer(
		HostileMonitorTimerHandle,
		this,
		&AEnemyBaseAIController::HostileMonitor,
        MonitorInterval,
		true
	);
}

void AEnemyBaseAIController::StopHostileMonitoring()
{
	bHostileMonitoring = false;
	GetWorldTimerManager().ClearTimer(HostileMonitorTimerHandle);
}

void AEnemyBaseAIController::HostileMonitor()
{
    APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn)
    {
        StopHostileMonitoring();
        return;
    }

    const float Now = GetWorld()->GetTimeSeconds();
    const float Delta = MonitorInterval; // Timer 주기를 Delta로 사용(정밀 필요하면 실제 시간 차 저장)

    // 엔트리 순회: Eval + Threat 갱신 + 제거
    for (auto It = HostileMap.CreateIterator(); It; ++It)
    {
        AActor* TargetActor = It.Key().Get();
        FThreatEntry& Entry = It.Value();

        if (!IsValid(TargetActor))
        {
            It.RemoveCurrent();
            continue;
        }

        UpdateRuntimeEval(Entry, ControlledPawn, TargetActor);
        UpdateThreatScore(Entry, Delta);

        if (ShouldRemoveEntry(Entry, Now))
        {
            // Primary였다면 해제
            if (PrimaryTarget.Get() == TargetActor)
            {
                PrimaryTarget = nullptr;
            }

            It.RemoveCurrent();
            continue;
        }
    }

    if (HostileMap.Num() == 0)
    {
        PrimaryTarget = nullptr;
        if (CurrentState != EAIBehaviorState::Stagger) CurrentState = EAIBehaviorState::Normal;
        PushToBlackboard(nullptr);
        StopHostileMonitoring();
        return;
    }

    // 1순위 타겟 선정/적용
    AActor* NewPrimary = ChoosePrimaryTarget(Now);
    ApplyPrimaryTarget(NewPrimary, Now);

    // 상태 업데이트 + BB 반영
    UpdateBehaviorStateFromPrimary();
    PushToBlackboard(PrimaryTarget.Get());
}

void AEnemyBaseAIController::UpdateEntryFromStimulus(FThreatEntry& Entry, AActor* Actor, const FAIStimulus& Stimulus)
{
    const FAISenseID SightID = UAISense::GetSenseID<UAISense_Sight>();
    const FAISenseID HearID = UAISense::GetSenseID<UAISense_Hearing>();
    const FAISenseID DamageID = UAISense::GetSenseID<UAISense_Damage>();

    const float Now = GetWorld()->GetTimeSeconds();

    Entry.LastStimulusTime = Now;
    Entry.LastStimulusLocation = Stimulus.StimulusLocation;
    Entry.bCurrentlySensed = Stimulus.WasSuccessfullySensed();

    // 어떤 센스인지 기록

    if (Stimulus.Type == SightID)
    {
        Entry.LastSense = EThreatSense::Sight;

        if (Entry.bCurrentlySensed)
        {
            Entry.LastSensedTime = Now;
            Entry.LastSeenLocation = Stimulus.StimulusLocation;
            Entry.bForgottened = false; // 다시 감지됐으면 만료 해제
        }
    }
    else if (Stimulus.Type == HearID)
    {
        Entry.LastSense = EThreatSense::Hearing;
        if (Entry.bCurrentlySensed)
        {
            Entry.LastSensedTime = Now;
            Entry.bForgottened = false;
            // 청각은 “봤던 위치” 개념이 애매하니 LastStimulusLocation만 신뢰하는 게 보통
        }
    }
    else if (Stimulus.Type == DamageID)
    {
        Entry.LastSense = EThreatSense::Damage;
        if (Entry.bCurrentlySensed)
        {
            Entry.LastSensedTime = Now;
            Entry.bForgottened = false;
            // 피격은 즉시 위협스코어 상승
            Entry.ThreatScore = FMath::Clamp(Entry.ThreatScore + DamageBonus, 0.f, MaxThreatScore);
        }
    }
    else
    {
        Entry.LastSense = EThreatSense::None;
        if (Entry.bCurrentlySensed)
        {
            Entry.LastSensedTime = Now;
            Entry.bForgottened = false;
        }
    }
}

void AEnemyBaseAIController::UpdateRuntimeEval(FThreatEntry& Entry, APawn* ControlledPawn, AActor* TargetActor)
{
    const FVector MyLoc = ControlledPawn->GetActorLocation();
    const FVector TargetLoc = TargetActor->GetActorLocation();

    Entry.PrevDistance = Entry.Distance;
    Entry.Distance = FVector::Dist2D(MyLoc, TargetLoc);

    // 접근 여부(거리가 줄면 approaching)
    const float DeltaDist = Entry.Distance - Entry.PrevDistance;
    Entry.bApproaching = (Entry.PrevDistance < FLT_MAX) && (DeltaDist < -50.f); // 50uu 이상 줄어들면 접근으로 판단

    // LOS는 "Perception 기억"이 아니라 실제 시야(Trace/엔진 함수)로 보는 게 안정적
    Entry.bHasLOS = LineOfSightTo(TargetActor);
}

void AEnemyBaseAIController::UpdateThreatScore(FThreatEntry& Entry, float DeltaSeconds)
{
    // 1) 거리 비례 감쇠 (멀수록 빨리 잊음, 가까이서 보고 있으면 거의 안 잊음)
    const float DecayAlpha = FMath::Clamp(Entry.Distance / SightRadiusForThreat, 0.f, 1.f);
    Entry.ThreatScore -= ThreatDecayPerSec * DecayAlpha * DeltaSeconds;

    // 2) 거리 기반 (0~SightRadius)
    const float DistAlpha = 1.f - FMath::Clamp(Entry.Distance / SightRadiusForThreat, 0.f, 1.f);
    const float DistanceThreat = DistAlpha * MaxDistanceThreat;

    // 3) LOS / 접근 보너스
    const float Approaching = Entry.bApproaching ? ApproachingBonus : 0.f;

    // 4) 센스별 “최근 자극” 보너스(간단 버전)
    float SenseBonus = 0.f;
    if (Entry.LastSense == EThreatSense::Hearing)
    {
        SenseBonus = HearingBonus;
    }

    // 5) 누적 (너무 과하지 않게: DeltaSeconds로 스케일)
    //   - “거리/LOS/접근”은 주기적으로 계속 영향을 주는 항목이므로, 여기서는 '목표치'처럼 반영.
    //   - 간단하게: (DistanceThreat + LOS + Approaching + SenseBonus) * DeltaSeconds * K
    // K 값은 체감 튜닝용: 1.0~2.0 추천
    constexpr float AccumK = 1.5f;
    Entry.ThreatScore += (DistanceThreat + Approaching + SenseBonus) * DeltaSeconds * AccumK;

    // 6) 클램프
    Entry.ThreatScore = FMath::Clamp(Entry.ThreatScore, 0.f, MaxThreatScore);
}

bool AEnemyBaseAIController::ShouldRemoveEntry(const FThreatEntry& Entry, float Now) const
{
    // 1) Threat가 남아있으면 유지
    if (Entry.ThreatScore > 0.f)
        return false;

    // 2) 기억 만료가 아니라면 유지(아직 감각 정보가 유효할 수 있음)
    if (!Entry.bForgottened)
        return false;

    // 3) Forgotten 이후 GraceTime까지는 유지
    const float SinceForgotten = Now - Entry.ForgottenTime;
    if (SinceForgotten < GraceTimeAfterForgotten)
        return false;

    // 최종 제거
    return true;
}

AActor* AEnemyBaseAIController::ChoosePrimaryTarget(float Now)
{
    AActor* BestActor = nullptr;
    float BestScore = -FLT_MAX;

    // 현재 Primary 유지 정책: 최소 유지 시간 안이면 스위치 금지
    bool bPrimaryLocked = false;
    if (PrimaryTarget.IsValid())
    {
        if (const FThreatEntry* Cur = HostileMap.Find(PrimaryTarget))
        {
            bPrimaryLocked = (Now < Cur->MinHoldUntilTime);
        }
    }

    // 현재 Primary의 점수
    float CurrentPrimaryScore = -FLT_MAX;
    if (PrimaryTarget.IsValid())
    {
        if (const FThreatEntry* Cur = HostileMap.Find(PrimaryTarget))
        {
            CurrentPrimaryScore = Cur->ThreatScore;
        }
    }

    for (auto& Pair : HostileMap)
    {
        AActor* Actor = Pair.Key.Get();
        FThreatEntry& Entry = Pair.Value;

        if (!IsValid(Actor))
            continue;

        // “Primary 잠금”이면 기존 Primary만 반환
        if (bPrimaryLocked)
        {
            return PrimaryTarget.Get();
        }

        // 점수 기반 선택
        const float Score = Entry.ThreatScore;

        if (Score > BestScore)
        {
            BestScore = Score;
            BestActor = Actor;
        }
    }

    // 스위칭 마진: 기존 Primary가 있고, 새 후보가 충분히 높지 않으면 유지
    if (PrimaryTarget.IsValid() && BestActor && BestActor != PrimaryTarget.Get())
    {
        if (BestScore < (CurrentPrimaryScore + SwitchScoreMargin))
        {
            return PrimaryTarget.Get();
        }
    }

    return BestActor;
}

void AEnemyBaseAIController::ApplyPrimaryTarget(AActor* NewPrimary, float Now)
{
    if (PrimaryTarget.Get() == NewPrimary)
        return;

    PrimaryTarget = NewPrimary;

    if (NewPrimary)
    {
        if (FThreatEntry* Entry = HostileMap.Find(NewPrimary))
        {
            Entry->LastBecamePrimaryTime = Now;
            Entry->MinHoldUntilTime = Now + MinPrimaryHoldTime;
        }
    }
}

void AEnemyBaseAIController::UpdateBehaviorStateFromPrimary()
{
    if (CurrentState == EAIBehaviorState::Stagger) return;

    AActor* TargetActor = PrimaryTarget.Get();
    if (!TargetActor)
    {
        CurrentState = EAIBehaviorState::Normal;
        ExitLockOn();
        return;
    }

    const FThreatEntry* Entry = HostileMap.Find(TargetActor);
    if (!Entry)
    {
        CurrentState = EAIBehaviorState::Normal;
        ExitLockOn();
        return;
    }

    if (Entry->ThreatScore >= CombatThreshold)
    {
        CurrentState = EAIBehaviorState::Combat;
    }
    else if (Entry->ThreatScore > AlertThreshold)
    {
        CurrentState = EAIBehaviorState::Alert;
    }
    else
    {
        CurrentState = EAIBehaviorState::Normal;
    }

    switch (CurrentState)
    {
    case EAIBehaviorState::Normal:
    case EAIBehaviorState::Flee:
    {
        ExitLockOn();
        break;
    }
    case EAIBehaviorState::Alert:
    case EAIBehaviorState::Combat:
    {
        EnterLockOn(TargetActor);
        break;
    }
    }
}

void AEnemyBaseAIController::PushToBlackboard(AActor* TargetActor)
{
    UBlackboardComponent* BB = GetBlackboardComponent();
    if (!BB)
        return;

    BB->SetValueAsObject(Key_Target, TargetActor);

    // Enum을 BB에 넣는 방식은 프로젝트마다 다름:
    // 1) BBKey_BehaviorState를 Enum(또는 uint8)로 만들었으면:
    if ((EAIBehaviorState)BB->GetValueAsEnum(Key_BehaviorState) != CurrentState)
    {
        UE_LOG(Log_AI, Log, TEXT("[EnemyBaseAIController] %s BehaviorState Set : %s"),
         *GetNameSafe(GetPawn()),
            *UEnum::GetValueAsString(CurrentState));
    }

    BB->SetValueAsEnum(Key_BehaviorState, static_cast<uint8>(CurrentState));

    if (TargetActor)
    {
        if (const FThreatEntry* Entry = HostileMap.Find(TargetActor))
        {
            BB->SetValueAsFloat(Key_TargetDistance, Entry->Distance);
            BB->SetValueAsBool(Key_TargetApproaching, Entry->bApproaching);
            BB->SetValueAsVector(Key_TargetLocation, TargetActor->GetActorLocation());
        }
    }
    else
    {
        BB->SetValueAsFloat(Key_TargetDistance, 0.f);
        BB->SetValueAsBool(Key_TargetApproaching, false);
    }
}

void AEnemyBaseAIController::OnStaggeredStarted()
{
    CurrentState = EAIBehaviorState::Stagger;
    ExitLockOn();

    if (UBlackboardComponent* BB = GetBlackboardComponent())
    {
        BB->SetValueAsEnum(Key_BehaviorState, static_cast<uint8>(EAIBehaviorState::Stagger));
    }
}

void AEnemyBaseAIController::OnStaggeredEnded()
{
    CurrentState = EAIBehaviorState::Normal;
    UpdateBehaviorStateFromPrimary();
    PushToBlackboard(PrimaryTarget.Get());
}

void AEnemyBaseAIController::EnterLockOn(AActor* Target)
{
    if (!IsValid(Target)) return;

    ACharacter* OwnerCharacter = Cast<ACharacter>(GetPawn());
    if (!OwnerCharacter) return;

    OwnerCharacter->GetCharacterMovement()->bUseControllerDesiredRotation = true;
    OwnerCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;

    SetFocus(Target, EAIFocusPriority::Gameplay);
}

void AEnemyBaseAIController::ExitLockOn()
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetPawn());
    if (!OwnerCharacter) return;

    ClearFocus(EAIFocusPriority::Gameplay);

    OwnerCharacter->GetCharacterMovement()->bUseControllerDesiredRotation = false;
    OwnerCharacter->GetCharacterMovement()->bOrientRotationToMovement = true;
}
