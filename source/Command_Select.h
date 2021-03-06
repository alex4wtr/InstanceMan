﻿#pragma once

#include "c4d.h"

#include "c4d_helpers_r20.h"
#include "instance_functions.h"


class Command_Select : public CommandData
{
INSTANCEOF(Command_Select, CommandData)

public:
	static maxon::Result<Command_Select*> Alloc()
	{
		iferr_scope;
		return NewObj(Command_Select) iferr_return;
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

		// Create Array that holds all objects to operate on
		const AutoAlloc<AtomArray> activeObjects;
		doc->GetActiveObjectsFilter(*activeObjects, false, NOTOK, Obase);


		// Allocation failed
		if (!activeObjects)
			return false;

		// Detect Key modifiers#
		BaseContainer state;
		GetInputState(BFM_INPUT_KEYBOARD, BFM_INPUT_MODIFIERS, state);
		const auto bShift = (state.GetInt32(BFM_INPUT_QUALIFIER) & QSHIFT) != 0;
		const auto bCtrl = (state.GetInt32(BFM_INPUT_QUALIFIER) & QCTRL) != 0;

		// Unselect all objects
		rh::g_DeselectAllObjects(doc);

		// Iterate through all selected objects
		for (auto i = 0; i < activeObjects->GetCount(); ++i)
		{
			const auto obj = static_cast<BaseObject*>(activeObjects->GetIndex(i));

			// No object was selected
			if (!doc)
				continue;


			// Currently processing an BaseObject?
			if (obj->IsInstanceOf(Obase))
			{
				// Retrieve the referenced object in the first instance selected and select all corresponding instances
				auto referenceObject = obj->IsInstanceOf(Oinstance) ? g_GetInstanceRef(obj, bCtrl) : obj; // Alternativley user getInstanceRefDeep
				auto currentObject = doc->GetFirstObject();


				// Iterate through all objects in the document
				while (currentObject)
				{
					if (currentObject->IsInstanceOf(Oinstance))
					{
						// Get linked object
						const auto linkedObj = g_GetInstanceRef(currentObject, bCtrl);

						if (linkedObj && linkedObj == referenceObject)
						{
							// Select reference object if Shift is held down
							if (bShift)
							{
								doc->AddUndo(UNDOTYPE::BITS, referenceObject);
								referenceObject->SetBit(BIT_ACTIVE);
							}

							doc->AddUndo(UNDOTYPE::BITS, currentObject);
							currentObject->SetBit(BIT_ACTIVE);
						}
					}
					currentObject = static_cast<BaseObject*>(rh::g_GetNextElement(static_cast<GeListNode*>(currentObject)));
				}
			}
		}

		EventAdd();

		doc->EndUndo();
		return true;
	}
};
