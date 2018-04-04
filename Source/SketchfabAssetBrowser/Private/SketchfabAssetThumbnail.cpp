// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#include "SketchfabAssetThumbnail.h"
#include "Engine/Blueprint.h"
#include "GameFramework/Actor.h"
#include "Layout/Margin.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SOverlay.h"
#include "Engine/GameViewportClient.h"
#include "Modules/ModuleManager.h"
#include "Animation/CurveHandle.h"
#include "Animation/CurveSequence.h"
#include "Textures/SlateTextureData.h"
#include "Fonts/SlateFontInfo.h"
#include "Application/ThrottleManager.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SViewport.h"
#include "EditorStyleSet.h"
#include "RenderingThread.h"
#include "Settings/ContentBrowserSettings.h"
#include "RenderUtils.h"
#include "Editor/UnrealEdEngine.h"
#include "Editor.h"
#include "UnrealEdGlobals.h"
#include "Slate/SlateTextures.h"
#include "ObjectTools.h"
#include "Styling/SlateIconFinder.h"
#include "ClassIconFinder.h"
#include "IVREditorModule.h"
#include "ImageLoader.h"
#include "SlateDynamicImageBrush.h"

class SSketchfabAssetThumbnail : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSketchfabAssetThumbnail)
		: _Style("SketchfabAssetThumbnail")
		, _ThumbnailPool(NULL)
		, _AllowFadeIn(false)
		, _ForceGenericThumbnail(false)
		, _AllowHintText(true)
		, _AllowAssetSpecificThumbnailOverlay(false)
		, _Label(ESketchfabThumbnailLabel::ClassName)
		, _HighlightedText(FText::GetEmpty())
		, _HintColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f))
		, _ClassThumbnailBrushOverride(NAME_None)
		, _AssetTypeColorOverride()
		{}

		SLATE_ARGUMENT( FName, Style )
		SLATE_ARGUMENT( TSharedPtr<FSketchfabAssetThumbnail>, AssetThumbnail )
		SLATE_ARGUMENT( TSharedPtr<FSketchfabAssetThumbnailPool>, ThumbnailPool )
		SLATE_ARGUMENT( bool, AllowFadeIn )
		SLATE_ARGUMENT( bool, ForceGenericThumbnail )
		SLATE_ARGUMENT( bool, AllowHintText )
		SLATE_ARGUMENT( bool, AllowAssetSpecificThumbnailOverlay )
		SLATE_ARGUMENT( ESketchfabThumbnailLabel::Type, Label )
		SLATE_ATTRIBUTE( FText, HighlightedText )
		SLATE_ATTRIBUTE( FLinearColor, HintColorAndOpacity )
		SLATE_ARGUMENT( FName, ClassThumbnailBrushOverride )
		SLATE_ARGUMENT( TOptional<FLinearColor>, AssetTypeColorOverride )

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct( const FArguments& InArgs )
	{
		Style = InArgs._Style;
		HighlightedText = InArgs._HighlightedText;
		Label = InArgs._Label;
		HintColorAndOpacity = InArgs._HintColorAndOpacity;
		bAllowHintText = InArgs._AllowHintText;

		AssetThumbnail = InArgs._AssetThumbnail;
		bHasRenderedThumbnail = false;
		WidthLastFrame = 0;
		GenericThumbnailBorderPadding = 2.f;

		AssetThumbnail->OnAssetDataChanged().AddSP(this, &SSketchfabAssetThumbnail::OnAssetDataChanged);

		const FSketchfabAssetData& AssetData = AssetThumbnail->GetAssetData();

		/*
		UClass* Class = FindObject<UClass>(ANY_PACKAGE, *AssetData.AssetClass.ToString());
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		TSharedPtr<IAssetTypeActions> AssetTypeActions;
		if ( Class != NULL )
		{
			AssetTypeActions = AssetToolsModule.Get().GetAssetTypeActionsForClass(Class).Pin();
		}
		*/

		AssetColor = FLinearColor::White;

		if( InArgs._AssetTypeColorOverride.IsSet() )
		{
			AssetColor = InArgs._AssetTypeColorOverride.GetValue();
		}
		/*
		else if ( AssetTypeActions.IsValid() )
		{
			AssetColor = AssetTypeActions->GetTypeColor();
		}
		*/

		TSharedRef<SOverlay> OverlayWidget = SNew(SOverlay);

		ClassThumbnailBrushOverride = InArgs._ClassThumbnailBrushOverride;

		// The generic representation of the thumbnail, for use before the rendered version, if it exists
		OverlayWidget->AddSlot()
		[
			SNew(SBorder)
			.BorderImage(GetAssetBackgroundBrush())
			.BorderBackgroundColor(AssetColor.CopyWithNewOpacity(0.3f))
			.Padding(GenericThumbnailBorderPadding)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.Visibility(this, &SSketchfabAssetThumbnail::GetGenericThumbnailVisibility)
			[
				SNew(SOverlay)

				+SOverlay::Slot()
				[
					SAssignNew(GenericLabelTextBlock, STextBlock)
					.Text(GetLabelText())
					.Font(GetTextFont())
					.Justification(ETextJustify::Center)
					.ColorAndOpacity(FEditorStyle::GetColor(Style, ".ColorAndOpacity"))
					.ShadowOffset(FEditorStyle::GetVector(Style, ".ShadowOffset"))
					.ShadowColorAndOpacity( FEditorStyle::GetColor(Style, ".ShadowColorAndOpacity"))
					.HighlightText(HighlightedText)
					.AutoWrapText(true)
				]

				+SOverlay::Slot()
				[
					SAssignNew(GenericThumbnailImage, SImage)
					.Image(this, &SSketchfabAssetThumbnail::GetClassThumbnailBrush)
				]
			]
		];

		//I am not using the SViewport method for rendering. So I have duplicated this code below this to just draw an imagebrush
		/*
		if ( InArgs._ThumbnailPool.IsValid() && !InArgs._ForceGenericThumbnail )
		{
			ViewportFadeAnimation = FCurveSequence();
			ViewportFadeCurve = ViewportFadeAnimation.AddCurve(0.f, 0.25f, ECurveEaseFunction::QuadOut);

			TSharedPtr<SViewport> Viewport = 
				SNew( SViewport )
				.EnableGammaCorrection(false)
				// In VR editor every widget is in the world and gamma corrected by the scene renderer.  Thumbnails will have already been gamma
				// corrected and so they need to be reversed
				.ReverseGammaCorrection(IVREditorModule::Get().IsVREditorModeActive())
				.EnableBlending(true);

			Viewport->SetViewportInterface( AssetThumbnail.ToSharedRef() );
			AssetThumbnail->GetViewportRenderTargetTexture(); // Access the render texture to push it on the stack if it isn't already rendered

			InArgs._ThumbnailPool->OnThumbnailRendered().AddSP(this, &SSketchfabAssetThumbnail::OnThumbnailRendered);
			InArgs._ThumbnailPool->OnThumbnailRenderFailed().AddSP(this, &SSketchfabAssetThumbnail::OnThumbnailRenderFailed);

			if ( ShouldRender() && (!InArgs._AllowFadeIn || InArgs._ThumbnailPool->IsRendered(AssetThumbnail)) )
			{
				bHasRenderedThumbnail = true;
				ViewportFadeAnimation.JumpToEnd();
			}

			// The viewport for the rendered thumbnail, if it exists
			OverlayWidget->AddSlot()
			[
				SAssignNew(RenderedThumbnailWidget, SBorder)
				.Padding(0)
				.BorderImage(FEditorStyle::GetBrush("NoBrush"))
				.ColorAndOpacity(this, &SSketchfabAssetThumbnail::GetViewportColorAndOpacity)
				[
					Viewport.ToSharedRef()
				]
			];
		}
		*/

		if (InArgs._ThumbnailPool.IsValid() && !InArgs._ForceGenericThumbnail)
		{
			ViewportFadeAnimation = FCurveSequence();
			ViewportFadeCurve = ViewportFadeAnimation.AddCurve(0.f, 0.25f, ECurveEaseFunction::QuadOut);

			AssetThumbnail->GetViewportRenderTargetTexture(); // Access the render texture to push it on the stack if it isn't already rendered
			FSlateDynamicImageBrush* ModelImageBrush = AssetThumbnail->GetImageBrush();

			InArgs._ThumbnailPool->OnThumbnailRendered().AddSP(this, &SSketchfabAssetThumbnail::OnThumbnailRendered);
			InArgs._ThumbnailPool->OnThumbnailRenderFailed().AddSP(this, &SSketchfabAssetThumbnail::OnThumbnailRenderFailed);

			if (ShouldRender() && (!InArgs._AllowFadeIn || InArgs._ThumbnailPool->IsRendered(AssetThumbnail)))
			{
				bHasRenderedThumbnail = true;
				ViewportFadeAnimation.JumpToEnd();
			}

			// The viewport for the rendered thumbnail, if it exists
			OverlayWidget->AddSlot()
				[
					SNew(SImage).Image(ModelImageBrush)
				];
		}

		if( bAllowHintText )
		{
			OverlayWidget->AddSlot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Top)
				.Padding(FMargin(2, 2, 2, 2))
				[
					SNew(SBorder)
					.BorderImage(FEditorStyle::GetBrush(Style, ".HintBackground"))
					.BorderBackgroundColor(this, &SSketchfabAssetThumbnail::GetHintBackgroundColor) //Adjust the opacity of the border itself
					.ColorAndOpacity(HintColorAndOpacity) //adjusts the opacity of the contents of the border
					.Visibility(this, &SSketchfabAssetThumbnail::GetHintTextVisibility)
					.Padding(0)
					[
						SAssignNew(HintTextBlock, STextBlock)
						.Text(GetLabelText())
						.Font(GetHintTextFont())
						.ColorAndOpacity(FEditorStyle::GetColor(Style, ".HintColorAndOpacity"))
						.ShadowOffset(FEditorStyle::GetVector(Style, ".HintShadowOffset"))
						.ShadowColorAndOpacity(FEditorStyle::GetColor(Style, ".HintShadowColorAndOpacity"))
						.HighlightText(HighlightedText)
					]
				];
		}

		// The asset color strip
		OverlayWidget->AddSlot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Bottom)
		[
			SAssignNew(AssetColorStripWidget, SBorder)
			.BorderImage(FEditorStyle::GetBrush("WhiteBrush"))
			.BorderBackgroundColor(AssetColor)
			.Padding(this, &SSketchfabAssetThumbnail::GetAssetColorStripPadding)
		];

		// The asset color strip
		OverlayWidget->AddSlot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Bottom)
			[
				SAssignNew(AssetColorStripWidget, SBorder)
				.BorderImage(FEditorStyle::GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FLinearColor::Green)
			.Padding(this, &SSketchfabAssetThumbnail::GetDownloadProgressColorStripPadding)
			];

		/*
		if( InArgs._AllowAssetSpecificThumbnailOverlay && AssetTypeActions.IsValid() )
		{
			// Does the asset provide an additional thumbnail overlay?
			TSharedPtr<SWidget> AssetSpecificThumbnailOverlay = AssetTypeActions->GetThumbnailOverlay(AssetData);
			if( AssetSpecificThumbnailOverlay.IsValid() )
			{
				OverlayWidget->AddSlot()
				[
					AssetSpecificThumbnailOverlay.ToSharedRef()
				];
			}
		}
		*/

		ChildSlot
		[
			OverlayWidget
		];

		UpdateThumbnailVisibilities();

	}

	FSlateColor GetHintBackgroundColor() const
	{
		const FLinearColor Color = HintColorAndOpacity.Get();
		return FSlateColor( FLinearColor( Color.R, Color.G, Color.B, FMath::Lerp( 0.0f, 0.5f, Color.A ) ) );
	}

	// SWidget implementation
	virtual void OnMouseEnter( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override
	{
		SCompoundWidget::OnMouseEnter(MyGeometry, MouseEvent);

		if (!GetDefault<UContentBrowserSettings>()->RealTimeThumbnails )
		{
			// Update hovered thumbnails if we are not already updating them in real-time
			AssetThumbnail->RefreshThumbnail();
		}
	}

	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) override
	{
		if ( WidthLastFrame != AllottedGeometry.Size.X )
		{
			WidthLastFrame = AllottedGeometry.Size.X;

			// The width changed, update the font
			if ( GenericLabelTextBlock.IsValid() )
			{
				GenericLabelTextBlock->SetFont( GetTextFont() );
				GenericLabelTextBlock->SetWrapTextAt( GetTextWrapWidth() );
			}

			if ( HintTextBlock.IsValid() )
			{
				HintTextBlock->SetFont( GetHintTextFont() );
				HintTextBlock->SetWrapTextAt( GetTextWrapWidth() );
			}
		}
	}

private:
	void OnAssetDataChanged()
	{
		if ( GenericLabelTextBlock.IsValid() )
		{
			GenericLabelTextBlock->SetText( GetLabelText() );
		}

		if ( HintTextBlock.IsValid() )
		{
			HintTextBlock->SetText( GetLabelText() );
		}

		// Check if the asset has a thumbnail.
		const FObjectThumbnail* ObjectThumbnail = NULL;
		FThumbnailMap ThumbnailMap;
		if( AssetThumbnail->GetAsset() )
		{
			FName FullAssetName = FName( *(AssetThumbnail->GetAssetData().GetFullName()) );
			TArray<FName> ObjectNames;
			ObjectNames.Add( FullAssetName );
			ThumbnailTools::ConditionallyLoadThumbnailsForObjects(ObjectNames, ThumbnailMap);
			ObjectThumbnail = ThumbnailMap.Find( FullAssetName );
		}

		bHasRenderedThumbnail = ObjectThumbnail && !ObjectThumbnail->IsEmpty();
		ViewportFadeAnimation.JumpToEnd();
		AssetThumbnail->GetViewportRenderTargetTexture(); // Access the render texture to push it on the stack if it isnt already rendered

		const FSketchfabAssetData& AssetData = AssetThumbnail->GetAssetData();


		AssetColor = FLinearColor(1.f, 1.f, 1.f, 1.f);

		/*
		if ( AssetTypeActions.IsValid() )
		{
			AssetColor = AssetTypeActions.Pin()->GetTypeColor();
		}
		*/

		UpdateThumbnailVisibilities();
	}

	FSlateFontInfo GetTextFont() const
	{
		return FEditorStyle::GetFontStyle( WidthLastFrame <= 64 ? FEditorStyle::Join(Style, ".FontSmall") : FEditorStyle::Join(Style, ".Font") );
	}

	FSlateFontInfo GetHintTextFont() const
	{
		return FEditorStyle::GetFontStyle( WidthLastFrame <= 64 ? FEditorStyle::Join(Style, ".HintFontSmall") : FEditorStyle::Join(Style, ".HintFont") );
	}

	float GetTextWrapWidth() const
	{
		return WidthLastFrame - GenericThumbnailBorderPadding * 2.f;
	}

	const FSlateBrush* GetAssetBackgroundBrush() const
	{
		const FName BackgroundBrushName( *(Style.ToString() + TEXT(".AssetBackground")) );
		return FEditorStyle::GetBrush(BackgroundBrushName);
	}

	const FSlateBrush* GetClassBackgroundBrush() const
	{
		const FName BackgroundBrushName( *(Style.ToString() + TEXT(".ClassBackground")) );
		return FEditorStyle::GetBrush(BackgroundBrushName);
	}

	FSlateColor GetViewportBorderColorAndOpacity() const
	{
		return FLinearColor(AssetColor.R, AssetColor.G, AssetColor.B, ViewportFadeCurve.GetLerp());
	}

	FLinearColor GetViewportColorAndOpacity() const
	{
		return FLinearColor(1, 1, 1, ViewportFadeCurve.GetLerp());
	}
	
	EVisibility GetViewportVisibility() const
	{
		return bHasRenderedThumbnail ? EVisibility::Visible : EVisibility::Collapsed;
	}

	float GetAssetColorStripHeight() const
	{
		// The strip is 2.5% the height of the thumbnail, but at least 4 units tall
		return FMath::Max(FMath::CeilToFloat(WidthLastFrame*0.025f), 4.0f);
	}

	FMargin GetAssetColorStripPadding() const
	{
		const float Height = GetAssetColorStripHeight();
		return FMargin(0,Height,0,0);
	}

	FMargin GetDownloadProgressColorStripPadding() const
	{
		const FSketchfabAssetData& AssetData = AssetThumbnail->GetAssetData();
		const float Height = GetAssetColorStripHeight();
		float progress = AssetData.DownloadProgress;

		//This is a hack for now. Since I am currently refreshing all the widgets the WidthLastFrame
		//value can't be used since it goes to 0 when rebuilt. But since I know the size of the widgets is
		//hard coded to 128 I can safely use this value.
		float Width = FMath::Max(FMath::CeilToFloat(WidthLastFrame*0.025f), 128.0f);
		Width = Width * progress;
		return FMargin(Width, Height, 0, 0);
	}


	const FSlateBrush* GetClassThumbnailBrush() const
	{
		return FClassIconFinder::FindThumbnailForClass(nullptr, ClassThumbnailBrushOverride);
	}

	EVisibility GetClassThumbnailVisibility() const
	{
		if(!bHasRenderedThumbnail)
		{
			const FSlateBrush* ClassThumbnailBrush = GetClassThumbnailBrush();
			if( ClassThumbnailBrush)
			{
				return EVisibility::Visible;
			}
		}

		return EVisibility::Collapsed;
	}

	EVisibility GetGenericThumbnailVisibility() const
	{
		return (bHasRenderedThumbnail && ViewportFadeAnimation.IsAtEnd()) ? EVisibility::Collapsed : EVisibility::Visible;
	}

	FMargin GetClassIconPadding() const
	{
		const float Height = GetAssetColorStripHeight();
		return FMargin(0,0,0,Height);
	}

	EVisibility GetHintTextVisibility() const
	{
		if ( bAllowHintText && ( bHasRenderedThumbnail || !GenericLabelTextBlock.IsValid() ) && HintColorAndOpacity.Get().A > 0 )
		{
			return EVisibility::Visible;
		}

		return EVisibility::Collapsed;
	}

	void OnThumbnailRendered(const FSketchfabAssetData& AssetData)
	{
		if ( !bHasRenderedThumbnail && AssetData == AssetThumbnail->GetAssetData() && ShouldRender() )
		{
			OnRenderedThumbnailChanged( true );
			ViewportFadeAnimation.Play( this->AsShared() );
		}
	}

	void OnThumbnailRenderFailed(const FSketchfabAssetData& AssetData)
	{
		if ( bHasRenderedThumbnail && AssetData == AssetThumbnail->GetAssetData() )
		{
			OnRenderedThumbnailChanged( false );
		}
	}

	bool ShouldRender() const
	{
		const FSketchfabAssetData& AssetData = AssetThumbnail->GetAssetData();

		// Never render a thumbnail for an invalid asset
		if ( !AssetData.IsValid() )
		{
			return false;
		}

		if( AssetData.IsAssetLoaded() )
		{
			// Loaded asset, return true if there is a rendering info for it
			/*
			UObject* Asset = AssetData.GetAsset();
			FThumbnailRenderingInfo* RenderInfo = GUnrealEd->GetThumbnailManager()->GetRenderingInfo( Asset );
			if ( RenderInfo != NULL && RenderInfo->Renderer != NULL )
			{
				return true;
			}
			*/
		}

		// Just render it. Ideally we would look for it in the render pool.
		if (AssetThumbnail->GetImageBrush())
		{
			return true;
		}

		/*
		const FObjectThumbnail* CachedThumbnail = ThumbnailTools::FindCachedThumbnail(*AssetData.GetFullName());
		if ( CachedThumbnail != NULL )
		{
			// There is a cached thumbnail for this asset, we should render it
			return !CachedThumbnail->IsEmpty();
		}
		if ( AssetData.AssetClass != UBlueprint::StaticClass()->GetFName() )
		{
			// If we are not a blueprint, see if the CDO of the asset's class has a rendering info
			// Blueprints can't do this because the rendering info is based on the generated class
			UClass* AssetClass = FindObject<UClass>(ANY_PACKAGE, *AssetData.AssetClass.ToString());

			if ( AssetClass )
			{
				FThumbnailRenderingInfo* RenderInfo = GUnrealEd->GetThumbnailManager()->GetRenderingInfo( AssetClass->GetDefaultObject() );
				if ( RenderInfo != NULL && RenderInfo->Renderer != NULL )
				{
					return true;
				}
			}
		}
		*/
		
		// Unloaded blueprint or asset that may have a custom thumbnail, check to see if there is a thumbnail in the package to render
		/*
		FString PackageFilename;
		if ( FPackageName::DoesPackageExist(AssetData.PackageName.ToString(), NULL, &PackageFilename) )
		{
			TSet<FName> ObjectFullNames;
			FThumbnailMap ThumbnailMap;

			FName ObjectFullName = FName(*AssetData.GetFullName());
			ObjectFullNames.Add(ObjectFullName);

			ThumbnailTools::LoadThumbnailsFromPackage(PackageFilename, ObjectFullNames, ThumbnailMap);

			const FObjectThumbnail* ThumbnailPtr = ThumbnailMap.Find(ObjectFullName);
			if (ThumbnailPtr)
			{
				const FObjectThumbnail& ObjectThumbnail = *ThumbnailPtr;
				return ObjectThumbnail.GetImageWidth() > 0 && ObjectThumbnail.GetImageHeight() > 0 && ObjectThumbnail.GetUncompressedImageData().Num() > 0;
			}
		}
		*/

		return false;
	}

	FText GetLabelText() const
	{
		if( Label != ESketchfabThumbnailLabel::NoLabel )
		{
			if ( Label == ESketchfabThumbnailLabel::AssetName ) 
			{
				return GetAssetDisplayName();
			}
		}
		return FText::GetEmpty();
	}

	FText GetDisplayNameForClass( UClass* Class ) const
	{
		FText ClassDisplayName;
		if ( Class )
		{
			if ( ClassDisplayName.IsEmpty() )
			{
				ClassDisplayName = FText::FromString( FName::NameToDisplayString(*Class->GetName(), false) );
			}
		}

		return ClassDisplayName;
	}

	FText GetAssetDisplayName() const
	{
		const FSketchfabAssetData& AssetData = AssetThumbnail->GetAssetData();
		return FText::FromName(AssetData.AssetName);
	}

	void OnRenderedThumbnailChanged( bool bInHasRenderedThumbnail )
	{
		bHasRenderedThumbnail = bInHasRenderedThumbnail;

		UpdateThumbnailVisibilities();
	}

	void UpdateThumbnailVisibilities()
	{
		// Either the generic label or thumbnail should be shown, but not both at once
		const EVisibility ClassThumbnailVisibility = GetClassThumbnailVisibility();
		if( GenericThumbnailImage.IsValid() )
		{
			GenericThumbnailImage->SetVisibility( ClassThumbnailVisibility );
		}
		if( GenericLabelTextBlock.IsValid() )
		{
			GenericLabelTextBlock->SetVisibility( (ClassThumbnailVisibility == EVisibility::Visible) ? EVisibility::Collapsed : EVisibility::Visible );
		}

		const EVisibility ViewportVisibility = GetViewportVisibility();
		if( RenderedThumbnailWidget.IsValid() )
		{
			RenderedThumbnailWidget->SetVisibility( ViewportVisibility );
			if( ClassIconWidget.IsValid() )
			{
				ClassIconWidget->SetVisibility( ViewportVisibility );
			}
		}
	}

private:
	TSharedPtr<STextBlock> GenericLabelTextBlock;
	TSharedPtr<STextBlock> HintTextBlock;
	TSharedPtr<SImage> GenericThumbnailImage;
	TSharedPtr<SBorder> ClassIconWidget;
	TSharedPtr<SBorder> RenderedThumbnailWidget;
	TSharedPtr<SBorder> AssetColorStripWidget;
	TSharedPtr<FSketchfabAssetThumbnail> AssetThumbnail;
	FCurveSequence ViewportFadeAnimation;
	FCurveHandle ViewportFadeCurve;

	FLinearColor AssetColor;
	float WidthLastFrame;
	float GenericThumbnailBorderPadding;
	bool bHasRenderedThumbnail;
	FName Style;
	TAttribute< FText > HighlightedText;
	ESketchfabThumbnailLabel::Type Label;

	TAttribute< FLinearColor > HintColorAndOpacity;
	bool bAllowHintText;

	/** The name of the thumbnail which should be used instead of the class thumbnail. */
	FName ClassThumbnailBrushOverride;
};

FSketchfabAssetThumbnail::FSketchfabAssetThumbnail( const FSketchfabAssetData& InAssetData , uint32 InWidth, uint32 InHeight, const TSharedPtr<class FSketchfabAssetThumbnailPool>& InThumbnailPool )
	: ThumbnailPool( InThumbnailPool )
	, AssetData ( InAssetData )
	, Width( InWidth )
	, Height( InHeight )
{
	if ( InThumbnailPool.IsValid() )
	{
		InThumbnailPool->AddReferencer(*this);
	}
}

FSketchfabAssetThumbnail::~FSketchfabAssetThumbnail()
{
	if ( ThumbnailPool.IsValid() )
	{
		ThumbnailPool.Pin()->RemoveReferencer(*this);
	}
}

FIntPoint FSketchfabAssetThumbnail::GetSize() const
{
	return FIntPoint( Width, Height );
}

FSlateShaderResource* FSketchfabAssetThumbnail::GetViewportRenderTargetTexture() const
{
	FSlateTexture2DRHIRef* Texture = NULL;
	if ( ThumbnailPool.IsValid() )
	{
		Texture = ThumbnailPool.Pin()->AccessTexture( AssetData, Width, Height, NULL );
	}
	if( !Texture || !Texture->IsValid() )
	{
		return NULL;
	}

	return Texture;
}

FSlateDynamicImageBrush* FSketchfabAssetThumbnail::GetImageBrush() const
{
	FSlateDynamicImageBrush* Texture = NULL;
	if (ThumbnailPool.IsValid())
	{
		ThumbnailPool.Pin()->AccessTexture(AssetData, Width, Height, &Texture);
	}
	return Texture;
}

UObject* FSketchfabAssetThumbnail::GetAsset() const
{
	if ( AssetData.ModelUID != NAME_None )
	{
		return FindObject<UObject>(NULL, *AssetData.ModelUID.ToString());
	}
	else
	{
		return NULL;
	}
}

const FSketchfabAssetData& FSketchfabAssetThumbnail::GetAssetData() const
{
	return AssetData;
}

void FSketchfabAssetThumbnail::SetAsset( const FSketchfabAssetData& InAssetData )
{
	if ( ThumbnailPool.IsValid() )
	{
		ThumbnailPool.Pin()->RemoveReferencer(*this);
	}

	if ( InAssetData.IsValid() )
	{
		AssetData = InAssetData;
		if ( ThumbnailPool.IsValid() )
		{
			ThumbnailPool.Pin()->AddReferencer(*this);
		}
	}
	else
	{
		AssetData = FSketchfabAssetData();
	}

	AssetDataChangedEvent.Broadcast();
}

TSharedRef<SWidget> FSketchfabAssetThumbnail::MakeThumbnailWidget( const FSketchfabAssetThumbnailConfig& InConfig )
{
	return
		SNew(SSketchfabAssetThumbnail)
		.AssetThumbnail( SharedThis(this) )
		.ThumbnailPool( ThumbnailPool.Pin() )
		.AllowFadeIn( InConfig.bAllowFadeIn )
		.ForceGenericThumbnail( InConfig.bForceGenericThumbnail )
		.Label( InConfig.ThumbnailLabel )
		.HighlightedText( InConfig.HighlightedText )
		.HintColorAndOpacity( InConfig.HintColorAndOpacity )
		.AllowHintText( InConfig.bAllowHintText )
		.ClassThumbnailBrushOverride( InConfig.ClassThumbnailBrushOverride )
		.AllowAssetSpecificThumbnailOverlay( InConfig.bAllowAssetSpecificThumbnailOverlay )
		.AssetTypeColorOverride( InConfig.AssetTypeColorOverride );
}

void FSketchfabAssetThumbnail::RefreshThumbnail()
{
	if ( ThumbnailPool.IsValid() && AssetData.IsValid() )
	{
		ThumbnailPool.Pin()->RefreshThumbnail( SharedThis(this) );
	}
}

void FSketchfabAssetThumbnail::SetProgress(float progress)
{
	AssetData.DownloadProgress = progress;
}

//==============================================================

FSketchfabAssetThumbnailPool::FSketchfabAssetThumbnailPool( uint32 InNumInPool, const TAttribute<bool>& InAreRealTimeThumbnailsAllowed, double InMaxFrameTimeAllowance, uint32 InMaxRealTimeThumbnailsPerFrame )
	: AreRealTimeThumbnailsAllowed( InAreRealTimeThumbnailsAllowed )
	, NumInPool( InNumInPool )
	, MaxRealTimeThumbnailsPerFrame( InMaxRealTimeThumbnailsPerFrame )
	, MaxFrameTimeAllowance( InMaxFrameTimeAllowance )
{
}

FSketchfabAssetThumbnailPool::~FSketchfabAssetThumbnailPool()
{
	// Release all the texture resources
	ReleaseResources();
}

FSketchfabAssetThumbnailPool::FThumbnailInfo::~FThumbnailInfo()
{
	if( ThumbnailTexture )
	{
		delete ThumbnailTexture;
		ThumbnailTexture = NULL;
	}

	if( ThumbnailRenderTarget )
	{
		delete ThumbnailRenderTarget;
		ThumbnailRenderTarget = NULL;
	}

	if (ModelImageBrush)
	{
		delete ModelImageBrush;
		ModelImageBrush = NULL;
	}


	/*
	//This is a UTexture that was created via NewObject(). I am unsure how to free this data at this point.
	if (ModelTexture)
	{
		delete ModelTexture;
		ModelTexture = NULL;
	}
	*/
}

void FSketchfabAssetThumbnailPool::ReleaseResources()
{
	// Clear all pending render requests
	ThumbnailsToRenderStack.Empty();

	TArray< TSharedRef<FThumbnailInfo> > ThumbnailsToRelease;

	for( auto ThumbIt = ThumbnailToTextureMap.CreateConstIterator(); ThumbIt; ++ThumbIt )
	{
		ThumbnailsToRelease.Add(ThumbIt.Value());
	}
	ThumbnailToTextureMap.Empty();

	for( auto ThumbIt = FreeThumbnails.CreateConstIterator(); ThumbIt; ++ThumbIt )
	{
		ThumbnailsToRelease.Add(*ThumbIt);
	}
	FreeThumbnails.Empty();

	for ( auto ThumbIt = ThumbnailsToRelease.CreateConstIterator(); ThumbIt; ++ThumbIt )
	{
		const TSharedRef<FThumbnailInfo>& Thumb = *ThumbIt;

			// Release rendering resources
			ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER( 
				ReleaseThumbnailResources,
				FThumbnailInfo_RenderThread, ThumbInfo, Thumb.Get(),
			{
				ThumbInfo.ThumbnailTexture->ClearTextureData();
				ThumbInfo.ThumbnailTexture->ReleaseResource();
				ThumbInfo.ThumbnailRenderTarget->ReleaseResource();
			});
		}

	// Wait for all resources to be released
	FlushRenderingCommands();

	// Make sure there are no more references to any of our thumbnails now that rendering commands have been flushed
	for ( auto ThumbIt = ThumbnailsToRelease.CreateConstIterator(); ThumbIt; ++ThumbIt )
	{
		const TSharedRef<FThumbnailInfo>& Thumb = *ThumbIt;
		if ( !Thumb.IsUnique() )
		{
			ensureMsgf(0, TEXT("Thumbnail info for '%s' is still referenced by '%d' other objects"), *Thumb->AssetData.ModelUID.ToString(), Thumb.GetSharedReferenceCount());
		}
	}
}

TStatId FSketchfabAssetThumbnailPool::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FSketchfabAssetThumbnailPool, STATGROUP_Tickables );
}

bool FSketchfabAssetThumbnailPool::IsTickable() const
{
	return false;
}

void FSketchfabAssetThumbnailPool::Tick( float DeltaTime )
{
}

FSlateTexture2DRHIRef* FSketchfabAssetThumbnailPool::AccessTexture( const FSketchfabAssetData& AssetData, uint32 Width, uint32 Height, FSlateDynamicImageBrush **Image)
{
	const FName &ThumbUID = AssetData.ThumbUID;
	const FName &PackagePath = AssetData.ContentFolder;

	FString path = PackagePath.ToString() / ThumbUID.ToString();
	path += ".jpg";

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.FileExists(*path))
	{
		return NULL;
	}

	if(AssetData.ModelUID == NAME_None || Width == 0 || Height == 0)
	{
		return NULL;
	}
	else
	{
		FThumbId ThumbId( AssetData.ModelUID, Width, Height ) ;
		// Check to see if a thumbnail for this asset exists.  If so we don't need to render it
		const TSharedRef<FThumbnailInfo>* ThumbnailInfoPtr = ThumbnailToTextureMap.Find( ThumbId );
		TSharedPtr<FThumbnailInfo> ThumbnailInfo;
		if( ThumbnailInfoPtr )
		{
			ThumbnailInfo = *ThumbnailInfoPtr;
		}
		else
		{
			// If a the max number of thumbnails allowed by the pool exists then reuse its rendering resource for the new thumbnail
			if( FreeThumbnails.Num() == 0 && ThumbnailToTextureMap.Num() == NumInPool )
			{
				// Find the thumbnail which was accessed last and use it for the new thumbnail
				float LastAccessTime = FLT_MAX;
				const FThumbId* AssetToRemove = NULL;
				for( TMap< FThumbId, TSharedRef<FThumbnailInfo> >::TConstIterator It(ThumbnailToTextureMap); It; ++It )
				{
					if( It.Value()->LastAccessTime < LastAccessTime )
					{
						LastAccessTime = It.Value()->LastAccessTime;
						AssetToRemove = &It.Key();
					}
				}

				check( AssetToRemove );

				// Remove the old mapping 
				ThumbnailInfo = ThumbnailToTextureMap.FindAndRemoveChecked( *AssetToRemove );
			}
			else if( FreeThumbnails.Num() > 0 )
			{
				ThumbnailInfo = FreeThumbnails.Pop();

				ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER( SlateUpdateThumbSizeCommand,
					FSlateTextureRenderTarget2DResource*, ThumbnailRenderTarget, ThumbnailInfo->ThumbnailRenderTarget,
					uint32, Width, Width,
					uint32, Height, Height,
				{
					ThumbnailRenderTarget->SetSize(Width, Height);
				});

				ThumbnailInfo->ModelTexture = UImageLoader::LoadImageFromDisk(NULL, path);
				ThumbnailInfo->ModelImageBrush = new FSlateDynamicImageBrush(ThumbnailInfo->ModelTexture, FVector2D(Width, Height), ThumbUID);
			}
			else
			{
				// There are no free thumbnail resources
				check( (uint32)ThumbnailToTextureMap.Num() <= NumInPool );
				check( !ThumbnailInfo.IsValid() );
				// The pool isn't used up so just make a new texture 

				// Make new thumbnail info if it doesn't exist
				// This happens when the pool is not yet full
				ThumbnailInfo = MakeShareable( new FThumbnailInfo );
				
				// Set the thumbnail and asset on the info. It is NOT safe to change or NULL these pointers until ReleaseResources.
				ThumbnailInfo->ThumbnailTexture = new FSlateTexture2DRHIRef( Width, Height, PF_B8G8R8A8, NULL, TexCreate_Dynamic );
				ThumbnailInfo->ThumbnailRenderTarget = new FSlateTextureRenderTarget2DResource(FLinearColor::Black, Width, Height, PF_B8G8R8A8, SF_Point, TA_Wrap, TA_Wrap, 0.0f);

				// Set the thumbnail and asset on the info. It is NOT safe to change or NULL these pointers until ReleaseResources.

				ThumbnailInfo->ModelTexture = UImageLoader::LoadImageFromDisk(NULL, path);
				ThumbnailInfo->ModelImageBrush = new FSlateDynamicImageBrush(ThumbnailInfo->ModelTexture, FVector2D(Width, Height), ThumbUID);

				BeginInitResource( ThumbnailInfo->ThumbnailTexture );
				BeginInitResource( ThumbnailInfo->ThumbnailRenderTarget );
			}


			check( ThumbnailInfo.IsValid() );
			TSharedRef<FThumbnailInfo> ThumbnailRef = ThumbnailInfo.ToSharedRef();

			// Map the object to its thumbnail info
			ThumbnailToTextureMap.Add( ThumbId, ThumbnailRef );

			ThumbnailInfo->AssetData = AssetData;
			ThumbnailInfo->Width = Width;
			ThumbnailInfo->Height = Height;
		
			// Mark this thumbnail as needing to be updated
			ThumbnailInfo->LastUpdateTime = -1.f;

			// Request that the thumbnail be rendered as soon as possible
			ThumbnailsToRenderStack.Push( ThumbnailRef );
		}

		// This thumbnail was accessed, update its last time to the current time
		// We'll use LastAccessTime to determine the order to recycle thumbnails if the pool is full
		ThumbnailInfo->LastAccessTime = FPlatformTime::Seconds();

		if (Image)
		{
			*Image = ThumbnailInfo->ModelImageBrush;
		}

		return ThumbnailInfo->ThumbnailTexture;
	}
}

void FSketchfabAssetThumbnailPool::AddReferencer( const FSketchfabAssetThumbnail& AssetThumbnail )
{
	FIntPoint Size = AssetThumbnail.GetSize();
	if ( AssetThumbnail.GetAssetData().ModelUID == NAME_None || Size.X == 0 || Size.Y == 0 )
	{
		// Invalid referencer
		return;
	}

	// Generate a key and look up the number of references in the RefCountMap
	FThumbId ThumbId( AssetThumbnail.GetAssetData().ModelUID, Size.X, Size.Y ) ;
	int32* RefCountPtr = RefCountMap.Find(ThumbId);

	if ( RefCountPtr )
	{
		// Already in the map, increment a reference
		(*RefCountPtr)++;
	}
	else
	{
		// New referencer, add it to the map with a RefCount of 1
		RefCountMap.Add(ThumbId, 1);
	}
}

void FSketchfabAssetThumbnailPool::RemoveReferencer( const FSketchfabAssetThumbnail& AssetThumbnail )
{
	FIntPoint Size = AssetThumbnail.GetSize();
	const FName ModelUID = AssetThumbnail.GetAssetData().ModelUID;
	if (ModelUID == NAME_None || Size.X == 0 || Size.Y == 0 )
	{
		// Invalid referencer
		return;
	}

	// Generate a key and look up the number of references in the RefCountMap
	FThumbId ThumbId(ModelUID, Size.X, Size.Y ) ;
	int32* RefCountPtr = RefCountMap.Find(ThumbId);

	// This should complement an AddReferencer so this entry should be in the map
	if ( RefCountPtr )
	{
		// Decrement the RefCount
		(*RefCountPtr)--;

		// If we reached zero, free the thumbnail and remove it from the map.
		if ( (*RefCountPtr) <= 0 )
		{
			RefCountMap.Remove(ThumbId);
			FreeThumbnail(ModelUID, Size.X, Size.Y);
		}
	}
	else
	{
		// This FSketchfabAssetThumbnail did not reference anything or was deleted after the pool was deleted.
	}
}

bool FSketchfabAssetThumbnailPool::IsInRenderStack( const TSharedPtr<FSketchfabAssetThumbnail>& Thumbnail ) const
{
	const FSketchfabAssetData& AssetData = Thumbnail->GetAssetData();
	const uint32 Width = Thumbnail->GetSize().X;
	const uint32 Height = Thumbnail->GetSize().Y;

	if ( ensure(AssetData.ModelUID != NAME_None) && ensure(Width > 0) && ensure(Height > 0) )
	{
		FThumbId ThumbId( AssetData.ModelUID, Width, Height ) ;
		const TSharedRef<FThumbnailInfo>* ThumbnailInfoPtr = ThumbnailToTextureMap.Find( ThumbId );
		if ( ThumbnailInfoPtr )
		{
			return ThumbnailsToRenderStack.Contains(*ThumbnailInfoPtr);
		}
	}

	return false;
}

bool FSketchfabAssetThumbnailPool::IsRendered(const TSharedPtr<FSketchfabAssetThumbnail>& Thumbnail) const
{
	const FSketchfabAssetData& AssetData = Thumbnail->GetAssetData();
	const uint32 Width = Thumbnail->GetSize().X;
	const uint32 Height = Thumbnail->GetSize().Y;

	if (ensure(AssetData.ModelUID != NAME_None) && ensure(Width > 0) && ensure(Height > 0))
	{
		FThumbId ThumbId(AssetData.ModelUID, Width, Height);
		const TSharedRef<FThumbnailInfo>* ThumbnailInfoPtr = ThumbnailToTextureMap.Find(ThumbId);
		if (ThumbnailInfoPtr)
		{
			return (*ThumbnailInfoPtr).Get().LastUpdateTime >= 0.f;
		}
	}

	return false;
}

void FSketchfabAssetThumbnailPool::PrioritizeThumbnails( const TArray< TSharedPtr<FSketchfabAssetThumbnail> >& ThumbnailsToPrioritize, uint32 Width, uint32 Height )
{
	if ( ensure(Width > 0) && ensure(Height > 0) )
	{
		TSet<FName> ObjectPathList;
		for ( int32 ThumbIdx = 0; ThumbIdx < ThumbnailsToPrioritize.Num(); ++ThumbIdx )
		{
			ObjectPathList.Add(ThumbnailsToPrioritize[ThumbIdx]->GetAssetData().ModelUID);
		}

		TArray< TSharedRef<FThumbnailInfo> > FoundThumbnails;
		for ( int32 ThumbIdx = ThumbnailsToRenderStack.Num() - 1; ThumbIdx >= 0; --ThumbIdx )
		{
			const TSharedRef<FThumbnailInfo>& ThumbnailInfo = ThumbnailsToRenderStack[ThumbIdx];
			if ( ThumbnailInfo->Width == Width && ThumbnailInfo->Height == Height && ObjectPathList.Contains(ThumbnailInfo->AssetData.ModelUID) )
			{
				FoundThumbnails.Add(ThumbnailInfo);
				ThumbnailsToRenderStack.RemoveAt(ThumbIdx);
			}
		}

		for ( int32 ThumbIdx = 0; ThumbIdx < FoundThumbnails.Num(); ++ThumbIdx )
		{
			ThumbnailsToRenderStack.Push(FoundThumbnails[ThumbIdx]);
		}
	}
}

void FSketchfabAssetThumbnailPool::RefreshThumbnail( const TSharedPtr<FSketchfabAssetThumbnail>& ThumbnailToRefresh )
{
	const FSketchfabAssetData& AssetData = ThumbnailToRefresh->GetAssetData();
	const uint32 Width = ThumbnailToRefresh->GetSize().X;
	const uint32 Height = ThumbnailToRefresh->GetSize().Y;

	if ( ensure(AssetData.ModelUID != NAME_None) && ensure(Width > 0) && ensure(Height > 0) )
	{
		FThumbId ThumbId( AssetData.ModelUID, Width, Height ) ;
		const TSharedRef<FThumbnailInfo>* ThumbnailInfoPtr = ThumbnailToTextureMap.Find( ThumbId );
		if ( ThumbnailInfoPtr )
		{
			ThumbnailsToRenderStack.AddUnique(*ThumbnailInfoPtr);
		}
	}
}

void FSketchfabAssetThumbnailPool::FreeThumbnail( const FName& ModelUID, uint32 Width, uint32 Height )
{
	if(ModelUID != NAME_None && Width != 0 && Height != 0)
	{
		FThumbId ThumbId(ModelUID, Width, Height ) ;

		const TSharedRef<FThumbnailInfo>* ThumbnailInfoPtr = ThumbnailToTextureMap.Find(ThumbId);
		if( ThumbnailInfoPtr )
		{
			TSharedRef<FThumbnailInfo> ThumbnailInfo = *ThumbnailInfoPtr;
			ThumbnailToTextureMap.Remove(ThumbId);
			ThumbnailsToRenderStack.Remove(ThumbnailInfo);

			if (ThumbnailInfo->ModelImageBrush)
			{
				delete ThumbnailInfo->ModelImageBrush;
				ThumbnailInfo->ModelImageBrush = NULL;
			}

			ThumbnailInfo->ModelTexture = NULL;

			ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER( 
				ReleaseThumbnailTextureData,
				FSlateTexture2DRHIRef*, ThumbnailTexture, ThumbnailInfo->ThumbnailTexture,
			{
				ThumbnailTexture->ClearTextureData();
			});

			FreeThumbnails.Add( ThumbnailInfo );
		}
	}
			
}

void FSketchfabAssetThumbnailPool::RefreshThumbnailsFor( FName ModelUID)
{
	for ( auto ThumbIt = ThumbnailToTextureMap.CreateIterator(); ThumbIt; ++ThumbIt)
	{
		if ( ThumbIt.Key().ModelUID == ModelUID)
		{
			ThumbnailsToRenderStack.Push( ThumbIt.Value() );
		}
	}
}
