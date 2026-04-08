// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ScriptWidget.h"
#include "Interaction/Dialogue/Components/DialogueSystem.h"
#include "Characters/NPC/PBNPCBase.h"
#include "Input/Reply.h"
#include "Interaction/Interfaces/InteractInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetStringLibrary.h"

UScriptWidget::UScriptWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	
}

void UScriptWidget::SetScriptWidget(APBNPCBase* InteractActor)
{
	if (InteractActor)
	{
		DialogueActor = InteractActor;
		DialogueComponent = DialogueActor->GetComponentByClass<UDialogueSystem>();
		if (DialogueComponent)
		{
			DialogueDT = DialogueComponent->GetDialogueDT();
			if (DialogueDT)
			{
				APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
				FInputModeUIOnly WidgetInputMode;
				WidgetInputMode.SetWidgetToFocus(this->TakeWidget());
				PlayerController->SetInputMode(WidgetInputMode);
				SetIsFocusable(true);
				SetKeyboardFocus();
				DialogueComponent->SetIsDialogue(true);
				ShowDialogue(GetDTLoc());
				AddToViewport();
			}
		}
	}
}

void UScriptWidget::ShowDialogue(FName RowName)
{
	CurDialogueDTRow = DialogueDT->FindRow<FDialogueContentStruct>(RowName, "");
	if (!CurDialogueDTRow)
	{
		UE_LOG(LogTemp, Warning, TEXT("Row Name Not Found in DT"));
		return;
	}
	SetTypeWriter(CurDialogueDTRow->Text);
}

void UScriptWidget::SetTypeWriter(FText CurText)
{
	//UE_LOG(LogTemp, Warning, TEXT("SetTypeWriter"));
	InitTypeWriter();
	TypingDelay.BindUObject(this, &UScriptWidget::TypeWriter);
	ScriptText = CurText;
	ScriptString = ScriptText.ToString();
	ScriptLength = ScriptString.Len();
	TypeWriter();
	IsTypeWriting = true;
}

void UScriptWidget::TypeWriter()
{
	//UE_LOG(LogTemp, Warning, TEXT("TypeWrtier"));
	ScriptChar = UKismetStringLibrary::GetSubstring(ScriptString, ScriptIndex, 1);
	FFormatNamedArguments CombineText;
	CombineText.Add(TEXT("Origin"), CurScriptText);
	CombineText.Add(TEXT("New"), FText::FromString(ScriptChar));

#define LOCTEXT_NAMESPACE "MyNamespace"
	CurScriptText = FText::Format(LOCTEXT("NextScriptText", "{Origin}{New}"), CombineText);
#undef LOCTEXT_NAMESPACE

	if (ScriptTextBlock != nullptr)
	{
		//UE_LOG(LogTemp, Warning, TEXT("ScriptTextBlock NOT NULL"));
		ScriptTextBlock->SetText(CurScriptText);
		ScriptIndex++;
		if (ScriptIndex < ScriptLength)
		{
			float Delay = 0.1f;
			GetWorld()->GetTimerManager().SetTimer(TypingDelayHandle, TypingDelay, Delay, false);
		}
		else
		{
			IsTypeWriting = false;
		}
	}
}

void UScriptWidget::CompleteTypeWriter()
{
	GetWorld()->GetTimerManager().ClearTimer(TypingDelayHandle);
	ScriptTextBlock->SetText(CurDialogueDTRow->Text);
	IsTypeWriting = false;
}

void UScriptWidget::InitTypeWriter()
{
	ScriptIndex = 0;
	ScriptLength = 0;
	ScriptText = FText::GetEmpty();
	CurScriptText = FText::GetEmpty();
	ScriptString = NULL;
	ScriptChar = NULL;
}

void UScriptWidget::CloseDialogue()
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	FInputModeGameOnly CharacterInputMode;
	PlayerController->SetInputMode(CharacterInputMode);
	RemoveFromParent();
	IInteractInterface::Execute_EndInteract(DialogueActor);
}

FName UScriptWidget::GetDTLoc()
{
	return DialogueComponent->GetPrevDialogLoc() == NAME_None ? FName("Start") : DialogueComponent->GetPrevDialogLoc();
}

FReply UScriptWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	Super::NativeOnKeyDown(InGeometry, InKeyEvent);
	if (IsTypeWriting)
	{
		CompleteTypeWriter();
	}
	else
	{
		if (CurDialogueDTRow->JumpTo == FName("Finish"))
		{
			CloseDialogue();
		}
		else
		{
			ShowDialogue(CurDialogueDTRow->JumpTo);
		}
	}
	return FReply::Handled();
	//return FReply::Unhandled();
}
