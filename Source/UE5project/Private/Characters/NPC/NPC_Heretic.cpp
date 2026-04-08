// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/NPC/NPC_Heretic.h"
#include "UI/ScriptWidget.h"
#include "Interaction/Dialogue/Components/DialogueSystem.h"

ANPC_Heretic::ANPC_Heretic()
{
    static ConstructorHelpers::FObjectFinder<USkeletalMesh>HERETIC_MESH(TEXT("/Game/Asset/Heretic/Meshes/SK_Heretic_A.SK_Heretic_A"));
    if (HERETIC_MESH.Succeeded())
    {
        GetMesh()->SetSkeletalMesh(HERETIC_MESH.Object);
    }


    GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);

    static ConstructorHelpers::FObjectFinder<UAnimSequence> NPC_Heretic_Anim (TEXT("/Game/NPC/Sitting_Idle.Sitting_Idle_C"));
    if (NPC_Heretic_Anim.Succeeded())
    {
        GetMesh()->PlayAnimation(NPC_Heretic_Anim.Object, true);
    }

    static ConstructorHelpers::FObjectFinder<UDataTable> DialogueDT_Asset(TEXT("DataTable'/Game/DialogueSample/DT_DisguisedHereticDialogue.DT_DisguisedHereticDialogue'"));
    if (DialogueDT_Asset.Succeeded())
    {
        DialogueDT = DialogueDT_Asset.Object;
    }
    
    NPCName = FName("Heretic");

    DialogueComponent->SetDialogueDT(DialogueDT);
    DialogueComponent->SetParticipant(NPCName);
}