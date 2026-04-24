// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TomatinaSoundCue.generated.h"

class USoundBase;

/**
 * 汎用サウンド Cue 構造体。
 * どのクラスからでも 1 UPROPERTY 分で Sound / Volume / Pitch / Delay を一括編集可能にする。
 * 未設定（Sound=null）なら黙ってスキップ。
 */
USTRUCT(BlueprintType)
struct TOMATO_API FTomatinaSoundCue
{
	GENERATED_BODY()

	/** 再生するサウンド（null ならこの Cue は無音扱い） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USoundBase* Sound = nullptr;

	/** 音量倍率 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="4.0"))
	float Volume = 1.0f;

	/** ピッチ倍率 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.1", ClampMax="4.0"))
	float Pitch = 1.0f;

	/** 再生までの遅延秒数（タイミング調整用。0 なら即時） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="10.0"))
	float DelaySeconds = 0.0f;
};
