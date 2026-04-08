// Fill out your copyright notice in the Description page of Project Settings.


#include "Utils/SpawnMarker.h"
#include "Core/Subsystems/WorldSystem/WorldSpawnSubsystem.h"
#include "Utils/CoreLog.h"
#include "AI/PatrolRoute.h"

ASpawnMarker::ASpawnMarker()
{

}

void ASpawnMarker::BeginPlay()
{
    Super::BeginPlay();

    UWorld* World = GetWorld();
    if (!World) return;

    if (auto* SpawnSystem = World->GetSubsystem<UWorldSpawnSubsystem>())
    {
        SpawnSystem->RegisterMarker(this);
    }
}

#if WITH_EDITOR

void ASpawnMarker::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    if (!SpawnID.IsValid())
    {
        SpawnID = FGuid::NewGuid();
    }
}

void ASpawnMarker::PostDuplicate(bool bDuplicateForPIE)
{
    Super::PostDuplicate(bDuplicateForPIE);
    // 복제 시 중복 방지
    SpawnID = FGuid::NewGuid();
}

void ASpawnMarker::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    if (!SpawnID.IsValid())
    {
        SpawnID = FGuid::NewGuid();
    }

#if WITH_EDITORONLY_DATA
    if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(ASpawnMarker, DefaultRoute) && DefaultRoute)
    {
        DefaultRouteID = DefaultRoute->GetRouteID(); // GUID 복사
        DefaultRoute = nullptr;                     // 하드 참조 제거
        Modify();                                // 변경 기록
        MarkPackageDirty();
    }
#endif
}
#endif