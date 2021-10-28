// Copyright Epic Games, Inc. All Rights Reserved.

#include "Tasks/SKGLTFAnimationTasks.h"
#include "Builders/SKGLTFContainerBuilder.h"
#include "Converters/SKGLTFConverterUtility.h"
#include "Converters/SKGLTFBoneUtility.h"
#include "LevelSequence.h"
#include "LevelSequencePlayer.h"
#include "MovieSceneTimeHelpers.h"
#include "Tracks/MovieScene3DTransformTrack.h"
#include "Sections/MovieScene3DTransformSection.h"
#include "Channels/MovieSceneChannelProxy.h"

#if !(ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 25)
using namespace UE;
#endif

void FGLTFAnimSequenceTask::Complete()
{
	// TODO: bone transforms should be absolute (not relative) according to gltf spec
#if ENGINE_MAJOR_VERSION == 5
	const int32 FrameCount = AnimSequence->GetDataModel()->GetNumberOfKeys();
#else
	const int32 FrameCount = AnimSequence->GetRawNumberOfFrames();
#endif
	const USkeleton* Skeleton = AnimSequence->GetSkeleton();

	FGLTFJsonAnimation& JsonAnimation = Builder.GetAnimation(AnimationIndex);
	JsonAnimation.Name = AnimSequence->GetName() + TEXT("_") + FString::FromInt(AnimationIndex); // Ensure unique name due to limitation in certain gltf viewers

	TArray<float> Timestamps;
	Timestamps.AddUninitialized(FrameCount);

	for (int32 Frame = 0; Frame < FrameCount; ++Frame)
	{
		Timestamps[Frame] = AnimSequence->GetTimeAtFrame(Frame);
	}

	// TODO: add animation data accessor converters to reuse track information

	FGLTFJsonAccessor JsonInputAccessor;
	JsonInputAccessor.BufferView = Builder.AddBufferView(Timestamps);
	JsonInputAccessor.ComponentType = ESKGLTFJsonComponentType::F32;
	JsonInputAccessor.Type = ESKGLTFJsonAccessorType::Scalar;
	JsonInputAccessor.MinMaxLength = 1;
	JsonInputAccessor.Min[0] = 0;

	FBoneContainer BoneContainer;
	if (Builder.ExportOptions->bRetargetBoneTransforms)
	{
		FGLTFBoneUtility::InitializeToSkeleton(BoneContainer, Skeleton);
	}

	ESKGLTFJsonInterpolation Interpolation = FGLTFConverterUtility::ConvertInterpolation(AnimSequence->Interpolation);
	if (Interpolation == ESKGLTFJsonInterpolation::None)
	{
		Interpolation = ESKGLTFJsonInterpolation::Linear;
		// TODO: report warning (about unknown interpolation exported as linear)
	}

#if ENGINE_MAJOR_VERSION == 5
	const int32 TrackCount = AnimSequence->GetDataModel()->GetNumBoneTracks();
#else
	const int32 TrackCount = AnimSequence->GetAnimationTrackNames().Num();
#endif

	for (int32 TrackIndex = 0; TrackIndex < TrackCount; ++TrackIndex)
	{
#if ENGINE_MAJOR_VERSION == 5
		const FBoneAnimationTrack& Track = AnimSequence->GetDataModel()->GetBoneTrackByIndex(TrackIndex);
		const TArray<FVector>& KeyPositions = Track.InternalTrackData.PosKeys;
		const TArray<FQuat>& KeyRotations = Track.InternalTrackData.RotKeys;
		const TArray<FVector>& KeyScales = Track.InternalTrackData.ScaleKeys;
#else
		const FRawAnimSequenceTrack& Track = AnimSequence->GetRawAnimationTrack(TrackIndex);
		const TArray<FVector>&KeyPositions = Track.PosKeys;
		const TArray<FQuat>&KeyRotations = Track.RotKeys;
		const TArray<FVector>&KeyScales = Track.ScaleKeys;
#endif
		const int32 MaxKeys = FMath::Max3(KeyPositions.Num(), KeyRotations.Num(), KeyScales.Num());
		if (MaxKeys == 0)
		{
			continue;
		}

		TArray<FTransform> KeyTransforms;
		KeyTransforms.AddUninitialized(MaxKeys);

		for (int32 Key = 0; Key < KeyTransforms.Num(); ++Key)
		{
			const FVector& KeyPosition = KeyPositions.IsValidIndex(Key) ? KeyPositions[Key] : FVector::ZeroVector;
			const FQuat& KeyRotation = KeyRotations.IsValidIndex(Key) ? KeyRotations[Key] : FQuat::Identity;
			const FVector& KeyScale = KeyScales.IsValidIndex(Key) ? KeyScales[Key] : FVector::OneVector;
			KeyTransforms[Key] = { KeyRotation, KeyPosition, KeyScale };
		}

#if ENGINE_MAJOR_VERSION == 5
		const int32 SkeletonBoneIndex = AnimSequence->GetDataModel()->GetBoneTrackByIndex(TrackIndex).BoneTreeIndex;
#else
		const int32 SkeletonBoneIndex = AnimSequence->GetSkeletonIndexFromRawDataTrackIndex(TrackIndex);
#endif

		const int32 BoneIndex = const_cast<USkeleton*>(Skeleton)->GetMeshBoneIndexFromSkeletonBoneIndex(SkeletalMesh, SkeletonBoneIndex);
		const FGLTFJsonNodeIndex NodeIndex = Builder.GetOrAddNode(RootNode, SkeletalMesh, BoneIndex);

		if (Builder.ExportOptions->bRetargetBoneTransforms && Skeleton->GetBoneTranslationRetargetingMode(SkeletonBoneIndex) != EBoneTranslationRetargetingMode::Animation)
		{
			for (int32 Key = 0; Key < KeyTransforms.Num(); ++Key)
			{
				FGLTFBoneUtility::RetargetTransform(AnimSequence, KeyTransforms[Key], SkeletonBoneIndex, BoneIndex, BoneContainer);
			}
		}

		if (KeyPositions.Num() > 0)
		{
			TArray<FGLTFJsonVector3> Translations;
			Translations.AddUninitialized(KeyPositions.Num());

			for (int32 Key = 0; Key < KeyPositions.Num(); ++Key)
			{
				const FVector KeyPosition = KeyTransforms[Key].GetTranslation();
				Translations[Key] = FGLTFConverterUtility::ConvertPosition(KeyPosition, Builder.ExportOptions->ExportUniformScale);
			}

			JsonInputAccessor.Count = Translations.Num();
			JsonInputAccessor.Max[0] = Timestamps[Translations.Num() - 1];

			FGLTFJsonAccessor JsonOutputAccessor;
			JsonOutputAccessor.BufferView = Builder.AddBufferView(Translations);
			JsonOutputAccessor.ComponentType = ESKGLTFJsonComponentType::F32;
			JsonOutputAccessor.Count = Translations.Num();
			JsonOutputAccessor.Type = ESKGLTFJsonAccessorType::Vec3;

			FGLTFJsonAnimationSampler JsonSampler;
			JsonSampler.Input = Builder.AddAccessor(JsonInputAccessor);
			JsonSampler.Output = Builder.AddAccessor(JsonOutputAccessor);
			JsonSampler.Interpolation = Interpolation;

			FGLTFJsonAnimationChannel JsonChannel;
			JsonChannel.Sampler = FGLTFJsonAnimationSamplerIndex(JsonAnimation.Samplers.Add(JsonSampler));
			JsonChannel.Target.Path = ESKGLTFJsonTargetPath::Translation;
			JsonChannel.Target.Node = NodeIndex;
			JsonAnimation.Channels.Add(JsonChannel);
		}

		if (KeyRotations.Num() > 0)
		{
			TArray<FGLTFJsonQuaternion> Rotations;
			Rotations.AddUninitialized(KeyRotations.Num());

			for (int32 Key = 0; Key < KeyRotations.Num(); ++Key)
			{
				const FQuat KeyRotation = KeyTransforms[Key].GetRotation();
				Rotations[Key] = FGLTFConverterUtility::ConvertRotation(KeyRotation);
			}

			JsonInputAccessor.Count = Rotations.Num();
			JsonInputAccessor.Max[0] = Timestamps[Rotations.Num() - 1];

			FGLTFJsonAccessor JsonOutputAccessor;
			JsonOutputAccessor.BufferView = Builder.AddBufferView(Rotations);
			JsonOutputAccessor.ComponentType = ESKGLTFJsonComponentType::F32;
			JsonOutputAccessor.Count = Rotations.Num();
			JsonOutputAccessor.Type = ESKGLTFJsonAccessorType::Vec4;

			FGLTFJsonAnimationSampler JsonSampler;
			JsonSampler.Input = Builder.AddAccessor(JsonInputAccessor);;
			JsonSampler.Output = Builder.AddAccessor(JsonOutputAccessor);
			JsonSampler.Interpolation = Interpolation;

			FGLTFJsonAnimationChannel JsonChannel;
			JsonChannel.Sampler = FGLTFJsonAnimationSamplerIndex(JsonAnimation.Samplers.Add(JsonSampler));
			JsonChannel.Target.Path = ESKGLTFJsonTargetPath::Rotation;
			JsonChannel.Target.Node = NodeIndex;
			JsonAnimation.Channels.Add(JsonChannel);
		}

		if (KeyScales.Num() > 0)
		{
			TArray<FGLTFJsonVector3> Scales;
			Scales.AddUninitialized(KeyScales.Num());

			for (int32 Key = 0; Key < KeyScales.Num(); ++Key)
			{
				const FVector KeyScale = KeyTransforms[Key].GetScale3D();
				Scales[Key] = FGLTFConverterUtility::ConvertScale(KeyScale);
			}

			JsonInputAccessor.Count = Scales.Num();
			JsonInputAccessor.Max[0] = Timestamps[Scales.Num() - 1];

			FGLTFJsonAccessor JsonOutputAccessor;
			JsonOutputAccessor.BufferView = Builder.AddBufferView(Scales);
			JsonOutputAccessor.ComponentType = ESKGLTFJsonComponentType::F32;
			JsonOutputAccessor.Count = Scales.Num();
			JsonOutputAccessor.Type = ESKGLTFJsonAccessorType::Vec3;

			FGLTFJsonAnimationSampler JsonSampler;
			JsonSampler.Input = Builder.AddAccessor(JsonInputAccessor);
			JsonSampler.Output = Builder.AddAccessor(JsonOutputAccessor);
			JsonSampler.Interpolation = Interpolation;

			FGLTFJsonAnimationChannel JsonChannel;
			JsonChannel.Sampler = FGLTFJsonAnimationSamplerIndex(JsonAnimation.Samplers.Add(JsonSampler));
			JsonChannel.Target.Path = ESKGLTFJsonTargetPath::Scale;
			JsonChannel.Target.Node = NodeIndex;
			JsonAnimation.Channels.Add(JsonChannel);
		}
	}
}

void FGLTFLevelSequenceTask::Complete()
{
	ULevelSequence* Sequence = const_cast<ULevelSequence*>(LevelSequence);
	UMovieScene* MovieScene = Sequence->GetMovieScene();

	ALevelSequenceActor* LevelSequenceActor;
	UWorld* World = Level->GetWorld();
	ULevelSequencePlayer* Player = ULevelSequencePlayer::CreateLevelSequencePlayer(World, Sequence, FMovieSceneSequencePlaybackSettings(), LevelSequenceActor);

	Player->Initialize(Sequence, LevelSequenceActor->GetLevel(), FMovieSceneSequencePlaybackSettings(), FLevelSequenceCameraSettings());
	Player->State.AssignSequence(MovieSceneSequenceID::Root, *Sequence, *Player);

	FFrameRate TickResolution = MovieScene->GetTickResolution();
	FFrameRate DisplayRate = MovieScene->GetDisplayRate(); // TODO: add option to switch between DisplayRate and TickResolution?
	TRange<FFrameNumber> PlaybackRange = MovieScene->GetPlaybackRange();

	int32 FrameOffset = FFrameRate::TransformTime(FFrameTime(MovieScene::DiscreteInclusiveLower(PlaybackRange)), TickResolution, DisplayRate).RoundToFrame().Value;
	int32 FrameCount = FFrameRate::TransformTime(FFrameTime(FFrameNumber(MovieScene::DiscreteSize(PlaybackRange))), TickResolution, DisplayRate).RoundToFrame().Value + 1;

	FGLTFJsonAnimation& JsonAnimation = Builder.GetAnimation(AnimationIndex);
	JsonAnimation.Name = Sequence->GetName() + TEXT("_") + FString::FromInt(AnimationIndex); // Ensure unique name due to limitation in certain gltf viewers

	TArray<float> Timestamps;
	TArray<FFrameTime> FrameTimes;
	Timestamps.AddUninitialized(FrameCount);
	FrameTimes.AddUninitialized(FrameCount);

	for (int32 Frame = 0; Frame < FrameCount; ++Frame)
	{
		Timestamps[Frame] = DisplayRate.AsSeconds(Frame);
		FrameTimes[Frame] = FFrameRate::TransformTime(FFrameTime(FrameOffset + Frame), DisplayRate, TickResolution);
	}

	// TODO: add animation data accessor converters to reuse track information

	FGLTFJsonAccessor JsonInputAccessor;
	JsonInputAccessor.BufferView = Builder.AddBufferView(Timestamps);
	JsonInputAccessor.ComponentType = ESKGLTFJsonComponentType::F32;
	JsonInputAccessor.Type = ESKGLTFJsonAccessorType::Scalar;
	JsonInputAccessor.Count = FrameCount;
	JsonInputAccessor.MinMaxLength = 1;
	JsonInputAccessor.Min[0] = 0;
	JsonInputAccessor.Max[0] = DisplayRate.AsSeconds(FrameCount - 1);
	FGLTFJsonAccessorIndex InputAccessorIndex = Builder.AddAccessor(JsonInputAccessor);

	for (const FMovieSceneBinding& Binding : MovieScene->GetBindings())
	{
		for (TWeakObjectPtr<UObject> Object : Player->FindBoundObjects(Binding.GetObjectGuid(), MovieSceneSequenceID::Root))
		{
			FGLTFJsonNodeIndex NodeIndex;

			if (AActor* Actor = Cast<AActor>(Object.Get()))
			{
				NodeIndex = Builder.GetOrAddNode(Actor);
			}
			else if (USceneComponent* SceneComponent = Cast<USceneComponent>(Object.Get()))
			{
				NodeIndex = Builder.GetOrAddNode(SceneComponent);
			}
			else
			{
				// TODO: report warning
				continue;
			}

			if (NodeIndex == INDEX_NONE)
			{
				// TODO: report warning
				continue;
			}

			for (UMovieSceneTrack* Track : Binding.GetTracks())
			{
				UMovieScene3DTransformTrack* TransformTrack = Cast<UMovieScene3DTransformTrack>(Track);
				if (TransformTrack == nullptr)
				{
					// TODO: report warning
					continue;
				}

				for (UMovieSceneSection* Section : TransformTrack->GetAllSections())
				{
					// TODO: add support for Section->GetCompletionMode() (i.e. when finished)

					UMovieScene3DTransformSection* TransformSection = Cast<UMovieScene3DTransformSection>(Section);
					if (TransformSection == nullptr)
					{
						// TODO: report warning
						continue;
					}

					FOptionalMovieSceneBlendType BlendType = TransformSection->GetBlendType();
					if (BlendType.IsValid() && BlendType.Get() != EMovieSceneBlendType::Absolute)
					{
						// TODO: report warning
						continue;
					}

					// TODO: do we need to account for TransformSection->GetOffsetTime() ?

					TArrayView<FMovieSceneFloatChannel*> Channels = TransformSection->GetChannelProxy().GetChannels<FMovieSceneFloatChannel>();
					EMovieSceneTransformChannel ChannelMask = TransformSection->GetMask().GetChannels();

					if (EnumHasAnyFlags(ChannelMask, EMovieSceneTransformChannel::Translation))
					{
						TArray<FGLTFJsonVector3> Translations;
						Translations.AddUninitialized(FrameCount);

						for (int32 Frame = 0; Frame < FrameCount; ++Frame)
						{
							FFrameTime& FrameTime = FrameTimes[Frame];

							FVector Translation = FVector::ZeroVector;
							Channels[0]->Evaluate(FrameTime, Translation.X);
							Channels[1]->Evaluate(FrameTime, Translation.Y);
							Channels[2]->Evaluate(FrameTime, Translation.Z);

							Translations[Frame] = FGLTFConverterUtility::ConvertPosition(Translation, Builder.ExportOptions->ExportUniformScale);
						}

						FGLTFJsonAccessor JsonOutputAccessor;
						JsonOutputAccessor.BufferView = Builder.AddBufferView(Translations);
						JsonOutputAccessor.ComponentType = ESKGLTFJsonComponentType::F32;
						JsonOutputAccessor.Count = Translations.Num();
						JsonOutputAccessor.Type = ESKGLTFJsonAccessorType::Vec3;

						FGLTFJsonAnimationSampler JsonSampler;
						JsonSampler.Input = InputAccessorIndex;
						JsonSampler.Output = Builder.AddAccessor(JsonOutputAccessor);
						JsonSampler.Interpolation = ESKGLTFJsonInterpolation::Linear;

						FGLTFJsonAnimationChannel JsonChannel;
						JsonChannel.Sampler = FGLTFJsonAnimationSamplerIndex(JsonAnimation.Samplers.Add(JsonSampler));
						JsonChannel.Target.Path = ESKGLTFJsonTargetPath::Translation;
						JsonChannel.Target.Node = NodeIndex;
						JsonAnimation.Channels.Add(JsonChannel);
					}

					if (EnumHasAnyFlags(ChannelMask, EMovieSceneTransformChannel::Rotation))
					{
						TArray<FGLTFJsonQuaternion> Rotations;
						Rotations.AddUninitialized(FrameCount);

						for (int32 Frame = 0; Frame < FrameCount; ++Frame)
						{
							FFrameTime& FrameTime = FrameTimes[Frame];

							FRotator Rotator = FRotator::ZeroRotator;
							Channels[3]->Evaluate(FrameTime, Rotator.Roll);
							Channels[4]->Evaluate(FrameTime, Rotator.Pitch);
							Channels[5]->Evaluate(FrameTime, Rotator.Yaw);

							Rotations[Frame] = FGLTFConverterUtility::ConvertRotation(Rotator.Quaternion());
						}

						JsonInputAccessor.Count = Rotations.Num();
						JsonInputAccessor.Max[0] = Timestamps[Rotations.Num() - 1];

						FGLTFJsonAccessor JsonOutputAccessor;
						JsonOutputAccessor.BufferView = Builder.AddBufferView(Rotations);
						JsonOutputAccessor.ComponentType = ESKGLTFJsonComponentType::F32;
						JsonOutputAccessor.Count = Rotations.Num();
						JsonOutputAccessor.Type = ESKGLTFJsonAccessorType::Vec4;

						FGLTFJsonAnimationSampler JsonSampler;
						JsonSampler.Input = InputAccessorIndex;
						JsonSampler.Output = Builder.AddAccessor(JsonOutputAccessor);
						JsonSampler.Interpolation = ESKGLTFJsonInterpolation::Linear;

						FGLTFJsonAnimationChannel JsonChannel;
						JsonChannel.Sampler = FGLTFJsonAnimationSamplerIndex(JsonAnimation.Samplers.Add(JsonSampler));
						JsonChannel.Target.Path = ESKGLTFJsonTargetPath::Rotation;
						JsonChannel.Target.Node = NodeIndex;
						JsonAnimation.Channels.Add(JsonChannel);
					}

					if (EnumHasAnyFlags(ChannelMask, EMovieSceneTransformChannel::Scale))
					{
						TArray<FGLTFJsonVector3> Scales;
						Scales.AddUninitialized(FrameCount);

						for (int32 Frame = 0; Frame < FrameCount; ++Frame)
						{
							FFrameTime& FrameTime = FrameTimes[Frame];

							FVector Scale = FVector::OneVector;
							Channels[6]->Evaluate(FrameTime, Scale.X);
							Channels[7]->Evaluate(FrameTime, Scale.Y);
							Channels[8]->Evaluate(FrameTime, Scale.Z);

							Scales[Frame] = FGLTFConverterUtility::ConvertScale(Scale);
						}

						FGLTFJsonAccessor JsonOutputAccessor;
						JsonOutputAccessor.BufferView = Builder.AddBufferView(Scales);
						JsonOutputAccessor.ComponentType = ESKGLTFJsonComponentType::F32;
						JsonOutputAccessor.Count = Scales.Num();
						JsonOutputAccessor.Type = ESKGLTFJsonAccessorType::Vec3;

						FGLTFJsonAnimationSampler JsonSampler;
						JsonSampler.Input = InputAccessorIndex;
						JsonSampler.Output = Builder.AddAccessor(JsonOutputAccessor);
						JsonSampler.Interpolation = ESKGLTFJsonInterpolation::Linear;

						FGLTFJsonAnimationChannel JsonChannel;
						JsonChannel.Sampler = FGLTFJsonAnimationSamplerIndex(JsonAnimation.Samplers.Add(JsonSampler));
						JsonChannel.Target.Path = ESKGLTFJsonTargetPath::Scale;
						JsonChannel.Target.Node = NodeIndex;
						JsonAnimation.Channels.Add(JsonChannel);
					}
				}
			}
		}
	}

	World->DestroyActor(LevelSequenceActor);
}
