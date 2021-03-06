#pragma once

#include "c4d.h"
#include "instance_functions.h"

class Command_Create : public CommandData
{
INSTANCEOF(Command_Create, CommandData)

public:


	static maxon::Result<Command_Create*> Alloc()
	{
		iferr_scope;
		return NewObj(Command_Create) iferr_return;
	}

	Int32 GetState(BaseDocument* doc) override
	{
		// Disable Menu entry if no object is selected
		const AutoAlloc<AtomArray> arr;
		doc->GetActiveObjects(arr, GETACTIVEOBJECTFLAGS::NONE);
		if (!arr || arr->GetCount() == 0)
			return 0;
		return CMD_ENABLED;
	}

	Bool Execute(BaseDocument* doc) override
	{
		if (!doc)
			return false;

		doc->StartUndo();


		// Create Array that holds all objects to operate on; respects order of selection
		const AutoAlloc<AtomArray> activeObjects;
		doc->GetActiveObjects(*activeObjects, GETACTIVEOBJECTFLAGS::SELECTIONORDER | GETACTIVEOBJECTFLAGS::CHILDREN);


		// Allocation failed
		if (!activeObjects)
			return false;


		// Detect Key modifiers
		const auto bCtrl = rh::g_CheckModifierKey(QCTRL);
		const auto bShift = rh::g_CheckModifierKey(QSHIFT);
		const auto bAlt = rh::g_CheckModifierKey(QALT);

		// Iterate through all selected objects
		const auto count = activeObjects->GetCount();
		for (auto i = 0; i < count; ++i)
		{
			const auto atom = activeObjects->GetIndex(i);

			// Something failed
			if (!atom)
				return false;

			// Treat atom as BaseObject
			const auto obj = static_cast<BaseObject*>(atom);

			// Create instances based on the object selected last
			if (bCtrl)
			{
				const auto refObj = static_cast<BaseObject*>(activeObjects->GetIndex(count - 1));
				if (obj != refObj && !refObj->IsInstanceOf(Oinstance))
				{
					if (bShift)
						g_MoveChildren(obj, refObj);
					g_CreateInstancesFromSelection(doc, refObj, obj, bAlt);
				}
			}

			// Normal operation if no modifier is pressed
			if (!bCtrl)
			{
				if (obj->IsInstanceOf(Oinstance))
					g_CreateInstanceCopy(obj);
				else
				{
					const auto instance = g_CreateInstance(obj);
					if (instance)
					{
						obj->CopyMatrixTo(instance);
						doc->InsertObject(instance, obj->GetUp(), obj->GetPred());
					}
				}
			}
		}
		EventAdd();

		return true;
	}
};
