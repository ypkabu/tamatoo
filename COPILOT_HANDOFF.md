# Tomatina プロジェクト引き継ぎ仕様書（Copilot 向け）

最終更新: 2026-04-23

このドキュメントは、Tomatina（トマト祭り写真撮影ゲーム）の実装を引き継ぐための完全仕様書。
プロジェクトのアーキテクチャ、各 C++ クラスの責務、Blueprint セットアップ手順、既知の問題、未完成の機能まで網羅している。

---

## 0. プロジェクト概要

**ゲーム名**: Tomatina（トマティーナ）
**ジャンル**: 2 人協力カメラ撮影アクション
**展示先**: BitSummit（インディーゲーム展示会）
**エンジン**: Unreal Engine 5.7
**プロジェクトパス**: `C:\Users\zhush\Documents\Unreal Projects\tamatoo`
**モジュール名**: Tomato（C++ クラスは全て `TOMATO_API`）

### ゲームコンセプト
- 1P（マウス操作）: カメラマンになり、お題のターゲット（人物・動物）を撮影
- 2P（LeapMotion 操作）: タオルを振ってカメラレンズの汚れを拭く
- 街では群衆が動き回り、投擲手がトマトを投げてくる（プレイヤー or 街中）
- 撮影成功でスコア。カメラレンズの汚れが多いと減点

### スクリーン構成（特殊）
- **横長ウィンドウ 3584 × 1600 px**
  - 左側 = メインモニター（カメラビュー）: 2560 × 1600
  - 右側 = iPhone（ズームビュー / ファインダー）: 1024 × 768（残りの空きはレターボックス）
- メインと iPhone は **1 つのウィンドウ**として扱う（疑似デュアルスクリーン）
- マウスカーソルはメインモニター内に閉じ込める（クランプ実装済み）
- ズーム中はカーソルを iPhone 中央に飛ばす

---

## 1. 開発環境

### バージョン
- **Unreal Engine 5.7**
- **Visual Studio 2026**（or 2022）
- ターゲット OS: Windows 11
- Build Configuration: **Development Editor / Win64**

### 使用プラグイン
- **Ultraleap Tracking 5.0.1**（`Plugins/UltraleapTracking_ue5_4-5.0.1/`）
  - LeapMotion でタオルを操作
  - イベント: `OnLeapTrackingData`（毎フレーム手データ）
  - 注意: バージョンによって `OnLeapHandLost` イベントが BP に出ないことがある（Hands.Length チェックで代替）
- **Enhanced Input**（標準）

### 依存モジュール（Tomato.Build.cs）
```cs
PublicDependencyModuleNames.AddRange(new string[]
{
    "Core", "CoreUObject", "Engine", "InputCore",
    "UMG", "Slate", "SlateCore", "EnhancedInput"
});
```

LeapMotion を C++ から触る場合は `"LeapMotion"` を追加（現状は BP 経由なので不要）。

---

## 2. ビルド方法

### Live Coding（推奨・速い）
- Editor 起動中に `Ctrl + Alt + F11`
- C++ ヘッダ追加・既存メンバ変更があるとリロード必要

### クリーンビルド（Live Coding が壊れたとき）
1. Editor、Visual Studio を完全終了
2. `Binaries/`、`Intermediate/`、`.vs/` を削除
3. `Tomato.uproject` 右クリック → Generate Visual Studio project files
4. `Tomato.sln` を VS で開く → Build

### Clang 言語サーバーの偽陽性について
**重要**: 編集中に下記のような diagnostic が大量に出るのは正常:
- `'CoreMinimal.h' file not found`
- `'AClassName' is not a class, namespace, or enumeration`
- `Unknown type name 'UCLASS'`

これは clangd が UE のビルドフラグを知らないため。**実際のコンパイルエラーではない**。Live Coding / VS Build が成功すれば問題なし。Copilot が修正を持ちかけてくる場合は無視させること。

---

## 3. プロジェクト全体のファイル構成

### Source/Tomato/
```
Tomato.Build.cs              # モジュール依存
Tomato.cpp / Tomato.h        # モジュール定義
MyActor.cpp / MyActor.h      # 未使用（消してよい）

# ─── ゲームの中核 ──────────────────────────
TomatinaGameMode.{h,cpp}     # ミッション管理・スコア・撮影処理
TomatinaPlayerPawn.{h,cpp}   # 1P カメラマン Pawn（DefaultPawn 派生、Enhanced Input）
TomatinaHUD.{h,cpp}          # 全 Widget の生成・表示制御
TomatinaFunctionLibrary.{h,cpp}  # 静的ヘルパー（撮影スコア計算・座標変換）

# ─── ターゲット（撮影対象）──────────────────
TomatinaTargetBase.{h,cpp}        # 撮影対象の親クラス。5 つの動きパターン + ポーズ + 頭上マーカー + アウトライン
TomatinaTargetSpawner.{h,cpp}     # ミッションごとにターゲットをスポーン

# ─── 群衆（背景の人々）─────────────────────
TomatinaCrowdMember.{h,cpp}       # 群衆 1 人。Wander/Burst/Cheer の 3 状態
TomatinaCrowdManager.{h,cpp}      # 群衆を一括管理（複数種類混在対応）

# ─── トマト投擲システム ────────────────────
TomatinaProjectile.{h,cpp}             # トマト本体（Straight/Arc/Curve 軌道、プレイヤーヒット or ワールドデカール）
TomatinaProjectileSpawner.{h,cpp}      # 旧式の場所固定スポーナー（残ってるが Thrower 推奨）
TomatinaThrower.{h,cpp}                # 投擲手キャラ（歩行モード + 投擲モーション + プレイヤー/街狙い分岐）
TomatinaThrowerSpawner.{h,cpp}         # 投擲手を時間経過で増えるよう自動スポーン

# ─── 汚れシステム ─────────────────────────
TomatoDirtManager.{h,cpp}         # 汚れデータ管理（追加・拭き取り・自然減衰）
TomatinaTowelSystem.{h,cpp}       # （現状ほぼ空。LeapMotion 拭き取りは BP_TomatinaPlayerPawn で実装）
```

### Content/
- `Blueprints/`（BP_ プレフィックス）
- `WBP_*`（Widget Blueprint）
- `Materials/`（汚れ・ズーム表示・写真表示・デカール）
- `Tomato_Asset/`（テクスチャ・メッシュ）
- `ModularBuildingSet/`（街の建物）

### Blueprint 命名規則
- C++ クラスを継承する BP は `BP_クラス名` または `BP_用途名`
- Widget は `WBP_用途名`
- Material は `M_用途名`、Material Instance は `MI_用途名`

---

## 4. 各システム詳細仕様

### 4-1. スクリーンサイズ（重要：4 変数で全体制御）

`ATomatinaPlayerPawn` が**唯一のソース**。GameMode と HUD は BeginPlay で Pawn から取得する。

| 変数 | デフォルト | 意味 |
|------|----------|------|
| `MainWidth`   | 2560 | メインモニター横幅（px） |
| `MainHeight`  | 1600 | メインモニター縦幅 |
| `PhoneWidth`  | 1024 | iPhone 横幅 |
| `PhoneHeight` | 768  | iPhone 縦幅 |
| `bTestMode`   | true | iPhone なしで開発するときの PiP モード |

**ウィンドウサイズ = MainWidth + PhoneWidth × MainHeight = 3584 × 1600**

PIE で動作確認するときは Editor Preferences → Level Editor → Play → Game Viewport Settings で `3584 × 1600` を新規追加して使う。

---

### 4-2. ATomatinaGameMode

#### 主要プロパティ
- `Missions` (TArray<FMissionData>): お題リスト
  - `TargetType` (FName): ターゲットの MyType と照合
  - `DisplayText` (FText): 「ゴリラを撮れ！」
  - `TimeLimit` (float, 15s): **ゲーム時間はここ**。0 以下で無制限
  - `TargetClass` (TSubclassOf<ATomatinaTargetBase>): スポーンする BP
  - `SpawnCount` (int): 同時スポーン数
  - `SpawnProfileName` (FName): TargetSpawner の SpawnProfiles から検索
  - `TargetImage` (UTexture2D*): HUD のお題プレビュー画像
- `bRandomOrder` (bool): true ならミッション順をシャッフル
- `RT_Photo` (UTextureRenderTarget2D*): 撮影写真用 RT
- `ShutterSound` / `BGM`
- `ResultDisplayTime` (3s): 撮影リザルト表示時間
- `MissionResultDisplayTime` (2s): ミッション結果表示時間
- `DirtPenaltyPerSplat` (-5): 写真に写った汚れ 1 個あたりの減点

#### 主要メソッド
- `TakePhoto(SceneCapture)` — 撮影実行。`UTomatinaFunctionLibrary::CalculatePhotoScore` でスコア算出 → DirtManager から汚れ取得 → 減点 → HUD->ShowResult
- `StartMission(Index)` — 指定ミッション開始

---

### 4-3. ATomatinaPlayerPawn（1P カメラマン）

#### 継承
`ADefaultPawn` 派生。`bAddDefaultMovementBindings = false` で内蔵入力を無効化、Enhanced Input で完全制御。

#### コンポーネント
- `PlayerCamera` (UCameraComponent): メインモニター描画用
- `SceneCapture_Zoom` (USceneCaptureComponent2D): RT_Zoom に出力（iPhone のファインダー用）

#### Enhanced Input
- `IA_RightMouse` (Started/Completed): ズーム開始 / 解除
- `IA_LeftMouse` (Started): 撮影
- `IA_Look` (Triggered): マウス移動を蓄積して Tick で消費
- `DefaultMappingContext`: BeginPlay で AddMappingContext

#### ズーム関連プロパティ
- `DefaultFOV = 90` / `ZoomFOV = 30`
- `ZoomSpeed = 5` (FInterpTo 速度)
- `MoveSpeed = 500`
- `ZoomLookSensitivity = 3.0` (ズーム中のマウス感度)
- `ZoomPitchLimit = 89` / `ZoomYawLimit = 0` (0 で制限なし)

#### 撮影フロー（OnLeftMousePressed）
1. ズーム中のみ撮影可能
2. GameMode->TakePhoto(SceneCapture_Zoom) を呼ぶ
3. ズーム解除（フラグリセット）
4. **カーソル位置はそのまま**（自動移動禁止仕様）
5. HUD->HideCursor
6. メインモニター外にカーソルがあれば次フレームの Tick で自動クランプされる

#### カーソルクランプ（Tick 内）
非ズーム時に毎フレーム:
```cpp
ClampedX = Clamp(MX, 0, MainWidth-1);
ClampedY = Clamp(MY, 0, MainHeight-1);
PC->SetMouseLocation(ClampedX, ClampedY);
```
これでカーソルが iPhone 領域（x=2560 以降）に出ない。

#### LeapMotion タオル（BP 側で実装）
- BP に Leap Component を追加
- `OnLeapTrackingData_Received` Custom Event を Bind 経由で作成
- 手のひら座標（Position.Y / Z）を Map Range Clamped で 0〜1 正規化
- `WipeDirtAt(HandNormPos, WipeRadius, Amount × DeltaSeconds)` を呼ぶ
- HUD の `Update Towel Position` で IMG_Towel を移動、`Show/Hide Towel` で表示制御
- 手なし判定は Hands.Length <= 0 で代替

詳細は §6「LeapMotion 拭き取りシステム」参照。

---

### 4-4. ATomatinaHUD

#### 役割
全 Widget の生成・表示制御を一括管理。位置・サイズは原則 C++ で触らない。
**例外1**: `UpdateCursorPosition` → IMG_Crosshair を移動
**例外2**: `UpdateDirtDisplay` → SplatContainer に動的に UImage を追加

#### Widget クラス参照（BP で TSubclassOf を設定）
- ViewFinderWidgetClass
- CursorWidgetClass
- DirtOverlayWidgetClass
- PhotoResultWidgetClass
- MissionDisplayWidgetClass
- MissionResultWidgetClass
- FinalResultWidgetClass
- ShutterFlashWidgetClass
- CountdownWidgetClass
- TestPipWidgetClass

#### マテリアル・テクスチャ
- `ZoomDisplayMaterial` (UMaterialInterface*)
- `PhotoDisplayMaterial` (UMaterialInterface*)
- `DirtTexture` (UTexture2D*) — 単体フォールバック
- `DirtTextures` (TArray<UTexture2D*>) — **複数バリエーション**。FDirtSplat::TextureIndex で参照される

#### 写真表示サイズ
- `PhotoDisplayWidth/Height = 1024 / 768` — WBP_PhotoResult の SplatContainer サイズと一致させる
- `DirtInnerMargin = 30` — 汚れ位置の内側余白

#### 主要メソッド
- `UpdateCursorPosition(Pos)` — IMG_Crosshair を iPhone 領域に移動
- `ShowCursor() / HideCursor()`
- `UpdateTowelPosition(Pos)` — IMG_Towel をメインモニター内に移動（要 WBP_DirtOverlay に IMG_Towel 配置）
- `ShowTowel() / HideTowel()`
- `ShowResult(Score, Comment, Dirts)` — 撮影リザルト表示。WBP_PhotoResult の SplatContainer に汚れを配置
- `HideResult()`
- `ShowMissionResult(Score, Comment)` / `HideMissionResult()`
- `ShowFinalResult(TotalScore, MissionCount)`
- `ShowMissionDisplay(Text, Image)` / `HideMissionDisplay()`
- `UpdateTimer(Seconds)` / `UpdateTotalScore(Score)`
- `ShowCountdown(Seconds)` / `HideCountdown()`
- `PlayShutterFlash()` — 実時間カウンタ（TimeDilation=0 でも動く）
- `UpdateDirtDisplay(Dirts)` — メイン + iPhone の両方に汚れ UImage を動的生成
- `UpdateTowelStatus(Percent, bSwapping)` — タオル耐久バー更新

#### AddDirtSplatsToCanvas（汚れ動的生成の中心）
Main / Phone / 写真の 3 か所から呼ばれる共通ヘルパー。
- AreaWidth/Height でサイズスケール
- OriginX/Y で配置原点シフト（Main=0,0 / Phone=MainWidth,0 / 写真=0,0）
- `Dirt.TextureIndex` で `DirtTextures[i]` を選択（範囲外なら `DirtTexture` フォールバック）
- 汚れサイズが領域＋マージンに収まるようクランプ
- Size は `Clamp(0.01, 1.0)` で安全側に

---

### 4-5. ATomatinaTargetBase（撮影対象）

#### 5 つの動きパターン（ETargetMovement）
- `DepthHideAndSeek` — 群衆の奥に隠れたり前に出たり
- `RunAcross` — 端から端へ走る
- `FlyArc` — 弧を描いて飛ぶ（鳥）
- `FloatErratic` — ランダム浮遊（虫など）
- `BlendWithCrowd` — 群衆と歩く

#### 共通プロパティ
- `MyType` (FName) — ミッションの TargetType と照合
- `MeshComp` (USkeletalMeshComponent)
- `MoveSpeed`、`bLoop`、`StartDelay`
- パターン別プロパティ多数（HiddenY, ShownY, RunStartOffset, ArcRadius など）

#### ポーズシステム
- `bEnablePose` (default false) — 個別 ON/OFF
- `PoseCheckInterval = 3.0`、`PoseChance = 0.3`、`PoseDuration = 1.5`
- `PoseScoreMultiplier = 2.0` — ポーズ中に撮影されたスコアに乗算
- BP イベント: `OnPoseStarted` / `OnPoseEnded`（BlueprintImplementableEvent）
- `bIsPosing` (BlueprintReadOnly) — `CalculatePhotoScore` から参照される

#### 頭上マーカー
- `MarkerWidgetComp` (UWidgetComponent) — EWidgetSpace::Screen で常にカメラ向き
- `bShowMarker = true`
- `MarkerWidgetClass` (TSubclassOf<UUserWidget>) — WBP_TargetMarker 等
- `MarkerHeightOffset = 60`、`MarkerDrawSize = (128, 128)`

#### ポストプロセスアウトライン用
- `bEnableOutlineStencil = true` — MeshComp の RenderCustomDepth + Stencil = 250 を有効化
- `OutlineStencilValue = 250`
- ポストプロセスマテリアル側で Stencil 値 250 のピクセルにエッジ検出を適用してアウトライン描画する想定

#### バックライト機能は廃止済み
過去の SpotLight 実装は削除（視認性悪・パフォ重い）。現在はマーカー Widget + アウトラインで代替。

#### 公開ヘルパー
- `GetHeadLocation()` — 頭の位置（MeshHeight 加算）
- `GetRootLocation()` — ルート位置

---

### 4-6. ATomatinaCrowdMember / ATomatinaCrowdManager

#### CrowdMember（群衆 1 人）
3 つの状態:
- `Wander` — 通常徘徊
- `Burst` — ダッシュ（速い + ランダムジッター）
- `Cheer` — その場で踊る（位置固定 + ゆっくり回転）

主要プロパティ:
- `WanderSpeed = 180`、`BurstSpeed = 450`、`BurstJitter = 80`
- `ActionCheckInterval = 2.5`、`ActionCheckVariance = 1.5`
- `BurstChance = 0.25`、`CheerChance = 0.2`
- `BurstDuration = 1.5`、`CheerDuration = 2.5`
- `MaxStartJitter = 1.5` — 同期見え防止

BP イベント: `OnEnterWander` / `OnEnterBurst` / `OnEnterCheer`（AnimBP 切替用）
状態フィールド: `CurrentAction` / `CurrentSpeed` / `bIsMoving`

#### CrowdManager
- `AreaExtent` — 動き回る範囲の半幅
- `Variants` (TArray<FCrowdVariant>) — 複数種類の群衆 BP を一括スポーン
  - `MemberClass` (TSubclassOf<ATomatinaCrowdMember>)
  - `Count` (int) — 人数
  - `bMaintainCount` (bool) — 死亡時補充
- `BeginPlay` で `SpawnAll()` 呼ぶ
- 補充モードのみ Tick で `MaintenanceTimer = 2.0s` 間隔チェック

---

### 4-7. ATomatinaProjectile（トマト弾）

#### 軌道（ETomatoTrajectory）
- `Straight` — 直線（速い）
- `Arc` — 山なり放物線（Sin で Z 加算）
- `Curve` — 横カーブ（Sin で Right 軸 + CurveDirection で左右）

#### 主要プロパティ
- `FlightSpeed = 1500` (cm/s)
- `ArcHeight = 500`、`CurveStrength = 300`、`CurveDirection = 1.0`
- `SplatSizeMin = 0.16` / `SplatSizeMax = 0.30`（正規化スケール、画面幅に対する比）
- `MaxLifetime = 6.0`（プレイヤーに当たらず消える保険）

#### 衝突仕様（重要）
- `QueryOnly` + `Pawn = Overlap` + `WorldStatic = Overlap` + GenerateOverlapEvents
- `bAimedAtPlayer` (bool) — Thrower がスポーン時にセット
  - **true** + ATomatinaPlayerPawn 命中 → `OnHitCamera` → DirtManager に汚れ追加 → Destroy
  - **true** + 他の Pawn → 貫通（無視）
  - **false** + 任意の Pawn → 貫通（無視）
  - **false** + ワールド StaticMesh → `OnHitWorld` → デカール貼り → Destroy

#### ワールドデカール
- `WorldDecalMaterial` (UMaterialInterface*) — M_TomatoDecal を BP で設定
- `WorldDecalSizeMin/Max = 30 / 60` (cm)
- `WorldDecalLifetime = 0`（永続）or 秒数

`UGameplayStatics::SpawnDecalAttached` で命中した PrimitiveComponent に貼る。回転は `(-Hit.ImpactNormal).Rotation()`。

#### Initialize(Target, Trajectory)
飛行先と軌道を設定。Tick で軌道計算 + bSweep 移動でオーバーラップ確実検出。

---

### 4-8. ATomatinaThrower（投擲手キャラ）

#### 状態（EThrowerState）
- `WalkingIn` — スポーン位置から目的地まで歩く
- `Active` — 目的地到着後、投擲ループ

#### コンポーネント
- `MeshComp` (USkeletalMeshComponent)

#### アニメーション
- `ThrowMontage` (UAnimMontage*)
- `ThrowMontagePlayRate = 1.0`
- `ReleaseDelay = 0.4` — Montage 開始から手から離れるまでの秒数（0 にして AnimNotify から `ReleaseTomato()` を呼ぶ運用も可）
- `HandSocketName = "HandSocket"` — トマト発射ソケット

#### 移動
- `WalkSpeed = 250`、`ArriveTolerance = 80`
- `bIsWalking` (BlueprintReadOnly) — AnimBP 用フラグ

#### 投擲
- `ProjectileClass` (TSubclassOf<ATomatinaProjectile>)
- `ThrowInterval = 4.0` / `ThrowIntervalVariance = 1.5`
- `StartDelay = 1.0`
- `AimAtPlayerChance = 0.4` — プレイヤーを狙う確率（0〜1）
- `SceneryAimActors` (TArray<AActor*>) — 街中の狙い候補
- `SceneryAimLocations` (TArray<FVector>) — 同上、座標版
- `bUseRandomScatter = true` — シーナリ未設定時にランダム散布
- `RandomScatterMinDistance = 800`、`RandomScatterMaxDistance = 3000`
- `RandomScatterHeightRange = 200`
- `Straight/Arc/CurveChance = 0.4 / 0.35 / 0.25`

#### BP イベント
- `OnArrivedAtDestination`
- `OnThrowStarted`

#### API
- `BeginWalkIn(Destination)` — 歩行モード開始（Spawner が呼ぶ）
- `StartThrow()` — モンタージュ再生 + ReleaseDelay 後に発射
- `ReleaseTomato()` — トマト生成（BlueprintCallable、AnimNotify から直接呼んでも OK）

#### 投擲時の挙動
1. `PickAimLocation(bOutAimAtPlayer)` で狙い先確定
2. 体を狙う方向に回転（Pitch/Roll は 0）
3. ThrowMontage 再生
4. ReleaseDelay 経過後、`ReleaseTomato()` 実行
5. 手のソケット位置からトマトスポーン
6. `Tomato->bAimedAtPlayer = bAimPlayer` をセット
7. `Tomato->Initialize(AimLoc, Traj)` 呼び出し

---

### 4-9. ATomatinaThrowerSpawner

#### 役割
広場の周囲（左/右/奥）から投擲手をランダムに出現させ、広場内のランダム地点に歩かせる。
時間経過で出現間隔短縮、最大数に達したら停止。

#### 範囲（広場）
- `PlayAreaExtent = (1500, 1500, 100)` — Spawner 位置中心の半幅
- `SpawnOutsideMargin = 200` — 範囲外の出現マージン
- `GroundZOffset = 0`
- `bUseLeftEdge / bUseRightEdge / bUseBackEdge` (3 つの bool)

#### テンポ
- `InitialDelay = 2.0`
- `StartingInterval = 12.0` → 1 体出るごとに `IntervalDecreasePerSpawn = 0.5` ずつ短くなる
- `MinInterval = 3.0` で頭打ち
- `IntervalVariance = 0.6` （± 揺らぎ）
- `MaxThrowers = 8` — 同時数キャップ

#### バリアント
- `ThrowerVariants` (TArray<FThrowerVariant>) — 強さプリセット BP
  - `ThrowerClass` (TSubclassOf<ATomatinaThrower>)
  - `Weight` (float) — 抽選重み
- 重み付きランダム抽選

#### 共有狙い候補
- `SceneryAimActors` / `SceneryAimLocations` — スポーン時に各 Thrower にコピー

#### API
- `SpawnThrowerNow()` — デバッグ用即時スポーン

---

### 4-10. ATomatoDirtManager

#### 役割
カメラレンズの汚れデータを一元管理。レベルに 1 個配置必須。

#### FDirtSplat 構造体
- `NormalizedPosition` (FVector2D) — 0〜1 正規化座標
- `Opacity` (float, 1.0)
- `Size` (float, 0.2) — 正規化スケール（0〜1、1.0 で領域幅と同じ）
- `FadeSpeed` (float, 0) — 自然減衰速度
- `bActive` (bool, true)
- `TextureIndex` (int32, 0) — DirtTextures 配列参照

#### Config
- `bEnableAutoSpawn = false` — タイマー方式の自動生成（トマト命中で生成するなら false）
- `SpawnInterval = 3.0` / `MaxDirts = 50`
- `SpawnSizeMin/Max = 0.16 / 0.30` — SpawnDirt 時のサイズランダム範囲
- `MaxDirtSize = 0.6` — AddDirt のセーフティクランプ（0 で無効）
- `SpawnRangeMin/Max = (0, 0) / (1, 1)` — SpawnDirt と AddDirt 両方に適用される範囲
- `NumDirtVariants = 1` — 画像バリエーション数（HUD の DirtTextures 配列サイズに合わせる）

#### メソッド
- `SpawnDirt()` — ランダム生成（SpawnRange + SpawnSizeMin/Max 使用）
- `AddDirt(NormPos, Size)` — トマト命中時に呼ばれる。Size は MaxDirtSize でクランプ、NormPos は SpawnRange でクランプ、TextureIndex はランダム抽選
- `ClearDirtAt(NormPos, Radius)` — 半径内を即時削除
- `WipeDirtAt(NormPos, Radius, Amount)` — 半径内の Opacity を Amount × Falloff で減算（中心ほど強い）。**変化があれば NotifyHUD を呼ぶ**（透明度変化が画面にスムーズ反映）
- `GetActiveDirts()` — bActive な汚れだけ返す

#### Tick
- 自動スポーン
- 自然減衰（FadeSpeed > 0 の汚れ）
- 非アクティブ削除
- 変化があれば NotifyHUD

---

### 4-11. UTomatinaFunctionLibrary

#### 静的ヘルパー関数
- `CheckVisibility(ZoomCamera, Target, CheckLoc, ScreenW, ScreenH)` — NDC 判定 + ライントレース。**遮蔽判定**: Skeletal Mesh / 他 Pawn / "HidingProp" タグ付きアクターは遮蔽、それ以外（建物の Static Mesh 等）は無視
- `CalculatePhotoScore(ZoomCamera, Targets, CurrentMission, ScreenW, ScreenH)`:
  - Head / Root の 2 点で CheckVisibility
  - 両方見える: 100、上半身のみ: 50、足のみ: 10、写ってない: 0
  - **ポーズボーナス**: BestTarget が `bIsPosing` なら `Score *= PoseScoreMultiplier`
- `CalculateZoomOffset(PC, Hit, Camera, FOV)` — クリック位置をズーム中心に持ってくるオフセット
- `GetZoomScreenCenter(MainW, PhoneW, PhoneH)` — iPhone 領域の中心座標
- `ProjectZoomToMainScreen(PC, SceneCapture, MainW, MainH)` — SceneCapture 視線方向をメイン画面ピクセルに投影
- `CopyZoomToPhoto(ZoomCamera, PhotoTarget)` — RT 差し替え方式で写真取得（RHI 不使用）

#### FPhotoResult
- `Score` (int)
- `Comment` (FString)
- `BestTarget` (ATomatinaTargetBase*)

---

## 5. Blueprint セットアップチェックリスト

### 必須 BP
- `BP_TomatinaGameMode` (ATomatinaGameMode 継承)
- `BP_TomatinaPlayerPawn` (ATomatinaPlayerPawn 継承)
- `BP_TomatinaHUD` (ATomatinaHUD 継承)
- `BP_TomatoDirtManager` (ATomatoDirtManager 継承) — **レベルに 1 個配置必須**
- `BP_TomatinaTargetSpawner` (ATomatinaTargetSpawner 継承) — レベル配置
- `BP_TomatinaProjectile` (ATomatinaProjectile 継承)
- `BP_TomatoThrower` (ATomatinaThrower 継承) — 強さ別に複数バリアント作成推奨
- `BP_ThrowerSpawner` (ATomatinaThrowerSpawner 継承) — レベル配置
- `BP_CrowdMember_*` (ATomatinaCrowdMember 継承) — 種類ごとに作成
- `BP_CrowdManager` (ATomatinaCrowdManager 継承) — レベル配置
- `BP_Target_*` (ATomatinaTargetBase 継承) — お題ごとに作成

### 必須 Widget
- `WBP_ViewFinder` — メイン画面のフレーム
- `WBP_Cursor` — IMG_Crosshair（CanvasPanel）
- `WBP_DirtOverlay` — 汚れと**タオル UI** を載せる
  - `SplatContainer` (UCanvasPanel) — 汚れ動的生成先
  - `IMG_Towel` (UImage) — タオル画像（Hidden 開始）
  - `PB_TowelStamina` (UProgressBar) — 耐久バー
  - `TXT_SwapMessage` (UTextBlock) — 「タオル交換中...」
- `WBP_PhotoResult` — 撮影リザルト
  - `IMG_Photo` (UImage)
  - `SplatContainer` (UCanvasPanel) — IMG_Photo と同サイズ・同位置（SizeBox + Overlay 推奨）
- `WBP_MissionDisplay` — お題表示
- `WBP_MissionResult` — ミッション結果
- `WBP_FinalResult` — 最終結果
- `WBP_ShutterFlash` — 撮影フラッシュ
- `WBP_Countdown` — カウントダウン
- `WBP_TestPip` — テストモード PiP
- `WBP_TargetMarker` — 頭上マーカー（任意）

### 必須 Material
- `M_ZoomDisplay` — ズームビュー表示
  - Material Domain = User Interface、Shading = Unlit
- `M_PhotoDisplay` — 撮影写真表示（同設定）
- `M_TomatoDecal` — ワールドのトマト汚れ
  - Material Domain = **Deferred Decal**
  - Blend Mode = Translucent
  - Decal Blend Mode = Translucent
- `M_TargetOutline`（オプション、ポストプロセス）
  - Material Domain = Post Process
  - Blendable Location = Before Tonemapping（UE5 では Material Domain を Post Process にしないと出ない）
  - CustomDepth Stencil 値 250 のピクセルを縁取る

### 必須 Render Target
- `RT_Zoom` — SceneCapture_Zoom 出力
- `RT_Photo` — 撮影画像出力
- 設定: `RTFormat = RTF_RGBA8`、sRGB = true

### SceneCapture_Zoom の重要設定
- Capture Source = **Final Color (LDR) in RGB**
- bCaptureEveryFrame = true（C++ で設定済み）
- TextureTarget = RT_Zoom

---

## 6. LeapMotion 拭き取りシステム（BP 実装ガイド）

### 全体フロー
1. プレイヤー Pawn に Leap Component 追加
2. BeginPlay で `Bind Event to On Leap Tracking Data` → Custom Event を作る
3. 毎フレーム、手の Position を 0〜1 正規化
4. 速度を計算（前フレームとの差 / DeltaTime）
5. 速度しきい値以上なら WipeDirtAt 呼ぶ
6. 同時にタオル耐久を消費
7. 耐久 0 で SwapTowel（2 秒交換アニメ）

### 必要な BP 変数（BP_TomatinaPlayerPawn）
| 変数 | 型 | 初期値 | 公開 |
|------|-----|-------|-----|
| DirtManagerRef | TomatoDirtManager Object Ref | - | - |
| HUDRef | TomatinaHUD Object Ref | - | - |
| TowelDurability | Float | 1.0 | - |
| bTowelSwapping | Bool | false | - |
| LastHandNormPos | Vector 2D | (0.5, 0.5) | - |
| LastHandValid | Bool | false | - |
| WipeRadius | Float | 0.08 | ★ |
| WipeBaseAmount | Float | 2.0 | ★ |
| WipeMinSpeed | Float | 0.05 | ★ |
| WipeSpeedScale | Float | 5.0 | ★ |
| DurabilityCostScale | Float | 0.3 | ★ |
| LeapMinX/MaxX/MinY/MaxY | Float | 環境次第（cm） | ★ |

★ = Instance Editable

### 軸マッピング（環境による）
標準的なデスクトップ配置（Leap を机に上向き）の場合（プラグインの UE 座標変換後）:
- 横（NormX）の Value = `Position.Y`
- 縦（NormY）の Value = `Position.Z`（OutRange を 1.0 → 0.0 で反転）
- 高さ（手前/奥） = `Position.X`（拭き取りでは未使用）

**実測必須**: Print String で 3 軸表示し、手を上下/左右に動かして変動軸を確認すること。

### 「振れば振るほど」公式
```
Speed = (HandNormPos - LastHandNormPos).Length / DeltaSeconds
Amount = WipeBaseAmount × min(Speed × WipeSpeedScale, 5.0)
DamageThisFrame = Amount × DeltaSeconds
```

### スタミナ消費
```
Cost = Amount × DeltaSeconds × DurabilityCostScale
TowelDurability = Max(TowelDurability - Cost, 0)
```

### 手なし判定（OnLeapHandLost が無いプラグインバージョン用）
OnLeapTrackingData の冒頭で `Hands.Length <= 0` なら:
- `Set LastHandValid = false`
- `HUDRef → Hide Towel`
- For Each Loop に行かず終了

### タオル位置表示
1. WBP_DirtOverlay に IMG_Towel（Image）を追加（Anchor: TopLeft、Hidden 開始）
2. OnLeapTrackingData の手あり処理内:
   - HandNormPos 計算後、`Update Towel Position(HandNormPos)` → `Show Towel` の順で呼ぶ
   - 順番が逆だと前回の位置で 1 フレーム表示される

### SwapTowel カスタム関数 / イベント
**Functions パネル内では Delay が使えないため Custom Event 推奨**
```
[Custom Event: SwapTowel]
   ↓
[Set bTowelSwapping = true]
[HUDRef → UpdateTowelStatus(0, true)]
   ↓
[Delay 2.0]
   ↓
[Set TowelDurability = 1.0]
[Set bTowelSwapping = false]
[HUDRef → UpdateTowelStatus(1.0, false)]
```

---

## 7. 既知の問題 / 注意事項

### Git / 大容量バイナリ
- `.umap` / `.uasset` は**バイナリ**。`.gitattributes` で `binary` 指定必須
- 推奨: **Git LFS** で `*.uasset` `*.umap` を track
- 過去に Mac の友達に送ったとき 1KB → 295B で壊れた事例あり（CRLF 変換が原因）
- `.gitattributes` 例:
  ```
  *.uasset binary
  *.umap binary
  *.ubulk binary
  ```

### Live Coding が壊れる場面
- USTRUCT のメンバ追加・削除 → 一度クリーンビルド推奨
- UENUM の値追加 → Live Coding で OK なことが多い
- UFUNCTION のシグネチャ変更 → Live Coding で OK
- USTRUCT の `GENERATED_BODY` 周りを触ると Live Coding 失敗多

### Clang 偽陽性
前述。`'CoreMinimal.h' file not found` 等は無視。

### マウスカーソル
- ズーム中は iPhone 中央に飛ばし、ズーム解除時はクランプで自然に戻る
- **撮影後は手動でカーソル位置を変えない**（仕様。以前の自動センタリングは削除済み）

### iPhone 領域へのカーソル流入
- Tick の Clamp で防いでる
- ただし `bShowMouseCursor = true` のときだけ。ズーム中は対象外

### SceneCapture が暗く見える問題（解決済み）
- Capture Source デフォは HDR pre-tonemapped → 暗い
- **Final Color (LDR) in RGB に変更**（C++ では SceneCapture_Zoom の bCaptureEveryFrame と FOVAngle のみセット、Capture Source は BP で設定）
- マテリアル: User Interface + Unlit、RT: RTF_RGBA8 + sRGB

### 汚れが急に消える問題（解決済み）
- WipeDirtAt が NotifyHUD を呼んでなかった → 透明度変化が見えなかった
- 修正済み。WipeDirtAt と Tick の自然減衰両方で変化検出時に NotifyHUD

### 汚れがくそデカサイズで生成される問題（解決済み）
- 旧コードで `FDirtSplat::Size` のデフォルトが `100.0f`（正規化想定で）→ 画面の 100 倍の汚れ
- 修正済み: デフォを 0.2 に、SpawnDirt も SpawnSizeMin/Max からランダム取得、AddDirt に MaxDirtSize クランプ追加

### 汚れの SpawnRange
- 旧コード: Projectile 側で `0.1〜0.9` ハードコード → 画面端に出ない
- 修正済み: Projectile は `0.0〜1.0` 全範囲、Manager の `SpawnRange` で AddDirt も含めクランプ

### LeapMotion で OnLeapHandLost が見つからない
- プラグインバージョンによっては存在しない
- 代替: OnLeapTrackingData 内で `Hands.Length <= 0` チェック

### LeapMotion タオルの初期位置が画面上に出る
- 原因候補: `LastHandNormPos` 初期値が `(0,0)`、または手なしフレームに UpdateTowelPosition を呼んでる、または Map Range の入力軸間違い
- 対策: 初期値を `(0.5, 0.5)` に、手なし時は HideTowel + 早期 return、Map Range の入力を Print String で実測

---

## 8. ファイル別変更履歴（2026-04 までの主要修正）

### TomatinaProjectile
- `bAimedAtPlayer` 追加（プレイヤー狙い vs 街狙いの分岐）
- `WorldDecalMaterial` / `WorldDecalSizeMin/Max` / `WorldDecalLifetime` 追加
- `OnHitWorld` 実装（StaticMesh 命中でデカール貼り）
- WorldStatic を Overlap に
- 命中位置のランダム化を `0.1〜0.9` → `0.0〜1.0`
- MaxLifetime 追加、bSweep 移動

### TomatinaThrower
- 歩行モード追加（BeginWalkIn / WalkingIn 状態 / TickWalk）
- AimAtPlayerChance + SceneryAim 配列で狙い分岐
- bUseRandomScatter + RandomScatterMin/MaxDistance + HeightRange 追加
- 投擲時の体の自動回転
- Tomato->bAimedAtPlayer をスポーン直後にセット
- BP イベント: OnArrivedAtDestination / OnThrowStarted

### TomatinaThrowerSpawner（新規）
- 広場周囲（左/右/奥）からランダム出現
- 時間経過で間隔短縮
- ThrowerVariants で重み付き種類選択

### TomatinaCrowdMember + Manager（新規）
- Wander/Burst/Cheer 状態
- Variants で複数種類混在

### TomatinaTargetBase
- ポーズシステム追加（bEnablePose, PoseChance, PoseDuration, PoseScoreMultiplier）
- 頭上マーカー Widget 追加
- アウトライン用 CustomDepth Stencil 自動有効化
- バックライト SpotLight は削除

### TomatoDirtManager
- `bEnableAutoSpawn` (default false) 追加
- `MaxDirts` 50、`SpawnSizeMin/Max`、`MaxDirtSize`、`SpawnRangeMin/Max`、`NumDirtVariants` 追加
- AddDirt にサイズクランプ + SpawnRange クランプ
- WipeDirtAt と Tick(自然減衰) で変化時に NotifyHUD 呼ぶ
- FDirtSplat に TextureIndex 追加

### TomatinaHUD
- DirtTextures 配列追加（複数バリエーション対応）
- AddDirtSplatsToCanvas で TextureIndex から画像選択 + Size クランプ
- UpdateTowelPosition / ShowTowel / HideTowel 追加
- ShowResult が Dirts も受け取るように

### TomatinaPlayerPawn
- ZoomLookSensitivity / ZoomPitchLimit / ZoomYawLimit 追加（広範囲視点移動）
- 撮影後の自動カーソルセンタリング削除
- Tick でカーソルをメインモニター内にクランプ

### TomatinaFunctionLibrary
- CheckVisibility: Static Mesh は遮蔽無視、Skeletal Mesh / Pawn / "HidingProp" タグは遮蔽
- CalculatePhotoScore にポーズボーナス（PoseScoreMultiplier）

---

## 9. やる予定のこと（TODO）

### 高優先度
- [ ] LeapMotion タオル拭き取りの BP 実装完了（仕様は §6）
- [ ] `M_TomatoDecal` マテリアル作成（Deferred Decal）
- [ ] `WBP_TargetMarker` 作成（頭上アイコン）
- [ ] Group の Spawner / Crowd / Thrower BP を全種類用意
- [ ] 強さ別 Thrower BP（Weak / Normal / Strong）作成

### 中優先度
- [ ] ポストプロセスアウトライン Material（M_TargetOutline）作成
- [ ] ターゲットの AnimBP（State Machine: Idle / Walk / Pose / Cheer）
- [ ] サウンド: シャッター音、トマト命中 SE、タオル拭き SE、BGM

### 低優先度
- [ ] リザルト演出（スコア表示アニメーション）
- [ ] 写真をギャラリーに残す機能
- [ ] 難易度設定 UI

### バグ・改善
- [ ] LeapMotion 手なし時のタオル位置リセット確認
- [ ] PIE で 3584×1600 のウィンドウ設定が反映されない場合の対処メモ整備
- [ ] CalculatePhotoScore で「写ってない」ときの最低保証点（現状 0 → 5 等）

---

## 10. 開発上の重要原則

### コード方針
- C++ で骨組み、BP で見た目・挙動微調整 という分業
- 数値はなるべく `UPROPERTY(EditAnywhere)` で BP からチューニング可能に
- スクリーンサイズは `ATomatinaPlayerPawn` の 4 変数を Single Source of Truth に
- `UE_LOG(LogTemp, Warning/Log, ...)` を要所に入れてデバッグしやすく
- マジックナンバーは `UPROPERTY` のデフォルト値に追い出す

### Widget 操作の原則
- 位置・サイズの変更は原則 BP（Designer）でやる
- C++ で動的操作するのは Cursor / Towel / 動的汚れ生成のみ（明確に例外として記載）
- HUD はあくまで「Widget の生成・表示・データ更新」に専念

### 衝突方針
- トマト = QueryOnly + Pawn/WorldStatic Overlap
- ターゲット = ECollisionEnabled は通常通り
- HidingProp タグを付けたアクター = 遮蔽として扱う

### コミットしないファイル
- `Binaries/`、`Intermediate/`、`Saved/`、`.vs/`、`DerivedDataCache/`
- 自動生成 `.generated.h`

---

## 11. テスト・確認手順

### スモークテスト
1. PIE 起動（解像度 3584 × 1600）
2. プレイヤーが動かないことを確認（DefaultPawn 入力無効化済み）
3. 右クリックでズーム → カーソルが iPhone 中央に飛ぶ
4. ズーム中にマウス移動で視点回転
5. 左クリックで撮影 → リザルト画面表示
6. リザルト後カーソルがそのまま残ってる（自動移動なし）
7. カーソルが iPhone 領域に行こうとしても止まる

### 投擲テスト
1. BP_ThrowerSpawner をレベル配置
2. プレイ開始 → 12 秒後に最初の Thrower 出現、歩いて広場へ
3. 到着後、ThrowMontage 再生 → トマト発射
4. プレイヤー命中 → カメラに汚れ出現
5. 街命中 → 街の StaticMesh にデカール

### 群衆テスト
1. BP_CrowdManager をレベル配置 + Variants に CrowdMember BP 設定
2. プレイ開始 → 範囲内に群衆スポーン
3. 一人ずつ違う方向に動き、たまにダッシュ・踊る

### 汚れテスト
1. トマト命中 → 汚れがメイン + iPhone の同じ正規化位置に出る
2. WBP_DirtOverlay に動的に UImage 追加されてる
3. 撮影 → リザルト画面の SplatContainer に同じ汚れ複製
4. 写った汚れ数 × DirtPenaltyPerSplat (-5) でスコア減点

---

## 12. 重要な数値リファレンス

### スコア
- 全身バッチリ: 100、上半身: 50、足のみ: 10、写ってない: 0
- ポーズボーナス: × `PoseScoreMultiplier` (default 2.0)
- 汚れ減点: 1 個あたり `-5` × 写った数

### タイミング
- ミッション制限時間: 15 秒（個別変更可）
- 撮影リザルト表示: 3 秒
- ミッション結果表示: 2 秒
- タオル交換: 2 秒（Delay）

### 投擲
- ThrowInterval: 4 ± 1.5 秒
- Spawner 出現間隔: 12 → 3 秒に短縮
- Max Throwers: 8 体

### 汚れ
- AddDirt サイズ: 0.16 〜 0.30（正規化）
- 自然減衰: 0（命中で生成、拭き取りで消える）
- 最大保持: 50 個

### LeapMotion
- WipeRadius: 0.08（正規化）
- 完全静止 → 拭けない（WipeMinSpeed = 0.05）
- ゆっくり拭き: 1 秒で約 0.05 耐久消費（20 秒もつ）
- 高速振り: 1 秒で 1.0 耐久消費（即交換）

---

## 13. 連絡事項（引き継ぎ前任者から Copilot への注意）

- このプロジェクトは展示会用のため、**即座にプレイヤーが理解できる UX** を最優先
- 動作の重さよりも分かりやすさ・派手さ優先
- 群衆 30 人 + 投擲手 8 体 + 汚れ 50 個 同時動作で重ければ Manager 側の Tick 間隔調整
- LeapMotion は実機がないと動作確認不可。BP のロジックは Print String で値を見ながらキャリブレーションする必要あり
- 写真機能の RT 差し替え方式（`CopyZoomToPhoto`）は RHI 非使用で安全。RHI 直叩きには手を出さないこと
- 既存ファイル `MyActor.{h,cpp}` は未使用なので削除して構わない
- 旧 `TomatinaProjectileSpawner` は新 `Thrower / ThrowerSpawner` で置き換え予定。レベルから消して OK

---

## 14. Copilot への作業依頼テンプレート

新機能を依頼するときは以下のフォーマットで:

```
## やりたいこと
（簡潔に 1〜2 文）

## 影響を受けるファイル / クラス
- Source/Tomato/XxxXxx.h
- Source/Tomato/XxxXxx.cpp
- Content/Blueprints/BP_Yyy

## 仕様
- ...
- ...

## 注意点
- Clang 偽陽性は無視
- Live Coding でビルド確認
- BP 側の手順も日本語で書く
```

---

以上。これを `COPILOT_HANDOFF.md` として根に置けば、Copilot が `@workspace` でプロジェクト全体を把握できる。

---

## 15. セッション引き継ぎ追記（2026-04-23 追記）

この章は、今回の会話で実施した変更内容と、未解決項目（iPhone 側 ZoomView 表示）を再開しやすくするための追加メモ。

### 15-1. 今回 C++ で追加・変更した内容

#### A) スタイリッシュランク（コンボ）基盤
- `ATomatinaGameMode` にランク `C/B/A/S/SSS`、ゲージ、コンボ、汚れ占有率連動減衰を追加
- 高得点連続撮影でゲージ加算、失敗や汚れ過多で減衰
- BP イベント追加
  - `OnStylishRankChanged(EStylishRank NewRank, EStylishRank OldRank, bool bRankUp)`
  - `OnHighStylishShot(ATomatinaTargetBase* Target, EStylishRank Rank, int32 ShotScore)`

#### B) HUD のスタイリッシュ表示 API
- `ATomatinaHUD::UpdateStylishDisplay(...)` を追加
- `WBP_MissionDisplay` 内の下記名前を参照して更新
  - `TXT_StylishRank`
  - `TXT_StylishCombo`
  - `PB_StylishGauge`
- 起動時に名前チェックする `ValidateMissionStylishWidgets()` も追加

#### C) 特殊トマト（黄色粘着）
- `ATomatoDirtManager` に `EDirtType` と粘着仕様を追加
  - `StickyYellowDash` は連続ダッシュ拭き回数（デフォ 4）達成まで残る
  - しきい値: `StickyDashMinAmount`, `StickyDashMinMoveDistance`, `StickyDashMaxInterval`
- `ATomatinaProjectile` に命中バリエーション追加
  - `ETomatoImpactVariant { NormalRed, StickyYellow }`
  - `StickyYellowChance` で発生率制御

#### D) ターゲットごとの撮影点調整
- `ATomatinaTargetBase` に個別スコア設定を追加
  - `FullBodyScore`, `UpperBodyScore`, `LowerBodyScore`
  - `PhotoScoreMultiplier`, `PhotoScoreFlatBonus`
  - `HighRankBonusScore`
- `UTomatinaFunctionLibrary::CalculatePhotoScore()` で個別値を使用するよう変更

### 15-2. iPhone 側 ZoomView バグ対応（今回の主題）

症状:
- テストモード OFF 時に iPhone 側へズーム映像が出ず、メイン画面が拡張されて見える

実施した対策（C++）:
1. `WBP_ViewFinder` に対して Zoom 表示マテリアルを必ずバインド
2. iPhone 領域へ Zoom Image の位置・サイズを強制レイアウト
   - `Pos = (MainWidth, (MainHeight - PhoneHeight)/2)`
   - `Size = (PhoneWidth, PhoneHeight)`
3. `IMG_ZoomView` が見つからない場合のフォールバック探索（名前に Zoom/Phone/Finder を含む `UImage`）
4. それでも見つからない場合、`IMG_ZoomView_Runtime` を実行時生成
5. 可能な場合は `RT_Zoom` を `UImage` へ直接バインド（マテリアル経由を回避）
6. `SceneCapture_Zoom->TextureTarget` 未設定時に起動ログでエラー表示

### 15-3. 画面サイズのデフォルト更新

今回の依頼値に合わせて、デフォルトを下記へ更新済み:
- Main: `2560 x 1600`
- Phone: `2256 x 1179`

対象:
- `ATomatinaPlayerPawn`
- `ATomatinaHUD`
- `ATomatinaGameMode` のフォールバック値

### 15-4. 再開時の最優先チェックリスト

1. `BP_TomatinaPlayerPawn` のインスタンス値が C++ デフォルトを上書きしていないか確認
   - `MainWidth=2560`, `MainHeight=1600`, `PhoneWidth=2256`, `PhoneHeight=1179`
2. `SceneCapture_Zoom` の `TextureTarget` が `RT_Zoom` か確認
3. `WBP_ViewFinder` に Zoom 用 Image が存在するか確認（推奨名 `IMG_ZoomView`）
4. 起動ログで以下を確認
   - 成功: `RT_Zoom を直接バインドしました`
   - 代替: `ZoomDisplayMaterial をバインドしました（RT 未検出のため）`
   - 失敗: `TextureTarget が未設定です` / `ZoomView 生成先 CanvasPanel が見つかりません`

### 15-5. まだ直らない場合の次アクション

`WBP_ViewFinder` のルート階層が Canvas 前提でない可能性がある。
次回は以下を確認して、親スロット型別に C++ レイアウト処理を分岐実装する。
- `IMG_ZoomView` の親が `CanvasPanel` / `Overlay` / `SizeBox` / `ScaleBox` のどれか
- ルート Widget タイプ
- ルートに SafeZone や DPI スケーラを噛ませていないか

### 15-6. 作業ツリー注意

- 今回、`Content/Chiled_BP/BP_TomatinaPlayerPawn.uasset` に既存差分が検出されている。
- この差分は本セッションで直接編集したものではないため、次回は意図差分かどうかを先に確認すること。
