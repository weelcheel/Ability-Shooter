#include "AbilityShooter.h"
#include "AbilityInput.h"

FAbilityKeyBinding UAbilityInput::GetKeyBinding(const FKeyEvent& keyEvent)
{
	FAbilityKeyBinding keyBinding;

	keyBinding.key = keyEvent.GetKey();
	keyBinding.keyAsString = keyEvent.GetKey().GetDisplayName().ToString();

	keyBinding.bAlt = keyEvent.IsAltDown();
	keyBinding.bCtrl = keyEvent.IsControlDown();
	keyBinding.bShift = keyEvent.IsShiftDown();
	keyBinding.bCmd = keyEvent.IsCommandDown();

	return keyBinding;
}

FAbilityAxisBinding UAbilityInput::GetAxisBinding(const FKeyEvent& keyEvent)
{
	FAbilityAxisBinding axisBinding;

	axisBinding.key = keyEvent.GetKey();
	axisBinding.keyAsString = keyEvent.GetKey().GetDisplayName().ToString();

	axisBinding.scale = 1.f;

	return axisBinding;
}

void UAbilityInput::GetAllAxisBindings(TArray<FAbilityAxisBinding>& bindings)
{
	bindings.Empty();

	const UInputSettings* settings = GetDefault<UInputSettings>();
	if (!settings)
		return;

	const TArray<FInputAxisKeyMapping>& axes = settings->AxisMappings;

	for (const FInputAxisKeyMapping& axis : axes)
	{
		bindings.Add(FAbilityAxisBinding(axis));
	}
}

void UAbilityInput::RemoveAxisBinding(FAbilityAxisBinding toRemoveAxis)
{
	UInputSettings* settings = GetMutableDefault<UInputSettings>();
	if (!settings)
		return;

	TArray<FInputAxisKeyMapping>& axes = settings->AxisMappings;

	bool bFound = false;
	for (int32 i = 0; i < axes.Num(); i++)
	{
		if (axes[i].Key == toRemoveAxis.key)
		{
			bFound = true;
			axes.RemoveAt(i);
			i = 0;
			continue;
		}
	}

	if (bFound)
	{
		settings->SaveKeyMappings();

		for (TObjectIterator<UPlayerInput> it; it; ++it)
		{
			it->ForceRebuildingKeyMaps(true);
		}
	}
}

void UAbilityInput::GetAllKeyBindings(TArray<FAbilityKeyBinding>& bindings)
{
	bindings.Empty();

	const UInputSettings* settings = GetDefault<UInputSettings>();
	if (!settings)
		return;

	const TArray<FInputActionKeyMapping>& keys = settings->ActionMappings;

	for (const FInputActionKeyMapping& key : keys)
	{
		bindings.Add(FAbilityKeyBinding(key));
	}
}

void UAbilityInput::RemoveKeyBinding(FAbilityKeyBinding toRemoveKey)
{
	UInputSettings* settings = GetMutableDefault<UInputSettings>();
	if (!settings)
		return;

	TArray<FInputActionKeyMapping>& actions = settings->ActionMappings;

	bool bFound = false;
	for (int32 i = 0; i < actions.Num(); i++)
	{
		if (actions[i].Key == toRemoveKey.key)
		{
			bFound = true;
			actions.RemoveAt(i);
			i = 0;
			continue;
		}
	}

	if (bFound)
	{
		settings->SaveKeyMappings();

		for (TObjectIterator<UPlayerInput> it; it; ++it)
		{
			it->ForceRebuildingKeyMaps(true);
		}
	}
}

void UAbilityInput::GetAllAxisAndActionBindingsForKey(FKey key, TArray<FAbilityKeyBinding>& keyBindings, TArray<FAbilityAxisBinding>& axisBindings)
{
	keyBindings.Empty();
	axisBindings.Empty();

	UInputSettings* settings = GetMutableDefault<UInputSettings>();
	if (!settings)
		return;

	const TArray<FInputActionKeyMapping>& keys = settings->ActionMappings;

	for (const FInputActionKeyMapping& thisKey : keys)
	{
		if (thisKey.Key == key)
		{
			keyBindings.Add(FAbilityKeyBinding(thisKey));
		}
	}

	const TArray<FInputAxisKeyMapping>& axes = settings->AxisMappings;

	for (const FInputAxisKeyMapping& axis : axes)
	{
		if (axis.Key == key)
		{
			axisBindings.Add(FAbilityAxisBinding(axis));
		}
	}
}

bool UAbilityInput::RebindAxis(FAbilityAxisBinding original, FAbilityAxisBinding newBinding)
{
	UInputSettings* settings = GetMutableDefault<UInputSettings>();
	if (!settings)
		return false;

	TArray<FInputAxisKeyMapping>& axes = settings->AxisMappings;

	bool bFound = false;
	for (FInputAxisKeyMapping& axis : axes)
	{
		if (axis.AxisName.ToString() == original.axisName && axis.Key == original.key)
		{
			UAbilityInput::UpdateAxisBinding(axis, newBinding);
			bFound = true;
			break;
		}
	}

	if (bFound)
	{
		const_cast<UInputSettings*>(settings)->SaveKeyMappings();

		for (TObjectIterator<UPlayerInput> it; it; ++it)
		{
			it->ForceRebuildingKeyMaps(true);
		}
	}

	return bFound;
}

bool UAbilityInput::RebindKey(FAbilityKeyBinding original, FAbilityKeyBinding newBinding)
{
	UInputSettings* settings = GetMutableDefault<UInputSettings>();
	if (!settings)
		return false;

	TArray<FInputActionKeyMapping>& actions = settings->ActionMappings;

	bool bFound = false;
	for (FInputActionKeyMapping& action : actions)
	{
		if (action.ActionName.ToString() == original.actionName && action.Key == original.key)
		{
			UAbilityInput::UpdateKeyBinding(action, newBinding);
			bFound = true;
			break;
		}
	}

	if (bFound)
	{
		const_cast<UInputSettings*>(settings)->SaveKeyMappings();

		for (TObjectIterator<UPlayerInput> it; it; ++it)
		{
			it->ForceRebuildingKeyMaps(true);
		}
	}

	return bFound;
}