// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TomatinaTargetBase.h"
#include "TomatinaFunctionLibrary.generated.h"

class APlayerController;
class USceneCaptureComponent2D;
class UCameraComponent;
class UTextureRenderTarget2D;

USTRUCT(BlueprintType)
struct FPhotoResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 Score = 0;

	UPROPERTY(BlueprintReadOnly)
	ATomatinaTargetBase* BestTarget = nullptr;

	UPROPERTY(BlueprintReadOnly)
	FString Comment;
};

UCLASS()
class TOMATO_API UTomatinaFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Tomatina|Visibility")
	static bool CheckVisibility(
		USceneCaptureComponent2D* ZoomCamera,
		AActor* Target,
		FVector CheckLoc,
		float ScreenWidth,
		float ScreenHeight);

	UFUNCTION(BlueprintCallable, Category="Tomatina|Photo")
	static FPhotoResult CalculatePhotoScore(
		USceneCaptureComponent2D* ZoomCamera,
		const TArray<ATomatinaTargetBase*>& Targets,
		FName CurrentMission,
		float ScreenWidth,
		float ScreenHeight);

	UFUNCTION(BlueprintCallable, Category="Tomatina|Camera")
	static FVector CalculateZoomOffset(
		APlayerController* PC,
		const FHitResult& HitResult,
		UCameraComponent* Camera,
		float CameraFOV = 90.f);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Tomatina|Camera")
	static FVector2D GetZoomScreenCenter(
		float MainWidth,
		float PhoneWidth,
		float PhoneHeight);

	UFUNCTION(BlueprintCallable, Category="Tomatina|Camera")
	static FVector2D ProjectZoomToMainScreen(
		APlayerController* PC,
		USceneCaptureComponent2D* SceneCapture,
		float MainWidth,
		float MainHeight);

	UFUNCTION(BlueprintCallable, Category="Tomatina|Photo")
	static void CopyZoomToPhoto(
		USceneCaptureComponent2D* ZoomCamera,
		UTextureRenderTarget2D* PhotoTarget);
};
