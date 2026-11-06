#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"

class UTexture2D;
class UButton;
class UBorder;
class UImage;

/** Loads Grimm Tower UI textures from /Game/UI/Art and applies them to widgets. */
namespace UiArt
{
	UTexture2D* Load(const TCHAR* AssetName);

	/** Sync-load the common menu/HUD art so first PIE has valid texture sizes. */
	void PreloadCommon();

	/** Uniformly scale native texture size to fit inside MaxBox (aspect preserved). */
	FVector2D FitNativeSize(UTexture2D* Texture, FVector2D MaxBox);

	FSlateBrush MakeBrush(UTexture2D* Texture, FVector2D Size = FVector2D::ZeroVector);

	/**
	 * Image button using native aspect ratio, scaled down to fit MaxSize.
	 * Pass MaxSize as the previous slot size (e.g. 360x110).
	 */
	void StyleImageButton(UButton* Button, const TCHAR* TextureName, FVector2D MaxSize);

	/** Panel/background brush. If MaxSize is set, uses fitted native size; else native pixels. */
	void StyleBorderImage(UBorder* Border, const TCHAR* TextureName, FVector2D MaxSize = FVector2D::ZeroVector);

	/** Image widget: native aspect, scaled to fit MaxSize. */
	void StyleImage(UImage* Image, const TCHAR* TextureName, FVector2D MaxSize);

	/** Returns fitted size for a texture name (ZeroVector MaxBox = native). */
	FVector2D GetFittedSize(const TCHAR* TextureName, FVector2D MaxBox);
}
