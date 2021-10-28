// Copyright Epic Games, Inc. All Rights Reserved.

#include "Actors/SKGLTFHotspotActor.h"
#include "Actors/SKGLTFCameraActor.h"
#include "Animation/SkeletalMeshActor.h"
#include "Animation/AnimSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequence.h"
#include "MovieSceneTimeHelpers.h"
#include "Components/SphereComponent.h"
#include "Components/MaterialBillboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Materials/MaterialInstanceDynamic.h"

#if WITH_EDITOR
#include "LevelEditor.h"
#include "Modules/ModuleManager.h"
#include "SLevelViewport.h"
#include "Slate/SceneViewport.h"
#endif

#if !(ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 25)
using namespace UE;
#endif

DEFINE_LOG_CATEGORY_STATIC(LogGLTFHotspot, Log, All);

namespace
{
	const FName NAME_HotspotTag = TEXT("GLTFHotspot");
	const FName NAME_LevelEditorModule = TEXT("LevelEditor");
	const FName NAME_SpriteParameter = TEXT("Sprite");
	const FName NAME_OpacityParameter = TEXT("OpacityMask");
}

ASKGLTFHotspotActor::ASKGLTFHotspotActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	Image(nullptr),
	HoveredImage(nullptr),
	ToggledImage(nullptr),
	ToggledHoveredImage(nullptr),
	BillboardComponent(nullptr),
	SphereComponent(nullptr),
	DefaultMaterial(nullptr),
	DefaultImage(nullptr),
	DefaultHoveredImage(nullptr),
	DefaultToggledImage(nullptr),
	DefaultToggledHoveredImage(nullptr),
	ActiveImage(nullptr),
	ActiveImageSize(0.0f, 0.0f),
	bToggled(false),
	bIsInteractable(true),
	RealtimeSecondsWhenLastInSight(0.0f),
	RealtimeSecondsWhenLastHidden(0.0f)
{
	USceneComponent* SceneRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRootComponent);
	AddInstanceComponent(SceneRootComponent);
	SceneRootComponent->SetMobility(EComponentMobility::Movable);

	BillboardComponent = CreateDefaultSubobject<UMaterialBillboardComponent>(TEXT("MaterialBillboardComponent"));
	AddInstanceComponent(BillboardComponent);
	BillboardComponent->SetupAttachment(RootComponent);
	BillboardComponent->SetMobility(EComponentMobility::Movable);

	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	AddInstanceComponent(BillboardComponent);
	SphereComponent->SetupAttachment(RootComponent);
	SphereComponent->SetMobility(EComponentMobility::Movable);
	SphereComponent->ComponentTags.Add(NAME_HotspotTag);
	SphereComponent->InitSphereRadius(100.0f);
	SphereComponent->SetVisibility(false);

	// Setup the most minimalistic collision profile for mouse input events
	SphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	SphereComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	SphereComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	SphereComponent->SetGenerateOverlapEvents(false);

	// Respond to interactions with the sphere-component
	SphereComponent->OnBeginCursorOver.AddDynamic(this, &ASKGLTFHotspotActor::BeginCursorOver);
	SphereComponent->OnEndCursorOver.AddDynamic(this, &ASKGLTFHotspotActor::EndCursorOver);
	SphereComponent->OnClicked.AddDynamic(this, &ASKGLTFHotspotActor::Clicked);

	DefaultMaterial = nullptr;
	DefaultIconMaterial = nullptr;
	DefaultImage = nullptr;
	DefaultHoveredImage = nullptr;
	DefaultToggledImage = nullptr;
	DefaultToggledHoveredImage = nullptr;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	LevelSequencePlayer = ObjectInitializer.CreateDefaultSubobject<ULevelSequencePlayer>(this, "LevelSequencePlayer");
}


#if WITH_EDITOR
void ASKGLTFHotspotActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FProperty* PropertyThatChanged = PropertyChangedEvent.Property;

	if (PropertyThatChanged)
	{
		const FName PropertyFName = PropertyThatChanged->GetFName();

		if (PropertyFName == GET_MEMBER_NAME_CHECKED(ThisClass, Image))
		{
			UpdateActiveImageFromState(ESKGLTFHotspotState::Default);
		}
		else if (PropertyFName == GET_MEMBER_NAME_CHECKED(ThisClass, SkeletalMeshActor))
		{
			if (SkeletalMeshActor != nullptr && AnimationSequence == nullptr)
			{
				AnimationSequence = Cast<UAnimSequence>(SkeletalMeshActor->GetSkeletalMeshComponent()->AnimationData.AnimToPlay);
			}
			ValidateAnimation();
		}
		else if (PropertyFName == GET_MEMBER_NAME_CHECKED(ThisClass, AnimationSequence))
		{
			ValidateAnimation();
		}
	}
}
#endif // WITH_EDITOR

void ASKGLTFHotspotActor::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();

	SetupSpriteElement();
	UpdateActiveImageFromState(ESKGLTFHotspotState::Default);
}

void ASKGLTFHotspotActor::Tick(float DeltaTime)
{
	UWorld* World = GetWorld();

	// TODO: is is safe to assume that we want the first controller for projections?
	APlayerController* PlayerController = World != nullptr ? World->GetFirstPlayerController() : nullptr;
	if (PlayerController == nullptr)
	{
		return;
	}

	const FVector ColliderLocation = SphereComponent->GetComponentLocation();

	FVector ColliderScreenLocation;
	if (!PlayerController->ProjectWorldLocationToScreenWithDistance(ColliderLocation, ColliderScreenLocation))
	{
		return;
	}

	// Update scale of the sphere-component to match the screen-size of the active image
	{
		const FVector2D CornerScreenLocation = FVector2D(ColliderScreenLocation) + ActiveImageSize * 0.5f;
		FVector RayLocation;
		FVector RayDirection;

		if (PlayerController->DeprojectScreenPositionToWorld(CornerScreenLocation.X, CornerScreenLocation.Y, RayLocation, RayDirection))
		{
			const FVector ExtentLocation = RayLocation + RayDirection * ColliderScreenLocation.Z;
			const float NewSphereRadius = (ExtentLocation - ColliderLocation).Size() / SphereComponent->GetShapeScale();
			const float OldSphereRadius = SphereComponent->GetUnscaledSphereRadius();

			if (FMath::Abs(NewSphereRadius - OldSphereRadius) > 0.1f)	// TODO: better epsilon?
			{
				SphereComponent->SetSphereRadius(NewSphereRadius);
			}
		}
	}

	// Update opacity and interactivity of the hotspot based on if it its occluded by other objects or not
	{
		FHitResult HitResult;
		bool bIsHotspotOccluded = false;

		if (PlayerController->GetHitResultAtScreenPosition(FVector2D(ColliderScreenLocation), ECC_Visibility, false, HitResult))
		{
			if (const UPrimitiveComponent* HitComponent = HitResult.GetComponent())
			{
				bIsHotspotOccluded = !HitComponent->ComponentTags.Contains(NAME_HotspotTag);
			}
		}

		const float CurrentRealtimeSeconds = UGameplayStatics::GetRealTimeSeconds(World);
		float Opacity;

		if (bIsHotspotOccluded)
		{
			RealtimeSecondsWhenLastHidden = CurrentRealtimeSeconds;

			const float HiddenDuration = FMath::Max(RealtimeSecondsWhenLastHidden - RealtimeSecondsWhenLastInSight, 0.0f);
			const float FadeOutDuration = 0.5f;

			Opacity = 1.0f - FMath::Clamp(HiddenDuration / FadeOutDuration, 0.0f, 1.0f);
		}
		else
		{
			RealtimeSecondsWhenLastInSight = CurrentRealtimeSeconds;

			const float VisibleDuration = FMath::Max(RealtimeSecondsWhenLastInSight - RealtimeSecondsWhenLastHidden, 0.0f);
			const float FadeInDuration = 0.25f;

			Opacity = FMath::Clamp(VisibleDuration / FadeInDuration, 0.0f, 1.0f);
		}

		SetSpriteOpacity(Opacity);
		bIsInteractable = Opacity >= 0.5f;
	}

	if (LevelSequencePlayer != nullptr)
	{
		LevelSequencePlayer->Update(DeltaTime);
	}
}

void ASKGLTFHotspotActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (LevelSequencePlayer != nullptr)
	{
		// Stop may modify a lot of actor state so it needs to be called
		// during EndPlay (when Actors + World are still valid) instead
		// of waiting for the UObject to be destroyed by GC.
		LevelSequencePlayer->Stop();
	}
}

void ASKGLTFHotspotActor::BeginCursorOver(UPrimitiveComponent* TouchedComponent)
{
	if (bIsInteractable)
	{
		UpdateActiveImageFromState(bToggled ? ESKGLTFHotspotState::ToggledHovered :  ESKGLTFHotspotState::Hovered);
	}
}

void ASKGLTFHotspotActor::EndCursorOver(UPrimitiveComponent* TouchedComponent)
{
	UpdateActiveImageFromState(bToggled ? ESKGLTFHotspotState::Toggled :  ESKGLTFHotspotState::Default);
}

void ASKGLTFHotspotActor::Clicked(UPrimitiveComponent* TouchedComponent, FKey ButtonPressed)
{
	if (!bIsInteractable)
	{
		return;
	}

	ToggleAnimation();

	UpdateActiveImageFromState(bToggled ? ESKGLTFHotspotState::ToggledHovered :  ESKGLTFHotspotState::Hovered);
}

void ASKGLTFHotspotActor::UpdateActiveImageFromState(ESKGLTFHotspotState State)
{
	UTexture2D* NewImage = const_cast<UTexture2D*>(GetImageForState(State));

	UTexture* DefaultTexture;
	GetSpriteMaterial()->GetTextureParameterDefaultValue(NAME_SpriteParameter, DefaultTexture);

	UTexture* SpriteTexture = NewImage != nullptr ? NewImage : DefaultTexture;
	GetSpriteMaterial()->SetTextureParameterValue(NAME_SpriteParameter, SpriteTexture);

	ActiveImage = NewImage;
	ActiveImageSize = { SpriteTexture->GetSurfaceWidth(), SpriteTexture->GetSurfaceHeight() };

	// NOTE: we do this even if size is unchanged since the last update may have failed
	UpdateSpriteSize();
}

void ASKGLTFHotspotActor::SetupSpriteElement() const
{
	UMaterialInstanceDynamic* MaterialInstance;

#if WITH_EDITORONLY_DATA
	if (GetWorld() != nullptr && GetWorld()->WorldType == EWorldType::Editor)
	{
		MaterialInstance = UMaterialInstanceDynamic::Create(DefaultIconMaterial, GetTransientPackage());
	}
	else
#endif // WITH_EDITORONLY_DATA
	{
		MaterialInstance = UMaterialInstanceDynamic::Create(DefaultMaterial, GetTransientPackage());
	}

	FMaterialSpriteElement Element;
	Element.Material = MaterialInstance;
	Element.bSizeIsInScreenSpace = true;
	Element.BaseSizeX = 0.1f;
	Element.BaseSizeY = 0.1f;

	BillboardComponent->SetElements({ Element });
}

UMaterialInstanceDynamic* ASKGLTFHotspotActor::GetSpriteMaterial() const
{
	return static_cast<UMaterialInstanceDynamic*>(BillboardComponent->GetMaterial(0));
}

void ASKGLTFHotspotActor::UpdateSpriteSize()
{
	const FIntPoint ViewportSize = GetCurrentViewportSize();

	float BaseSizeX = 0.1f;
	float BaseSizeY = 0.1f;

	if (ViewportSize.X > 0 && ViewportSize.Y > 0)
	{
		BaseSizeX = ActiveImageSize.X / static_cast<float>(ViewportSize.X);
		BaseSizeY = ActiveImageSize.Y / static_cast<float>(ViewportSize.Y);
	}

	FMaterialSpriteElement& Element = BillboardComponent->Elements[0];

	if (BaseSizeX != Element.BaseSizeX || BaseSizeY != Element.BaseSizeY)	// TODO: use epsilon for comparison?
	{
		Element.BaseSizeX = BaseSizeX;
		Element.BaseSizeY = BaseSizeY;

		BillboardComponent->MarkRenderStateDirty();
	}
}

void ASKGLTFHotspotActor::SetSpriteOpacity(const float Opacity) const
{
	GetSpriteMaterial()->SetScalarParameterValue(NAME_OpacityParameter, Opacity);
}

FIntPoint ASKGLTFHotspotActor::GetCurrentViewportSize()
{
	// TODO: verify that correct size is calculated in the various play-modes and in the editor

	const FViewport* Viewport = nullptr;

	if (UWorld* World = GetWorld())
	{
		if (World->IsGameWorld())
		{
			if (UGameViewportClient* GameViewportClient = World->GetGameViewport())
			{
				Viewport = GameViewportClient->Viewport;
			}
		}
#if WITH_EDITOR
		else
		{
			if (FModuleManager::Get().IsModuleLoaded(NAME_LevelEditorModule))
			{
				FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>(NAME_LevelEditorModule);

				if (const TSharedPtr<SLevelViewport> ActiveLevelViewport = LevelEditorModule.GetFirstActiveLevelViewport())
				{
					Viewport = ActiveLevelViewport->GetActiveViewport();
				}
			}
		}
#endif
	}

	if (Viewport != nullptr)
	{
		if (!Viewport->ViewportResizedEvent.IsBoundToObject(this))
		{
			Viewport->ViewportResizedEvent.AddUObject(this, &ASKGLTFHotspotActor::ViewportResized);
		}

		return Viewport->GetSizeXY();
	}
	else
	{
		return FIntPoint(0, 0);
	}
}

void ASKGLTFHotspotActor::ViewportResized(FViewport*, uint32)
{
	UpdateSpriteSize();
}

void ASKGLTFHotspotActor::ToggleAnimation()
{
	const bool bReverseAnimation = bToggled;
	const float TargetPlayRate = bToggled ? -1.0f : 1.0f;

	if (SkeletalMeshActor != nullptr && AnimationSequence != nullptr)
	{
		USkeletalMeshComponent* SkeletalMeshComponent = SkeletalMeshActor->GetSkeletalMeshComponent();
		SkeletalMeshComponent->SetAnimationMode(EAnimationMode::Type::AnimationSingleNode);
		const UAnimSingleNodeInstance* SingleNodeInstance = SkeletalMeshComponent->GetSingleNodeInstance();

		if (SkeletalMeshComponent->IsPlaying() && SingleNodeInstance != nullptr && SingleNodeInstance->GetAnimationAsset() == AnimationSequence)
		{
			// If the same animation is already playing, just reverse the play rate for a smooth transition
			SkeletalMeshComponent->SetPlayRate(TargetPlayRate);
		}
		else
		{
			SkeletalMeshComponent->SetAnimation(AnimationSequence);
			SkeletalMeshComponent->SetPlayRate(TargetPlayRate);
			SkeletalMeshComponent->SetPosition(bReverseAnimation ? AnimationSequence->GetPlayLength() : 0.0f);
			SkeletalMeshComponent->Play(false);
		}
	}
	else if (LevelSequence != nullptr)
	{
		if (LevelSequencePlayer->IsPlaying())
		{
			// If the same animation is already playing, just reverse the play rate for a smooth transition
			LevelSequencePlayer->SetPlayRate(TargetPlayRate);
		}
		else
		{
			// TODO: is there a simpler way to get the length (in seconds) of a level sequence?
			const float SequenceLength = LevelSequence->GetMovieScene()->GetTickResolution().AsSeconds(MovieScene::DiscreteSize(LevelSequence->GetMovieScene()->GetPlaybackRange()));

			FMovieSceneSequencePlaybackSettings PlaybackSettings;
			PlaybackSettings.PlayRate = TargetPlayRate;
			PlaybackSettings.StartTime = bReverseAnimation ? SequenceLength : 0.0f;
			PlaybackSettings.bAutoPlay = true;
			PlaybackSettings.bPauseAtEnd = true;
			PlaybackSettings.LoopCount.Value = 0;
			LevelSequencePlayer->Initialize(LevelSequence, GetLevel(), PlaybackSettings, FLevelSequenceCameraSettings());
			LevelSequencePlayer->Play();
		}
	}

	bToggled = !bToggled;
}

void ASKGLTFHotspotActor::ValidateAnimation()
{
	if (SkeletalMeshActor != nullptr && AnimationSequence != nullptr)
	{
		if (USkeletalMesh* SkeletalMesh = SkeletalMeshActor->GetSkeletalMeshComponent()->SkeletalMesh)
		{
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 26
			if (SkeletalMesh->Skeleton != AnimationSequence->GetSkeleton())
			{
				if (SkeletalMesh->Skeleton != nullptr)
				{
					UE_LOG(LogGLTFHotspot, Warning, TEXT("Animation %s is incompatible with skeleton %s, removing animation from actor."), *AnimationSequence->GetName(), *SkeletalMesh->Skeleton->GetName());
				}
#else
			if (SkeletalMesh->GetSkeleton() != AnimationSequence->GetSkeleton())
			{
				if (SkeletalMesh->GetSkeleton() != nullptr)
				{
					UE_LOG(LogGLTFHotspot, Warning, TEXT("Animation %s is incompatible with skeleton %s, removing animation from actor."), *AnimationSequence->GetName(), *SkeletalMesh->GetSkeleton()->GetName());
				}
#endif
				else
				{
					UE_LOG(LogGLTFHotspot, Warning, TEXT("Animation %s is incompatible because mesh %s has no skeleton, removing animation from actor."), *AnimationSequence->GetName(), *SkeletalMesh->GetName());
				}

				AnimationSequence = nullptr;
			}
		}
	}
}

const UTexture2D* ASKGLTFHotspotActor::GetImageForState(ESKGLTFHotspotState State) const
{
	const UTexture2D* CurrentImage = DefaultImage;
	const UTexture2D* CurrentHoveredImage = DefaultHoveredImage;
	const UTexture2D* CurrentToggledImage = DefaultToggledImage;
	const UTexture2D* CurrentToggledHoveredImage = DefaultToggledHoveredImage;

	if (Image != nullptr)
	{
		CurrentImage = Image;
		CurrentHoveredImage = HoveredImage != nullptr ? HoveredImage : CurrentImage;
		CurrentToggledImage = ToggledImage != nullptr ? ToggledImage : CurrentImage;

		if (ToggledHoveredImage != nullptr)
		{
			CurrentToggledHoveredImage = ToggledHoveredImage;
		}
		else if (ToggledImage != nullptr)
		{
			CurrentToggledHoveredImage = ToggledImage;
		}
		else if (HoveredImage != nullptr)
		{
			CurrentToggledHoveredImage = HoveredImage;
		}
		else
		{
			CurrentToggledHoveredImage = Image;
		}
	}

	switch (State)
	{
		case ESKGLTFHotspotState::Default:        return CurrentImage;
		case ESKGLTFHotspotState::Hovered:        return CurrentHoveredImage;
		case ESKGLTFHotspotState::Toggled:        return CurrentToggledImage;
		case ESKGLTFHotspotState::ToggledHovered: return CurrentToggledHoveredImage;
		default:
			checkNoEntry();
			return nullptr;
	}
}
