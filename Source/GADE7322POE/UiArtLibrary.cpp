#include "UiArtLibrary.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Engine/Texture2D.h"
#include "Styling/SlateTypes.h"

namespace UiArt
{
	UTexture2D* Load(const TCHAR* AssetName)
	{
		if (!AssetName || !*AssetName)
		{
			return nullptr;
		}
		const FString Path = FString::Printf(TEXT("/Game/UI/Art/%s.%s"), AssetName, AssetName);
		UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, *Path);
		if (Tex)
		{
			Tex->ConditionalPostLoad();
			Tex->SetForceMipLevelsToBeResident(30.f);
		}
		return Tex;
	}

	void PreloadCommon()
	{
		static const TCHAR* Names[] = {
			TEXT("playbttn"), TEXT("menubutton"), TEXT("backbttn"), TEXT("gametitle"),
			TEXT("startscreenpanel"), TEXT("blankpanel1"), TEXT("blankpanel2"),
			TEXT("infobttn"), TEXT("xbttn"),
			TEXT("threatbudgetbttn"), TEXT("counterplaybttn"), TEXT("pausepanel"),
		};
		for (const TCHAR* Name : Names)
		{
			Load(Name);
		}
	}

	FVector2D FitNativeSize(UTexture2D* Texture, FVector2D MaxBox)
	{
		if (!Texture)
		{
			return MaxBox;
		}

		const int32 SizeX = Texture->GetSizeX();
		const int32 SizeY = Texture->GetSizeY();
		// Platform data can be 0 on the very first construct — do not lock SizeBoxes to 1x1.
		if (SizeX < 8 || SizeY < 8)
		{
			return MaxBox.IsNearlyZero() ? FVector2D(256.f, 128.f) : MaxBox;
		}

		const float NativeW = static_cast<float>(SizeX);
		const float NativeH = static_cast<float>(SizeY);

		if (MaxBox.X <= KINDA_SMALL_NUMBER && MaxBox.Y <= KINDA_SMALL_NUMBER)
		{
			return FVector2D(NativeW, NativeH);
		}

		float Scale = 1.f;
		if (MaxBox.X > KINDA_SMALL_NUMBER)
		{
			Scale = FMath::Min(Scale, MaxBox.X / NativeW);
		}
		if (MaxBox.Y > KINDA_SMALL_NUMBER)
		{
			Scale = FMath::Min(Scale, MaxBox.Y / NativeH);
		}
		// Never upscale past native — only shrink to fit.
		Scale = FMath::Min(Scale, 1.f);
		return FVector2D(NativeW * Scale, NativeH * Scale);
	}

	FSlateBrush MakeBrush(UTexture2D* Texture, FVector2D Size)
	{
		FSlateBrush Brush;
		if (!Texture)
		{
			return Brush;
		}
		Brush.SetResourceObject(Texture);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.Tiling = ESlateBrushTileType::NoTile;
		Brush.Mirroring = ESlateBrushMirrorType::NoMirror;
		Brush.ImageSize = Size.IsNearlyZero() ? FitNativeSize(Texture, FVector2D::ZeroVector) : Size;
		Brush.TintColor = FSlateColor(FLinearColor::White);
		return Brush;
	}

	void StyleImageButton(UButton* Button, const TCHAR* TextureName, FVector2D MaxSize)
	{
		if (!Button)
		{
			return;
		}

		auto SyncSizeBox = [Button](FVector2D Size)
		{
			if (Size.X < 32.f || Size.Y < 32.f)
			{
				return;
			}
			if (USizeBox* Box = Cast<USizeBox>(Button->GetParent()))
			{
				Box->SetWidthOverride(Size.X);
				Box->SetHeightOverride(Size.Y);
			}
		};

		UTexture2D* Tex = Load(TextureName);
		if (!Tex)
		{
			// Still reserve layout space so a late texture load cannot leave a tiny Size Box.
			SyncSizeBox(MaxSize);
			return;
		}

		FVector2D Size = FitNativeSize(Tex, MaxSize);
		if (Size.X < 32.f || Size.Y < 32.f)
		{
			Size = MaxSize;
		}

		FButtonStyle Style = Button->GetStyle();
		const FSlateBrush Normal = MakeBrush(Tex, Size);
		Style.SetNormal(Normal);

		FSlateBrush Hovered = Normal;
		Hovered.TintColor = FSlateColor(FLinearColor(1.12f, 1.08f, 1.15f, 1.f));
		Style.SetHovered(Hovered);

		FSlateBrush Pressed = Normal;
		Pressed.TintColor = FSlateColor(FLinearColor(0.8f, 0.8f, 0.88f, 1.f));
		Style.SetPressed(Pressed);

		FSlateBrush Disabled = Normal;
		Disabled.TintColor = FSlateColor(FLinearColor(0.4f, 0.4f, 0.45f, 0.6f));
		Style.SetDisabled(Disabled);

		Style.NormalPadding = FMargin(0.f);
		Style.PressedPadding = FMargin(0.f);
		Button->SetStyle(Style);

		SyncSizeBox(Size);
	}

	void StyleBorderImage(UBorder* Border, const TCHAR* TextureName, FVector2D MaxSize)
	{
		if (!Border)
		{
			return;
		}
		UTexture2D* Tex = Load(TextureName);
		if (!Tex)
		{
			Border->SetBrushColor(FLinearColor(0.08f, 0.11f, 0.18f, 0.97f));
			return;
		}
		const FVector2D Size = FitNativeSize(Tex, MaxSize);
		FSlateBrush Brush = MakeBrush(Tex, Size);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Border->SetBrush(Brush);
		Border->SetBrushColor(FLinearColor::White);

		if (!MaxSize.IsNearlyZero())
		{
			if (USizeBox* Box = Cast<USizeBox>(Border->GetParent()))
			{
				Box->SetWidthOverride(Size.X);
				Box->SetHeightOverride(Size.Y);
			}
		}
	}

	void StyleImage(UImage* Image, const TCHAR* TextureName, FVector2D MaxSize)
	{
		if (!Image)
		{
			return;
		}
		UTexture2D* Tex = Load(TextureName);
		if (!Tex)
		{
			return;
		}
		const FVector2D Size = FitNativeSize(Tex, MaxSize);
		Image->SetBrush(MakeBrush(Tex, Size));
		Image->SetDesiredSizeOverride(Size);
		Image->SetColorAndOpacity(FLinearColor::White);
	}

	FVector2D GetFittedSize(const TCHAR* TextureName, FVector2D MaxBox)
	{
		return FitNativeSize(Load(TextureName), MaxBox);
	}
}
