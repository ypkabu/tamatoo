// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"
#include "TomatinaSoundCue.h"  // FTomatinaSoundCue
#include "TomatinaTargetBase.generated.h"

class UWidgetComponent;
class UUserWidget;

// ─────────────────────────────────────────────────────────────────────────────
// 移動パターン
// ─────────────────────────────────────────────────────────────────────────────
UENUM(BlueprintType)
enum class ETargetMovement : uint8
{
	DepthHideAndSeek  UMETA(DisplayName="Depth Hide And Seek"),  // 奥行き出入り
	RunAcross         UMETA(DisplayName="Run Across"),           // 画面端から端へ
	FlyArc            UMETA(DisplayName="Fly Arc"),              // 弧を描いて飛ぶ
	FloatErratic      UMETA(DisplayName="Float Erratic"),        // 不規則浮遊
	BlendWithCrowd    UMETA(DisplayName="Blend With Crowd"),     // 群衆に紛れて歩く
};

// ─────────────────────────────────────────────────────────────────────────────
// ATomatinaTargetBase
// ─────────────────────────────────────────────────────────────────────────────
UCLASS(Blueprintable)
class TOMATO_API ATomatinaTargetBase : public AActor
{
	GENERATED_BODY()

public:
	ATomatinaTargetBase();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// =========================================================================
	// 識別・ビジュアル
	// =========================================================================

	/** お題タグ。GameMode の CurrentMission と照合する */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Target")
	FName MyType = FName("None");

	/** ターゲット出現・移動ループ・ポーズの詳細ログ。通常はOFF。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Target|Debug")
	bool bDebugTargetLog = false;

	/** シーンルート。MeshComp はこの配下にぶら下がるので BP で Location/Rotation/Scale を自由に変更できる */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Target")
	USceneComponent* SceneRoot;

	/** 被写体のビジュアルメッシュ（SceneRoot の子。BP で RelativeLocation/Rotation/Scale 編集可） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Target")
	USkeletalMeshComponent* MeshComp;

	// =========================================================================
	// 撮影スコア（ターゲット個別チューニング）
	// =========================================================================

	/** 全身が写ったときの基礎点 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Target|PhotoScore", meta=(ClampMin="0"))
	int32 FullBodyScore = 100;

	/** 上半身のみ写ったときの基礎点 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Target|PhotoScore", meta=(ClampMin="0"))
	int32 UpperBodyScore = 50;

	/** 足元のみ写ったときの基礎点 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Target|PhotoScore", meta=(ClampMin="0"))
	int32 LowerBodyScore = 10;

	/** 被写体ごとの倍率（例: 1.5 で 1.5 倍） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Target|PhotoScore", meta=(ClampMin="0.1"))
	float PhotoScoreMultiplier = 1.0f;

	/** 被写体ごとの固定加点（難しい被写体用） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Target|PhotoScore")
	int32 PhotoScoreFlatBonus = 0;

	/** スタイリッシュ高ランク帯でこの被写体を撮ると入る追加点 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Target|PhotoScore")
	int32 HighRankBonusScore = 0;

	/** スタイリッシュランクが変化したときの BP フック（ゴリラ演出トリガー用） */
	UFUNCTION(BlueprintImplementableEvent, Category="Target|Stylish")
	void OnStylishRankChanged(FName NewRank, bool bIsHighRank);

	/** 高ランク状態でこの被写体を撮影した瞬間の BP フック */
	UFUNCTION(BlueprintImplementableEvent, Category="Target|Stylish")
	void OnHighRankShot(FName RankName, int32 ShotScore);

	// =========================================================================
	// 目印（頭上に浮かぶアイコン Widget）
	// =========================================================================

	/** 頭上に表示する目印ウィジェット（アイコンや矢印） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Target|Marker")
	UWidgetComponent* MarkerWidgetComp;

	/** 目印を表示するか */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Target|Marker")
	bool bShowMarker = true;

	/** 目印ウィジェットクラス（WBP_TargetMarker 等） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Target|Marker")
	TSubclassOf<UUserWidget> MarkerWidgetClass;

	/** 頭上からの高さオフセット（cm）。0 だと頭の位置に出る */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Target|Marker")
	float MarkerHeightOffset = 60.f;

	/** 目印の描画サイズ（ピクセル） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Target|Marker")
	FVector2D MarkerDrawSize = FVector2D(128.f, 128.f);

	// =========================================================================
	// ポストプロセス アウトライン用ステンシル
	// （BP 側で Post Process Material を組むときに使う）
	// =========================================================================

	/** MeshComp に CustomDepth (Stencil) を有効にしてターゲットだけアウトライン対象にする */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Target|Outline")
	bool bEnableOutlineStencil = true;

	/** ポストプロセスマテリアルで判定する Stencil 値（1〜255） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Target|Outline", meta=(ClampMin="1", ClampMax="255"))
	int32 OutlineStencilValue = 250;

	// =========================================================================
	// 移動パターン共通
	// =========================================================================

	/** 移動パターン */
	UPROPERTY(EditAnywhere, Category="Movement")
	ETargetMovement MovementType = ETargetMovement::DepthHideAndSeek;

	/** 移動速度（パターンによって意味が変わる） */
	UPROPERTY(EditAnywhere, Category="Movement")
	float MoveSpeed = 200.0f;

	/** true: ループ動作　false: 1 回動いて Destroy */
	UPROPERTY(EditAnywhere, Category="Movement")
	bool bLoop = true;

	/** 出現までの待機秒数（レベル配置時のタイミングをバラけさせる） */
	UPROPERTY(EditAnywhere, Category="Movement")
	float StartDelay = 0.0f;

	/** 移動中、進行方向に体を向ける（全 MovementType 共通） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
	bool bFaceMovementDirection = true;

	/** メッシュ natural forward が +X でない場合の Yaw 補正（度） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
	float MeshYawOffset = 0.f;

	/** ターゲットが出現した瞬間（StartDelay 終了）に鳴らす SE */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Target|Audio")
	FTomatinaSoundCue SpawnAppearSound;

	// =========================================================================
	// DepthHideAndSeek 用
	// =========================================================================

	/** 群衆の奥に隠れるときの Y 軸オフセット */
	UPROPERTY(EditAnywhere, Category="Movement|Depth")
	float HiddenY = -300.0f;

	/** 群衆の前に出るときの Y 軸オフセット */
	UPROPERTY(EditAnywhere, Category="Movement|Depth")
	float ShownY = 300.0f;

	/** 前に出ている秒数 */
	UPROPERTY(EditAnywhere, Category="Movement|Depth")
	float ShowDuration = 2.0f;

	/** 奥に隠れている秒数 */
	UPROPERTY(EditAnywhere, Category="Movement|Depth")
	float HideDuration = 3.0f;

	/** 前後移動の補間速度 */
	UPROPERTY(EditAnywhere, Category="Movement|Depth")
	float DepthInterpSpeed = 5.0f;

	// =========================================================================
	// RunAcross 用
	// =========================================================================

	/** スタート地点（OriginLocation からの相対オフセット） */
	UPROPERTY(EditAnywhere, Category="Movement|Run")
	FVector RunStartOffset = FVector(0.f, -2000.f, 0.f);

	/** ゴール地点（OriginLocation からの相対オフセット） */
	UPROPERTY(EditAnywhere, Category="Movement|Run")
	FVector RunEndOffset = FVector(0.f, 2000.f, 0.f);

	/** 地面からの高さ（犬=0、鳥=高い値） */
	UPROPERTY(EditAnywhere, Category="Movement|Run")
	float RunHeight = 0.0f;

	// =========================================================================
	// FlyArc 用
	// =========================================================================

	/** 弧の横方向の半径 */
	UPROPERTY(EditAnywhere, Category="Movement|Fly")
	float ArcRadius = 1500.0f;

	/** 弧の最高点の高さ */
	UPROPERTY(EditAnywhere, Category="Movement|Fly")
	float ArcHeight = 500.0f;

	/** ホバリング（一時停止）の秒数。0 = ホバリングなし */
	UPROPERTY(EditAnywhere, Category="Movement|Fly")
	float HoverDuration = 1.5f;

	/** ホバリング発生確率（0〜1） */
	UPROPERTY(EditAnywhere, Category="Movement|Fly")
	float HoverChance = 0.3f;

	// =========================================================================
	// FloatErratic 用
	// =========================================================================

	/** ランダム移動の範囲半径 */
	UPROPERTY(EditAnywhere, Category="Movement|Float")
	float FloatRadius = 500.0f;

	/** ワープ抽選を行う間隔（秒） */
	UPROPERTY(EditAnywhere, Category="Movement|Float")
	float WarpInterval = 5.0f;

	/** ワープ発生確率（0〜1） */
	UPROPERTY(EditAnywhere, Category="Movement|Float")
	float WarpChance = 0.2f;

	// =========================================================================
	// BlendWithCrowd 用
	// =========================================================================

	/** 群衆と合わせた歩行速度 */
	UPROPERTY(EditAnywhere, Category="Movement|Blend")
	float WalkSpeed = 100.0f;

	/** 奥行き方向のゆらぎ量 */
	UPROPERTY(EditAnywhere, Category="Movement|Blend")
	float DepthSwayAmount = 200.0f;

	/** ゆらぎの周期（Hz） */
	UPROPERTY(EditAnywhere, Category="Movement|Blend")
	float DepthSwayFrequency = 0.5f;

	// =========================================================================
	// ポーズ（一時停止して決めポーズ → 写真ボーナス）
	// =========================================================================

	/** ポーズ機能を有効にするか（Details で個別 ON/OFF） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pose")
	bool bEnablePose = false;

	/** ポーズ抽選を行う間隔（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pose", meta=(EditCondition="bEnablePose"))
	float PoseCheckInterval = 3.0f;

	/** 抽選 1 回あたりのポーズ発生確率（0〜1） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pose", meta=(EditCondition="bEnablePose", ClampMin="0.0", ClampMax="1.0"))
	float PoseChance = 0.3f;

	/** ポーズ継続秒数 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pose", meta=(EditCondition="bEnablePose"))
	float PoseDuration = 1.5f;

	/** ポーズ中に撮影されたときのスコア倍率（例: 2.0 = 2 倍） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pose", meta=(EditCondition="bEnablePose", ClampMin="1.0"))
	float PoseScoreMultiplier = 2.0f;

	/** 現在ポーズ中か（CalculatePhotoScore から参照される） */
	UPROPERTY(BlueprintReadOnly, Category="Pose")
	bool bIsPosing = false;

	/** ポーズ開始時に BP で再生する用のフック（アニメ等） */
	UFUNCTION(BlueprintImplementableEvent, Category="Pose")
	void OnPoseStarted();

	/** ポーズ終了時に BP で再生する用のフック */
	UFUNCTION(BlueprintImplementableEvent, Category="Pose")
	void OnPoseEnded();

	// =========================================================================
	// ソケット代替：頭・ルート位置
	// =========================================================================

	/** GetHeadLocation が使う「メッシュ高さ」 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Target")
	float MeshHeight = 100.0f;

	/** 頭の位置を返す（ソケット未使用時のフォールバック） */
	UFUNCTION(BlueprintCallable, Category="Target")
	FVector GetHeadLocation() const;

	/** アクターのルート位置を返す */
	UFUNCTION(BlueprintCallable, Category="Target")
	FVector GetRootLocation() const;

private:
	// ── 内部状態 ─────────────────────────────────────────────────────────────
	FVector OriginLocation;

	/** 進行方向算出用：前フレームの位置 */
	FVector LastFrameLocation = FVector::ZeroVector;
	bool    bHasLastLocation  = false;

	float StateTimer    = 0.f;
	bool  bIsShowing    = false;  // DepthHideAndSeek

	float RunProgress   = 0.f;   // RunAcross（0〜1）

	float ArcAngle      = 0.f;   // FlyArc（度数）
	bool  bIsHovering   = false;
	float HoverTimer    = 0.f;

	FVector FloatTarget;          // FloatErratic
	float   WarpTimer   = 0.f;
	float   ElapsedTime = 0.f;   // FloatErratic / BlendWithCrowd 共用

	float DelayTimer    = 0.f;
	bool  bActive       = false;

	// ポーズ用タイマー
	float PoseCheckTimer = 0.f;
	float PoseTimer      = 0.f;

	// ── Tick サブ関数 ────────────────────────────────────────────────────────
	void TickDepthHideAndSeek(float DeltaTime);
	void TickRunAcross(float DeltaTime);
	void TickFlyArc(float DeltaTime);
	void TickFloatErratic(float DeltaTime);
	void TickBlendWithCrowd(float DeltaTime);

	/** ポーズ抽選・継続管理。true を返したら movement Tick はスキップする */
	bool TickPose(float DeltaTime);
};
