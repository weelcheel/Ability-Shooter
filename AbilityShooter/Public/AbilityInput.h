#pragma once

#include "Runtime/UMG/Public/UMG.h"
#include "Runtime/UMG/Public/UMGStyle.h"
#include "Runtime/UMG/Public/Slate/SObjectWidget.h"
#include "Runtime/UMG/Public/IUMGModule.h"
#include "Runtime/UMG/Public/Blueprint/UserWidget.h"

#include "AbilityInput.generated.h"

USTRUCT(BlueprintType)
struct FAbilityKeyBinding
{
	GENERATED_USTRUCT_BODY()

	/* name of the action this binding represents in code */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Bindings)
	FString actionName;

	/* name of the action this binding represents in code */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Bindings)
	FKey key;

	/* name of the action this binding represents in code */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Bindings)
	FString keyAsString;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Bindings)
	bool bShift;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Bindings)
	bool bCtrl;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Bindings)
	bool bAlt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Bindings)
	bool bCmd;

	FAbilityKeyBinding()
	{}
	FAbilityKeyBinding(const FString InActionName, const FKey InKey, const bool bInShift, const bool bInCtrl, const bool bInAlt, const bool bInCmd)
	{
		key = InKey;
		keyAsString = key.GetDisplayName().ToString();
		bShift = bInShift;
		bCtrl = bInCtrl;
		bAlt = bInAlt;
		bCmd = bInCmd;
		actionName = InActionName;
	}
	FAbilityKeyBinding(const FInputActionKeyMapping& action)
	{
		key = action.Key;
		keyAsString = action.Key.GetDisplayName().ToString();
		bShift = action.bShift;
		bCtrl = action.bCtrl;
		bAlt = action.bAlt;
		bCmd = action.bCmd;
		actionName = action.ActionName.ToString();
	}
};

USTRUCT(BlueprintType)
struct FAbilityAxisBinding
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input Song")
	FString axisName = "";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input Song")
	FString keyAsString = "";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input Song")
	FKey key;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input Song")
	float scale = 1;

	FAbilityAxisBinding() {}
	FAbilityAxisBinding(const FString InAxisName, FKey InKey, float InScale)
		: axisName(InAxisName)
		, keyAsString(InKey.GetDisplayName().ToString())
		, key(InKey)
		, scale(InScale)
	{ }

	FAbilityAxisBinding(const FInputAxisKeyMapping& Axis)
		: key(Axis.Key)
		, keyAsString(Axis.Key.GetDisplayName().ToString())
		, scale(Axis.Scale)
	{
		axisName = Axis.AxisName.ToString();
	}
};

UCLASS()
class UAbilityInput : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UAbilityInput() {}

	/* gets all bindings for the specified key */
	UFUNCTION(BlueprintPure, Category = Bindings)
	static void GetAllAxisAndActionBindingsForKey(FKey key, TArray<FAbilityKeyBinding>& keyBindings, TArray<FAbilityAxisBinding>& axisBindings);

	/* gets an axis binding for a key event */
	UFUNCTION(BlueprintPure, Category = Bindings)
	static FAbilityAxisBinding GetAxisBinding(const FKeyEvent& keyEvent);

	/* gets every saved axis binding available */
	UFUNCTION(BlueprintPure, Category = Bindings)
	static void GetAllAxisBindings(TArray<FAbilityAxisBinding>& bindings);

	/* removes an axis binding */
	UFUNCTION(BlueprintCallable, Category = Bindings)
	static void RemoveAxisBinding(FAbilityAxisBinding toRemoveAxis);

	/* Returns false if the key could not be found as an existing mapping, otherwise rebinds a specified axis */
	UFUNCTION(BlueprintCallable, Category = Bindings)
	static bool RebindAxis(FAbilityAxisBinding original, FAbilityAxisBinding newBinding);

	/* updates an ability binding from an engine binding */
	static FORCEINLINE void UpdateAxisBinding(FInputAxisKeyMapping& destination, const FAbilityAxisBinding& inputBinding)
	{
		destination.Key = inputBinding.key;
		destination.Scale = inputBinding.scale;
	}

	/* gets an ability key binding from a key event */
	UFUNCTION(BlueprintPure, Category = Bindings)
	static FAbilityKeyBinding GetKeyBinding(const FKeyEvent& keyEvent);

	static FORCEINLINE void UpdateKeyBinding(FInputActionKeyMapping& destination, const FAbilityKeyBinding& inputBinding)
	{
		destination.Key = inputBinding.key;
		destination.bShift = inputBinding.bShift;
		destination.bCtrl = inputBinding.bCtrl;
		destination.bAlt = inputBinding.bAlt;
		destination.bCmd = inputBinding.bCmd;
	}

	/* gets all key bindings available */
	UFUNCTION(BlueprintPure, Category = Bindings)
	static void GetAllKeyBindings(TArray<FAbilityKeyBinding>& bindings);

	/* Returns false if the key could not be found as an existing mapping, otherwise rebinds a specified key */
	UFUNCTION(BlueprintCallable, Category = Bindings)
	static bool RebindKey(FAbilityKeyBinding original, FAbilityKeyBinding newBinding);

	/* removes a key binding */
	UFUNCTION(BlueprintCallable, Category = Bindings)
	static void RemoveKeyBinding(FAbilityKeyBinding toRemoveKey);
};