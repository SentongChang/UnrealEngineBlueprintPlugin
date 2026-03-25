// Copyright (c) 2026 AIBlueprintAssistant. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SWindow.h"

/**
 * Slate widget shown inside a modal SWindow after the AI generates T3D code.
 * The user can review the raw T3D, then click "Import Nodes" to apply it or
 * "Cancel" to discard.
 *
 * Usage:
 *   TSharedRef<SWindow> Window = SNew(SWindow)...;
 *   TSharedRef<SAIBPPreviewDialog> Dialog = SNew(SAIBPPreviewDialog)
 *       .T3DCode(MyCode).ParentWindow(Window);
 *   Window->SetContent(Dialog);
 *   FSlateApplication::Get().AddModalWindow(Window, ParentWidget);
 *   bool bImport = Dialog->WasConfirmed();
 */
class SAIBPPreviewDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAIBPPreviewDialog)
		: _T3DCode()
		, _ParentWindow()
	{}
		/** The raw T3D text to display for review. */
		SLATE_ARGUMENT(FString, T3DCode)

		/** The SWindow that hosts this widget — closed when Import/Cancel is clicked. */
		SLATE_ARGUMENT(TSharedPtr<SWindow>, ParentWindow)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Returns true if the user clicked "Import Nodes", false if they cancelled. */
	bool WasConfirmed() const { return bConfirmed; }

private:
	FReply OnImportClicked();
	FReply OnCancelClicked();

	TSharedPtr<SWindow> OwnerWindow;
	bool bConfirmed = false;
};
