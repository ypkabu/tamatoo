<<<<<<< HEAD
# Tomatina ― ES 用ゲーム総合解説（ES 用 Claude への申し送り）

> このファイルは「ES 添削/作文を担当する Claude」が、本作の内容・技術・苦労・残課題をすべて理解できるように書かれている。
> ES 本文・面接想定 Q&A・ガクチカ用のテキストを生成するときは、このファイルを **第一の根拠資料** として使うこと。

---

## 0. 一行サマリ

**スペインのトマト祭り「La Tomatina」をモチーフにした、メインモニター + スマートフォン + LeapMotion による 2 人協力体験型ゲーム。Unreal Engine 5.7 / C++ / Blueprint。1P はカメラマンとしてトマトを発射し、2P は LeapMotion でメイン画面の汚れをタオルで拭く。**

---

## 1. プロジェクト基本情報

| 項目 | 内容 |
|------|------|
| タイトル | **Tomatina** |
| リポジトリ名 | `tamatoo` |
| エンジン | Unreal Engine 5.7（`Tomato.uproject` の `EngineAssociation` で確定） |
| 言語 | C++ + Blueprint 併用 |
| 主要プラグイン | `UltraleapTracking`（LeapMotion）, `OpenXR`, `ModelingToolsEditorMode`, `GameplayStateTree` |
| 想定ハード | メインモニター（2560×1600 想定）+ スマートフォン縦置き拡張ディスプレイ + LeapMotion |
| プレイ人数 | 2 人協力（1P=カメラマン、2P=タオル拭き取り） |
| パッケージング | Monolithic / `Tomato.exe` 約 297MB |
| 用途 | 学内展示・試遊会・就活ポートフォリオ（ES, Cygames インターン応募） |
| 想定プレイ時間 | 要確認（数分のセッション制） |
=======
# Tomatina（タマトゥー）— ES 用ゲーム詳細説明書

このドキュメントは **ES（エントリーシート／自己 PR）作成を支援する Claude へ向けて、本ゲームの内容を漏れなく伝える**ことを目的とする。
作者がエピソードトーク・志望動機・ガクチカ・自己 PR・面接想定問答を組み立てる際に、
**「このゲームで何を作り、どんな課題に対しどんな技術判断をしたか」** を即引きできるようにする。

---

## 0. 一行要約

**LeapMotion ハンドトラッキング × デュアルディスプレイ（モニター＋スマホ）を組み合わせた、
２人協力プレイの「カメラマン体験ゲーム」**。
1P が祭り会場（モチーフ：スペイン・ブニョルの「ラ・トマティーナ」）でカメラマンとなり、
お題に沿った被写体を撮影する。観客（NPC・２P）が投げてくるトマトでカメラレンズが汚れていくのを、
2P が LeapMotion に手をかざしてリアルタイムに拭き取って 1P をサポートする。

---

## 1. 基本情報

| 項目 | 値 |
|---|---|
| タイトル | **Tomatina（タマトゥー）** |
| ジャンル | 協力体験型ミニゲーム／撮影アクション |
| プレイ人数 | 2 人協力（1P：カメラマン／2P：レンズ拭き） |
| 想定プレイ時間 | 1 セッション 3〜5 分 |
| プラットフォーム | Windows（スタンドアロン `.exe`） |
| 開発エンジン | Unreal Engine 5.7 |
| 主要言語 | **C++**（コアロジック）＋ **Blueprint**（演出・UI バインド） |
| 開発体制 | 個人開発（コード・設計・統合は本人。アセットは一部購入素材を使用） |
| 入力ハード | キーボード／マウス（1P）、**LeapMotion / Ultraleap**（2P）、外部スマホディスプレイ（1P 用ファインダー） |
| 出力 | **メインモニター（2560×1600）＋ スマホ画面（2024×1152）の２画面構成** |
>>>>>>> 3d651346c6e35aacce63e4ee5f2b056db33db6b0

---

## 2. ゲームコンセプト

<<<<<<< HEAD
### 2.1 元ネタ

スペイン・ブニョール村で毎年 8 月最終水曜に開催される **トマト投げ祭り「La Tomatina」**。街中がトマトで汚れる、世界最大級の食べ物投げ祭り。

### 2.2 体験のコア

「祭りに参加した 2 人」をそのまま操作役割に分けた:

- **1P = カメラマン**: トマトを街にぶつけ、汚れる瞬間を“撮る”
- **2P = 巻き込まれる側 / 拭き取り役**: タオルで画面の汚れを物理的に（手をかざして）拭き取る

ゲーム上、**1P と 2P は同じスクリーンを共有しない**:

- 1P はメインモニター（三人称ゲーム画面）を見る
- 2P は LeapMotion を覗き込みながら、**メインモニターの絵に向かって手をかざす**
- スマホ画面は **1P のためのファインダー（望遠ズーム映像）** として右に置かれる

「**視線（1P）と手（2P）が物理的に交差する**」配置を体験のコアに据えている。

### 2.3 相互依存ルール

- 1P が撃たないと 2P がやることがない
- 2P が拭かないと、1P の構図がトマト汚れで埋まっていく
- → **2 人の協調度（Sync Rate）** がスコアに直結する
=======
> 「**祭りの興奮の真っ只中で、お題の決定的瞬間をカメラに収めろ。レンズが汚れる前に**。」

- スペインのトマト祭り「ラ・トマティーナ」を題材にした賑やかなお祭り空間
- カメラマン（1P）の役割は、**お題で指定された被写体を制限時間内に "決定的瞬間" で撮る**こと
- 観客（2P or NPC）が投げてくる**トマトがカメラのレンズに当たり、汚れがどんどん溜まっていく**
- 汚れたまま撮影するとスコアが大幅減点 → そこで **2P が LeapMotion で実際の手を画面の前にかざして物理的に拭き取る**
- ２人で「**1P：構図と決定的瞬間**」「**2P：レンズの清掃ペース**」を分担し、シンクロ率と総合点を競う

### コア体験の柱
1. **「自分の手」でレンズを拭くフィジカル操作感**（LeapMotion）
2. **物理的に分かれた２画面**による「カメラを覗く」感覚（メインモニター＋スマホ）
3. **協力プレイのリズム**：1P がズームしている数秒の間に 2P がどれだけ汚れを落とせるか
>>>>>>> 3d651346c6e35aacce63e4ee5f2b056db33db6b0

---

## 3. ゲームプレイ詳細

<<<<<<< HEAD
### 3.1 ゲームフロー

```
[ロード中表示] → [カウントダウン] → [ミッション提示]
   → ┌───────────────────────────────────────┐
     │  プレイループ:                            │
     │   1P: 視点操作 / 右クリックでズーム /     │
     │       左クリックでトマト発射               │
     │   2P: LeapMotion で手を動かして汚れを拭く │
     │   汚れが追加・拭かれを繰り返す            │
     │   スタイリッシュランクが C → SSS で変動    │
     └───────────────┬───────────────────────┘
                     ↓
         [ミッション終了 / 全汚れクリア]
                     ↓
            [ミッション結果表示] → 次ミッション
                     ↓
                [最終リザルト]
              （Total Score / 平均ランク / Sync Rate）
=======
### 3.1 ゲーム進行フロー

```
レベルロード
  ↓
「ロード中...」表示（必要アクター生成完了 + 最低 LoadingHoldSeconds 秒）
  ↓
カウントダウン 3 → 2 → 1 → スタート！（SE：CountdownTickSound, CountdownGoSound）
  ↓
ミッション 1 開始（ShowMissionDisplay でお題テキストとプレビュー画像を表示）
  ↓
[ミッション中ループ]
  ・1P：マウスで視点回転、右クリックでズーム、左クリックで撮影
  ・2P：LeapMotion に手をかざして汚れを拭く
  ・観客 NPC：トマトを投げる → カメラ命中で汚れ追加
  ・スタイリッシュゲージ：高得点撮影連打で C → B → A → S → SSS にランクアップ
  ・ゲーム全体タイマーが PB_GameTimer で減っていく
  ↓
（撮影成功 / 時間切れ / 全ミッション完了 のいずれか）
  ↓
リザルト溜め演出（FinalResultBuildup：BGM のみ残し他音停止、汚れ全削除）
  ↓
最終リザルト画面：総合点 / ランク（C〜S） / 平均スタイリッシュランク / 1P-2P シンクロ率（％）
>>>>>>> 3d651346c6e35aacce63e4ee5f2b056db33db6b0
```

### 3.2 1P の操作（カメラマン）

<<<<<<< HEAD
- **入力デバイス**: キーボード + マウス（Enhanced Input）
- **アクション**:
  - `IA_Look`（マウス移動）: 視点旋回
  - `IA_RightMouse`: 押している間ズーム（FOV `90 → 30`、`ZoomSpeed = 5`）
  - `IA_LeftMouse`: トマト発射（`ATomatinaProjectile` を spawn）
- **ズーム時の特殊挙動**:
  - 視点感度が `ZoomLookSensitivity` 倍に（細かい構図合わせ用）
  - Pitch を `ZoomPitchLimit`（既定 89°）で制限、Yaw は 0 で無制限
  - Sphere Sweep + Safety Margin + 近接クリップで壁抜け対策（`HARDWARE_CHALLENGES.md` §2.4）
  - 空にトレースが当たらない場合 `SkyFallbackDistance`（既定 30m）で仮想ヒット（同 §2.5）

### 3.3 2P の操作（タオル拭き取り）

- **入力デバイス**: LeapMotion（Ultraleap Tracking）
- **アクション**: 手を LeapMotion の上にかざすと、その位置がメイン画面上の **正規化座標 (0〜1)** に変換され、`ATomatoDirtManager::WipeDirtAt(NormPos, Radius, Amount)` が呼ばれる
- **拭き取りモデル**:
  - 中心ほどフォールオフが大きい（中心ほど一気に拭ける）
  - 1 フレームの `Amount` は `DeltaSeconds` 掛け運用を想定
  - 半径は **汚れ自身の `Size * 0.5f`** が加算された実効半径

### 3.4 汚れ（`FDirtSplat`）

`Source/Tomato/TomatoDirtManager.h` の `FDirtSplat` 構造体で管理。主要フィールド:

| フィールド | 役割 |
|-----------|------|
| `NormalizedPosition` | 画面上の正規化座標 (0〜1) |
| `Opacity` | 不透明度。0 で消滅扱い |
| `Size` | 正規化スケール (0〜1)。1.0 で領域幅と同サイズ |
| `FadeSpeed` | 自然減衰速度（0 で減衰なし） |
| `bActive` | 削除対象フラグ |
| `TextureIndex` | HUD の `DirtTextures` 配列インデックス |
| `DirtType` | `NormalRed` / `StickyYellowDash` |
| `RequiredDashCount` | 特殊トマト用、必要連続ダッシュ回数 |
| `CurrentDashCount` | 現在の連続ダッシュ回数 |
| `LastDashTime` / `LastDashPos` | ダッシュ判定の時刻・位置 |

#### 通常トマト（赤・`NormalRed`）

- `WipeDirtAt` の `Amount` 分だけ `Opacity` が漸減
- `Opacity` が 0 になったら `bActive = false` で削除

#### 特殊トマト（黄・`StickyYellowDash`）― ミニゲーム要素

ただ拭いても消えない。**短時間で大きく手を動かす（=ダッシュ）を 4 回連続で成立** させる必要がある。

判定パラメータ（`ATomatoDirtManager` 側で `EditAnywhere`）:

- `StickyRequiredDashCount = 4` （必要連続回数）
- `StickyDashMinAmount = 0.03f`（最小拭き取り量）
- `StickyDashMinMoveDistance = 0.06f`（最小移動距離・正規化）
- `StickyDashMaxInterval = 0.28f`（連続扱い最大間隔・秒）
- `StickyProgressMinOpacity = 0.35f`（途中段階の最小不透明度・進捗視覚化）

**ゲームデザイン意図**: 単調な拭き取りに「短い瞬発技」リズムを混ぜることで、Sync Rate のメリハリを作る。

### 3.5 スコアシステム

- **スタイリッシュランク**: `C → B → A → S → SSS` の 5 段階。連続拭き取り・コンボの維持で上昇、ミスで下降。
- **Sync Rate**: 1P の発射と 2P の拭き取りの時系列マッチ度を 0〜1 で計測。
- **Total Score**: ミッション横断の累計。
- 表示は `ATomatinaHUD::ShowFinalResult(TotalScore, MissionCount, AverageStylishRank, SyncRate01)`。

### 3.6 演出・音

- **シャッターフラッシュ**: `PlayShutterFlash`（`ShutterFlashDuration` 秒、Z-Order 9999）
- **拭き取り SE のループ**: `UpdateWipeSound`、立ち上がり/立ち下がりエッジ検出で Start/Stop を 1 回ずつ
- **ズーム In/Out SE**: `ZoomInSound` / `ZoomOutSound`（`FTomatinaSoundCue`）
- **キャラ全体に汚れマスク**: `SetOverlayMaterial`（UE 5.0+）でトマト命中部位の見た目変化
=======
| 操作 | 入力 | 効果 |
|---|---|---|
| 視点回転 | マウス移動 | カメラの向きを変える |
| ズーム開始 | 右クリック押下 | クリックした地点へカメラが寄る（FOV: 90 → 30、位置オフセット計算） |
| ズーム解除 | 右クリック離す | ズーム前に戻る |
| 撮影 | ズーム中に左クリック | RT_Photo に SceneCapture スナップ → 採点 → リザルト Widget |

**ズームは「クリックした位置に応じてカメラを動かす」設計**。`UTomatinaFunctionLibrary::CalculateZoomOffset()` で
画面端をクリックすればカメラが横に大きく動き、中央付近なら前進ズームになる。これにより**「望遠レンズで覗き込む感覚」**を再現。

ズーム中の壁めり込み防止：球形スイープ（`ZoomOffsetSweepRadius=15cm`）+ 安全距離マージン（`ZoomOffsetSafetyMargin=25cm`）+ `CustomNearClippingPlane=20cm` の三段構え。

### 3.3 2P の操作（レンズ拭き）

| 操作 | 入力 | 効果 |
|---|---|---|
| タオル表示 | LeapMotion に手をかざす | 画面上に手の正規化座標でタオル UI が出る |
| 拭き取り | 手を素早く動かす | `HandSpeed >= MinSpeedToWipe` で拭き取り発動 |
| タオル消耗 | 拭き取り中ずっと | `CurrentDurability` が `DurabilityDrainRate` で減少 |
| タオル交換 | 耐久 0 で自動 | `SwapDuration` 秒の交換時間中は拭けない（SE：TowelBreakSound / TowelReadySound） |

LeapMotion からの入力は **Blueprint で取得 → C++ 側 `ATomatinaTowelSystem::UpdateHandData(bool, FVector2D ScreenPosition, float Speed)` に毎フレーム流し込む**設計。
これにより LeapMotion の差分・別ハードへの差し替えが BP 側だけで済むようにしてある。

### 3.4 スコアリング

- **基本点**：被写体の中央度・サイズ・お題タグ一致（`ATomatinaTargetBase::MyType`）で算出
- **減点**：撮影時に写り込んだ汚れ（`FDirtSplat`）１個あたり `DirtPenaltyPerSplat=-5`
- **スタイリッシュゲージ**：
  - 高得点撮影で `+(ShotScore × StylishScoreToGaugeScale)`
  - 連続高得点で `+StylishTempoBonusGauge`（`StylishTempoWindow=4 秒`以内の連発）
  - 失敗で `-StylishMissPenalty`、汚れが画面の `DirtCriticalThreshold=92%` を覆うと毎秒 `-32` の大減衰
  - C / B / A / S / SSS の 5 段階。S 以上は `OnHighStylishShot` BP イベント発火で特殊演出
- **総合点（最終リザルト）**：
  - 平均点ベースのランク（S/A/B/C）
  - 平均スタイリッシュランク（撮影ごとに記録した RankSum / SampleCount）
  - **1P-2P シンクロ率**：1P 撮影時刻から `SyncWindowSeconds=2.0` 秒以内に 2P が拭き活動していれば「同期」とカウント、`SyncPhotoCount / TotalPhotoAttempts` で％算出

### 3.5 ファンファーレ・SE 設計

- 撮影直後の `FFanfareTier` 配列：MinScore 降順で最初に一致したものを再生（高得点ほど派手な SE）
- カウントダウン、ミッション開始、タイムアップ、リザルト溜め開始、結果発表、ズーム In/Out、タオル破損／交換完了、それぞれ独立した `FTomatinaSoundCue` で BP から差し替え可能
- BGM と観客ざわめき（CrowdAmbient）は別 AudioComponent で管理し、リザルト溜めで「BGM 以外停止」を `StopAllSoundsExceptBGM()` で実装
>>>>>>> 3d651346c6e35aacce63e4ee5f2b056db33db6b0

---

## 4. ハードウェア構成

<<<<<<< HEAD
### 4.1 物理配置

```
              ┌──────────────────────────┐
              │   メインモニター 2560×1600 │ ← 1P が見る、2P が手をかざす
              │   (UE のメインウィンドウ)   │
              └─────────┬────────────────┘
                        │
   ┌────────────────┐   │   ┌──────────────────┐
   │ スマホ (縦長)    │ ← │ → │  LeapMotion       │
   │ 拡張ディスプレイ │       │  メイン手前下方     │
   │ RT_Zoom 表示     │       │  2P 用ハンド入力    │
   └────────────────┘       └──────────────────┘
        ↑
     1P が覗き込んで構図確認
```

### 4.2 デュアルウィンドウ採用と nDisplay 不採用

**採用**: `FSlateApplication::Get().AddWindow(...)` で第二 `SWindow` を動的生成。中身は `SImage` + `RT_Zoom` 用 DMI。

**不採用**: `nDisplay`。理由は以下のとおり「目的が合わない」から。

| 観点 | nDisplay | 本作 |
|------|----------|------|
| 想定 | LED ウォール / CAVE で **同一シーンを多面で連結** | スマホには **別カメラ視点（ズーム）** を出したい |
| カメラ | 全ディスプレイが共通カメラ | メイン=三人称、スマホ=`SceneCaptureComponent2D` の RT |
| 設定 | コンフィグ / Switchboard / Cluster | 「USB 挿して `.exe` 起動」したい |
| 学習コスト | 高 | 低 |

詳しい技術実装は `HARDWARE_CHALLENGES.md` を参照。
=======
### 4.1 物理セットアップ

```
   ┌──────────────────────┐         ┌──────────────┐
   │   メインモニター      │         │ スマホ画面     │
   │   2560×1600           │  ←→    │ 2024×1152     │
   │   通常HUD・三人称ビュー│         │ ズームファインダー│
   │   お題・タイマー・スコア│         │ RT_Zoom 出力  │
   └──────────────────────┘         └──────────────┘
            ▲                                ▲
            │ HDMI                           │ HDMI(extended desktop)
            │                                │
       ┌────┴───────────────────────────────┴────┐
       │             Windows PC                   │
       │       UE 5.7 ランタイム                 │
       └────────┬─────────────────┬──────────────┘
                │                 │
        ┌──────┴───────┐   ┌─────┴────────┐
        │ 1P：マウス／KB │   │ 2P：LeapMotion │
        └────────────────┘   └────────────────┘
```

- **Windows の拡張デスクトップ**でスマホを２枚目のディスプレイ（or HDMI キャプチャされた iPhone）として認識
- Tomato.exe が起動時に `SWindow` を２枚生成：メインウィンドウ（モニター位置）＋スマホウィンドウ（`PhoneWindowPositionOverride` 座標）
- LeapMotion は USB 接続。`UltraleapTracking` プラグインが `LeapC.dll` をロードして手の３次元位置をストリーミング

### 4.2 デュアルウィンドウ実装の要点

通常 UE で「２画面に違う絵を出す」というと nDisplay が候補に上がるが、本作では使わなかった。理由：

| | nDisplay | 本作のデュアルウィンドウ |
|---|---|---|
| 描画内容 | 同一シーンを多面分担 | **メインとスマホで全く別のレンダーターゲット** |
| 要件複雑度 | クラスタ設定 / Launcher / 同期ノード | `SWindow` を C++ で `AddWindow` するだけ |
| 設定変数 | 専用 .uasset エディタ | `MainWidth/Height`, `PhoneWidth/Height`, `PhoneWindowPositionOverride` の３ペア |
| 用途 | LED ウォール、CAVE、シミュ | 学内展示・卒制・小規模イベント |

**Tomatina の要件は「メインモニターには三人称ビュー、スマホには SceneCapture から RT_Zoom した別画像」という"別シーン２面出力"**であって、
nDisplay の想定する "同一シーン多面分担" とは方向性が違う。よってシンプルな SWindow 動的生成で十分要件を満たし、
依存ライブラリ・配布サイズ・保守コストを最小化できた。詳細は `HARDWARE_CHALLENGES.md` 参照。
>>>>>>> 3d651346c6e35aacce63e4ee5f2b056db33db6b0

---

## 5. 技術スタック

<<<<<<< HEAD
### 5.1 エンジン・モジュール

- **Unreal Engine 5.7**
- ランタイムモジュール: `Tomato`（`Source/Tomato/Tomato.Build.cs`）
- 依存: `Engine`, `UMG`, `Slate`, `SlateCore`, `EnhancedInput`
- プラグイン: `UltraleapTracking`, `OpenXR`, `ModelingToolsEditorMode`, `GameplayStateTree`

### 5.2 主要 C++ クラス

| クラス | ファイル | 役割 |
|--------|----------|------|
| `ATomatinaGameMode` | `TomatinaGameMode.h/.cpp` | ゲームステートマシン、ミッション配列、ランク / Sync Rate |
| `ATomatinaPlayerPawn` | `TomatinaPlayerPawn.h/.cpp` | 1P 操作、ズーム、SceneCapture 強制、デュアルウィンドウ確認 |
| `ATomatinaHUD` | `TomatinaHUD.h/.cpp` | HUD 全般、第二 SWindow 生成、DPI 補正、DMI 経由 RT 表示 |
| `UTomatinaTowelSystem` | `TomatinaTowelSystem.h/.cpp` | LeapMotion 入力受付、拭き取り API 呼び出し |
| `ATomatoDirtManager` | `TomatoDirtManager.h/.cpp` | 汚れデータ（`FDirtSplat`）管理、Add/Wipe/Clear |
| `ATomatinaProjectile` | `TomatinaProjectile.h/.cpp` | トマト発射体、着弾時に `AddDirt` トリガ |
| `FTomatinaSoundCue` | `TomatinaSoundCue.h` | サウンドキューの薄いラッパ |

### 5.3 採用しているパターン・テクニック

- **デュアル `SWindow` 構成**: `FSlateApplication::AddWindow`
- **動的マテリアルインスタンス**: `UMaterialInstanceDynamic` で RT を貼り、毎フレームパラメータ再注入で Slate に再描画させる
- **DPI 補正**: `UWidgetLayoutLibrary::GetViewportScale()`
- **手動 `CaptureScene()`**: UE 5.7 の自動キャプチャタイミング不安定対策
- **`SetOverlayMaterial`** で全キャラに汚れマスクを乗せる（UE 5.0+）
- **`SetGlobalTimeDilation(0)` + `FApp::GetDeltaTime()`**: 世界停止中も UI を動かす
- **`SpawnActorDeferred` + `FinishSpawning`**: `BeginPlay` 前のプロパティ注入
- **エッジ検出 SE 制御**: 拭き取り音のループ防止
- **Enhanced Input** を採用（`UInputMappingContext` / `UInputAction`）

---

## 6. 解決した技術課題（ES 自由記述で使う Top 5）

> 詳細・コード例は `HARDWARE_CHALLENGES.md` 側、こちらは ES 文章作成のためのダイジェスト。

### Top 1: 旧シングルウィンドウ方式の破棄とデュアルウィンドウ移行

- **問題**: ウィンドウをモニター + スマホ幅でスパンさせ、Widget 内で 2 領域に分けていた。引き伸ばし、位置ズレ、設定項目爆発、展示先での 1 時間級調整。
- **判断**: **動いていた構造を捨て、`SWindow` を 2 つ出す構成に作り直す**。
- **結果**: 調整パラメータが「スマホ幅 1 つ」に集約。展示先での起動時間が 1 時間 → 5 分に。
- **学び**: 「動いているものを捨てる勇気」「保守コストが指数的に増える構造は早めに畳む」。

### Top 2: nDisplay を採用しない判断の言語化

- 候補に挙がったが「nDisplay は 1 シーン多面、本作は 2 シーン 2 面」と要件不一致を確認、**`AddWindow` の素朴解** を選択。
- **学び**: 公式機能をデフォルト採用するのではなく、**自分のユースケースを 1 行で書いてから候補と突き合わせる**。

### Top 3: UE 5.7 の SceneCapture 自動キャプチャ問題

- `bCaptureEveryFrame=true` でもサブウィンドウ構成下で更新が止まる現象。
- **対処**: `Tick` 内で `CaptureScene()` 明示呼び出し。
- **学び**: エンジンのバージョン依存挙動に当たったとき、**「自動を諦めて手動制御に倒す」決断速度**。

### Top 4: 第二 SWindow で RT が再評価されない問題

- Slate がサブウィンドウ側の Brush を「変化なし」と判定して描画しない。
- **対処**: `UMaterialInstanceDynamic` 経由で RT を貼り、毎フレームパラメータ再注入。Slate に「変化した」と判定させる。
- **学び**: 「**最適化を意図的に騙す**」ことが正解になるケースがある。可読性とコストのトレードオフを意識。

### Top 5: ズームカメラの壁抜け / 空ヒット

- Sphere Sweep + Safety Margin + 近接クリップで壁抜け対策。`SkyFallbackDistance` で空も撮影可。
- **学び**: ハード起因に見える不具合の多くは **古典的な数値設計（半径・マージン・フォールバック）で潰せる**。

---

## 7. 残った課題

### A. 〔最重要〕スマホ画面の視認性 UX

- **試遊会フィードバック**: 1P がスマホ画面を見ながらメインモニターで操作するのが想像以上に難しい。
- **設計時の意図**: スマホは望遠ファインダーとして注視される前提だった。
- **実態**: 視野の端にあり、構えながら首を動かすコストが高く、ほぼ無視される。
- **対策の方向**:
  1. 「常時注視」前提から「**決定的瞬間にだけ見る**」前提への情報設計組み替え
  2. メイン側に小さな PiP でズーム冗長表示（「見ても見なくてもプレイは成立」）
  3. 物理レイアウトの調整（角度・位置・距離）
  4. スマホを見るほど得する報酬設計（ベストショット判定など）
- **現時点の方針**: ハード再構成より **ゲームデザインで吸収** が現実解。

> **ES での書き方提案**: この課題は「失敗談」ではなく「**プレイヤーの実観察によって設計仮説が崩れた経験 → 次に何を変えるか言語化できる**」という枠で書くと強い。

### B. LeapMotion パッケージビルドの認識問題

- 診断ログ仕込み済み、`Saved/Logs/Tomato.log` の解析待ち。
- 仮の最終手段: `UltraleapTracking_ue5_4-5.0.1` バンドル DLL の手動コピー。

### C. スマホウィンドウの初期位置が手動

- `bOverridePhoneWindowPosition` + `PhoneWindowPositionOverride` で OS 座標を直接指定。
- 改善余地: 拡張ディスプレイ自動検出。優先度低。

### D. DPI キャッシュ初回 0

- `LayoutPhoneZoomImage` 内、`GetCachedGeometry()` が初回 0 を返すケースを 1 フレーム遅延でしのぎ中。実害なし。

---

## 8. 意思決定の見どころ（ES での問われ方別）

### 8.1 「直面した最大の困難は」と聞かれたら

→ **§6 Top 1（デュアルウィンドウ移行）+ §7 A（スマホ視認性）の組み合わせ**。
  ストーリー: 「設計を一度大きく作り直して負債を畳んだが、試遊会でゲームデザイン側の課題が露呈した」。技術的判断とプレイヤー観察の両輪が見える。

### 8.2 「チームで何をしたか」と聞かれたら

→ プロジェクト規模（個人 / 小チーム）を明記したうえで、**自分の担当範囲をはっきり書く**（要確認: 担当範囲）。コードベースは C++ 全般 + Blueprint 連携部分 + ハード系すべて自身で実装、と書ける状況なら明記。

### 8.3 「失敗から何を学んだか」と聞かれたら

→ **§7 A**。「設計時にプレイヤー視線の動線を机上だけで決めた → 実観察で覆った → 次プロジェクトではペーパープロトで設置動線を確かめる」。

### 8.4 「Cygames で何をやりたいか」と聞かれたら（要パーソナライズ）

- 提案フレーム:
  - 「ゲームを作るときに、**ハード制約 / プレイヤー観察 / コード設計** を並行で回せる人になりたい」
  - 「本作で『動いた』段階で満足せず、試遊会で出た UX 課題まで含めて改善し切れなかったので、**次は実観察を設計初期から組み込んだ開発** をやりたい」
  - 具体タイトル名はパーソナライズ要素として ES 用 Claude が補う

### 8.5 「強み / 弱み」

- 強み: **動いていた構造を捨てて作り直す決断ができる**（§6 Top 1）/ **公式機能の採用を要件とすり合わせて選別できる**（§6 Top 2）
- 弱み: **机上設計に寄りがち**（§7 A の反省）。試遊で初めて気づいた → 学習継続中。

---

## 9. ES 想定 Q&A サンプル

### Q1. このゲームを一言で説明して

> A. スペインのトマト祭りをモチーフに、1P がカメラマンとしてトマトを撃ち、2P が LeapMotion でメイン画面の汚れをタオルで拭き取る、メインモニター + スマートフォン + ハンドトラッキングを使った 2 人協力体験ゲームです。Unreal Engine 5.7 / C++ / Blueprint で実装しました。

### Q2. 一番工夫したところは？

> A. **メイン画面とスマホ画面に別々の絵を出すための描画構成** です。当初は 1 つのウィンドウをモニターとスマホにスパンさせ、Widget で 2 領域に分けていましたが、引き伸ばしバグや環境ごとの調整項目の爆発に悩まされました。`FSlateApplication::AddWindow` で第二 `SWindow` を素直に生成する構成に作り直すことで、調整項目を「スマホ幅 1 つ」に集約し、展示先での起動時間を 1 時間級から 5 分程度に短縮しました。nDisplay も検討しましたが「同一シーン多面」と「異シーン 2 面」で要件が違うため不採用にしました。

### Q3. 一番苦戦した技術的問題は？

> A. デュアルウィンドウに切り替えた直後、第二ウィンドウ側で `RT_Zoom` の絵が初期フレームで止まる問題に当たりました。Slate がサブウィンドウの Brush を「変化なし」と判定してテクスチャ参照を更新していなかったのが原因で、`UMaterialInstanceDynamic` 経由で RT を貼り、毎フレームパラメータを再注入することで「変化があった」と判定させて解決しました。同時期に UE 5.7 の `SceneCaptureComponent2D` の `bCaptureEveryFrame` が効かない瞬間にも当たり、`Tick` 内で `CaptureScene()` を明示呼び出しする実装に切り替えました。

### Q4. 残っている課題は？

> A. 試遊会で **「1P がスマホ画面を見ながら操作するのが難しい」** という UX 課題が浮上しました。設計時はスマホを望遠ファインダーとして注視される前提で組んでいましたが、実際にプレイしてもらうと視線がメインに固定され、スマホ側はほぼ無視されました。対策として、ハード再構成より **「常時見る」前提から「決定的瞬間にだけ見る」前提への情報設計組み替え** を優先する方針です。机上設計に寄っていた反省として、次プロジェクトでは設置動線をペーパープロト段階から確認する予定です。

### Q5. 学んだことは？

> A. 3 つあります。
> ① **設計を捨てる勇気**: 動いている構造でも、保守コストが環境ごとに増え続けるなら早めに畳むほうが結果的に安い。
> ② **公式機能の採用は目的との突き合わせから**: nDisplay という大きな選択肢があっても、要件 1 行で書き出して比較すれば不採用判断ができる。
> ③ **机上設計と実観察は別物**: 試遊会で初めて気づいた UX 課題があり、次は設計初期からプレイヤー動線を確かめたい。

---

## 10. 主要ファイル早見表

```
tamatoo/
├ Tomato.uproject              # UE 5.7 / プラグイン明示
├ Source/Tomato/
│   ├ Tomato.Build.cs          # 依存: Engine, UMG, Slate, SlateCore, EnhancedInput
│   ├ TomatinaGameMode.h/.cpp  # ステートマシン / ミッション / ランク / Sync Rate
│   ├ TomatinaPlayerPawn.h/.cpp # 1P 操作 / ズーム / SceneCapture 強制
│   ├ TomatinaHUD.h/.cpp        # HUD / 第二 SWindow / DPI 補正 / DMI RT
│   ├ TomatinaTowelSystem.h/.cpp # LeapMotion 入力 / 拭き取り API
│   ├ TomatoDirtManager.h/.cpp  # FDirtSplat / Add/Wipe/Clear / Sticky 系
│   ├ TomatinaProjectile.h/.cpp # トマト発射体 / 着弾で AddDirt
│   └ TomatinaSoundCue.h        # サウンドキューラッパ
├ Plugins/
│   └ UltraleapTracking_ue5_4-5.0.1/  # LeapMotion プラグイン
├ HARDWARE_CHALLENGES.md       # ハード壁の詳解（このドキュメントの技術裏付け）
├ ES_GAME_OVERVIEW.md          # 本ドキュメント
├ ZENN_CYGAMES_ES.md           # Zenn 記事ドラフト（公開向け）
├ TOMATINA_SETUP.md            # セットアップ手順
└ COPILOT_HANDOFF.md           # 旧 AI 引き継ぎ資料
=======
### 5.1 エンジン・言語

- **Unreal Engine 5.7**
- **C++17**（UE 標準）：コアロジック・状態管理・パフォーマンスクリティカル部
- **Blueprint Visual Scripting**：演出、Widget バインド、LeapMotion → C++ ブリッジ、デザイナー調整領域

### 5.2 主要モジュール／プラグイン

| 種類 | 名前 | 用途 |
|---|---|---|
| エンジン | Engine, Core, CoreUObject, InputCore | UE 標準 |
| UI | UMG, Slate, SlateCore | HUD / Widget / 第二 SWindow |
| 入力 | EnhancedInput | 1P 操作（IA_RightMouse, IA_LeftMouse, IA_Look） |
| プラグイン | **UltraleapTracking** | LeapMotion ハンドトラッキング |
| プラグイン | OpenXR | 一部の入力デバイス互換 |
| プラグイン | GameplayStateTree | （将来拡張用） |
| プラグイン | ModelingToolsEditorMode | エディタ内モデリング（Editor only） |

### 5.3 主要 C++ クラス

| クラス | 役割 |
|---|---|
| `ATomatinaGameMode` | ゲーム全体の状態機械（ロード → カウントダウン → ミッション → リザルト）、スコア集計、スタイリッシュランク管理、シンクロ率算出 |
| `ATomatinaPlayerPawn` | 1P カメラマン本体。`PlayerCamera` + `SceneCapture_Zoom` の二眼構成。Enhanced Input、ズームスイープ、ウィンドウレイアウト |
| `ATomatinaHUD` | 全 Widget の生成・破棄を一元管理。**第二 SWindow（スマホウィンドウ）の動的生成・破棄**。汚れ Widget の動的生成 |
| `ATomatoDirtManager` | 汚れデータ（`FDirtSplat`）の中央管理。`AddDirt`, `WipeDirtAt`, `ClearDirtAt`, `ClearAllDirts`。特殊トマト（連続ダッシュで剥がす）の状態管理 |
| `ATomatinaTowelSystem` | LeapMotion 入力を受けるアクター。耐久値・拭き音ループ・タオル交換タイマー |
| `ATomatinaProjectile` | 観客が投げるトマト。カメラ命中時に `AddDirt` 呼び出し |
| `ATomatinaTargetBase` | 撮影対象の基底クラス。`MyType`（FName）でお題タグ判定 |
| `ATomatinaCrowdManager` / `ATomatinaCrowdMember` | 観客 NPC の生成・配置・汚れオーバーレイ |
| `ATomatinaThrowerSpawner` / `ATomatinaThrower` | トマトを投げる NPC のスポーンと AI |
| `UTomatinaFunctionLibrary` | ユーティリティ関数群（`CalculateZoomOffset`, `ProjectZoomToMainScreen`, `PlayTomatinaCue2D` 等） |
| `FDirtSplat`（USTRUCT） | 汚れ１個分のデータ：正規化座標／不透明度／サイズ／タイプ／特殊状態 |
| `FMissionData`（USTRUCT） | お題定義：タグ／表示テキスト／制限時間／ターゲットクラス（または候補配列）／同時数 |
| `FFanfareTier` / `FTomatinaSoundCue` | スコア閾値別ファンファーレ／音 SE 構造体 |

### 5.4 採用したパターン・テクニック

- **`SetOverlayMaterial`（UE 5.0+）による汚れオーバーレイ**：キャラ／投擲者の元マテリアルに手を入れず、`UMaterialInstanceDynamic` で `DirtAmount` スカラパラメータを動的に書き換えるだけで全キャラ共通の汚れ表現を実現。Manager 側で一括設定 → 個別アクターはオプトアウト
- **`SpawnActorDeferred` + `FinishSpawning` パターン**：BeginPlay 前に `DirtOverlayMaterial` を Manager から子に注入
- **`SetGlobalTimeDilation(0)` + `FApp::GetDeltaTime()`**：「ロード中」「カウントダウン」「リザルト溜め」中はワールドを止めるが UI は実時間で動かす
- **エッジ検出によるループ SE 制御**：`UpdateWipeSound` で拭き状態の変化点だけで `SpawnSound2D` / `Stop`（毎フレーム呼ばない）
- **`UMaterialInstanceDynamic` 経由の RT 強制再評価**：第二 SWindow で Slate が RT を再サンプルしないバグへの実用的回避策
- **モノリシックパッケージング**：シェーダ／プラグイン込みで `Tomato.exe` を 297MB にビルド。配布が１ディレクトリで済む

---

## 6. 解決した主な技術課題（ハイライト 5 つ）

ES の「困難をどう乗り越えたか」エピソード用の素材。

### ① **デュアルディスプレイ表示の安定化**

- **問題**：当初は単一ウィンドウをモニター＋スマホ幅にスパンさせ Widget で位置合わせしていたが、環境ごとに DPI／解像度／Windows 配置を再調整する必要があり、画面引き伸ばし不具合も多発
- **対処**：`FSlateApplication::Get().AddWindow()` で **第二の `SWindow` を C++ から動的生成**するデュアルウィンドウ方式に切り替え
- **効果**：設定項目が解像度３ペア＋位置オーバーライド１個に集約。試遊会のたびの再調整時間が**数十分 → 数十秒**に短縮
- **判断**：nDisplay は「同一シーン多面分担」用で本作の「別シーン２面出力」要件と合わないため採用せず。**枯れた API（SWindow + UMG）の組み合わせで十分**と判断
- **アピールポイント**：「巨大プラグインを使わず、エンジン提供の最小構成 API で要件を満たした技術判断」

### ② **LeapMotion 連携と耐パッケージビルド**

- **問題**：エディタ実行（PIE）ではハンドトラッキングで汚れが拭けるが、パッケージビルドでは動作しない
- **対処**：
  1. `Tomato.uproject` に `UltraleapTracking` プラグインを**明示的に列挙**（自動検出に任せていた箇所）
  2. `[TowelDiag]` プレフィックスの**Warning レベル診断ログ**を BeginPlay／UpdateHandData／状態遷移／実拭き量の４箇所に注入
  3. ログから真因を切り分けるためのデシジョンテーブルを設計（プラグイン未配布／BP 呼び出し漏れ／単位ミスマッチ／Manager 未配置）
- **アピールポイント**：「症状から原因仮説を分岐させ、最小ログで真因を特定する診断設計」

### ③ **UE 5.7 における SceneCapture 自動キャプチャ不具合への対処**

- **問題**：`bCaptureEveryFrame=true` でも SceneCapture が更新されず RT_Zoom が黒画面
- **対処**：Tick で毎フレーム `CaptureScene()` を明示呼び出し。さらに BeginPlay で `CaptureSource` が `SCS_SceneColorHDR`（UMG 上で真っ黒になりがち）のままなら `SCS_FinalColorLDR` に自動補正
- **アピールポイント**：「エンジンバグや前提が崩れたケースで、ドキュメントに頼らず実機検証から回避策を設計した経験」

### ④ **第二 SWindow でのリアルタイム RenderTarget 描画**

- **問題**：第二 SWindow に直接 `UTextureRenderTarget2D` を `SetBrush` すると初回フレームで描画が固まる（Slate が再サンプルしない既知系挙動）
- **対処**：RT を直接貼らず、**`UMaterialInstanceDynamic` 経由でサンプル**。さらに毎フレーム `SetTextureParameterValue` を再注入してマテリアルを invalidate。マテリアル側パラメータ名のブレに対しても候補名総当たりでロバスト化
- **アピールポイント**：「Slate / UMG / RHI レイヤを跨いだ問題を、安定描画パイプラインに置き換えて解決した」

### ⑤ **物理ハード（LeapMotion）操作と論理スコアの統合**

- **問題**：LeapMotion から流れてくるハンド情報は粒度がフレーム依存で、そのまま「拭き量 × 速度」を加算するとフレームレート差で消費が変わる
- **対処**：`HandSpeed × WipeEfficiency × DeltaTime` で時間正規化、さらに `MinSpeedToWipe` 閾値で**意図的な動き**だけを拭き判定にする。耐久値は `DurabilityDrainRate × DeltaTime` で消耗
- **アピールポイント**：「外部ハードからのノイジーな入力をゲームメカニクスに馴染ませる正規化設計」

---

## 7. まだ残っている課題（正直に書ける部分）

ES では「課題を認識し、次の打ち手も考えている」姿勢で書ける素材。

### A. **1P がスマホ画面を見ながら操作するのが物理的に難しい**【最大の UX 課題】

- 試遊会で「画面が２つあるのに気づかない」「スマホに視線を固定すると敵が見えなくなる」のフィードバックが複数
- 技術的にはデュアルウィンドウで安定描画できているが、**ゲーム体験としてまだ届いていない**
- 検討中の打ち手：物理配置（モニター下端に貼り付け）／メイン画面 PiP／ズーム時のメイン画面暗転／カウントダウン時の視線誘導演出
- ESで使える論点：「**技術完成度＝体験完成度ではない**ことを試遊から学んだ」「ハードウェア要件を変えずに体験を届ける UX 改善は別問題」

### B. LeapMotion パッケージ版動作確認が完全には完了していない（ログ取得待ち）

### C. Windows のディスプレイ配置を自動取得していないため、`PhoneWindowPositionOverride` を試遊会のたびに手動で当てる必要がある

### D. 高 DPI 環境で UMG の `GetCachedGeometry().GetLocalSize()` が初回 0 になる（フォールバックで凌いでいるが本来は 1F 待ってから走らせるべき）

---

## 8. 開発期間中の意思決定ハイライト

ES のエピソード／ガクチカ用に。

| 場面 | 採用 | 不採用 | 理由 |
|---|---|---|---|
| ２画面表示 | デュアル `SWindow` 動的生成 | nDisplay | 要件が「別シーン２面出力」で nDisplay の想定と不一致。依存軽量化を優先 |
| 汚れの全キャラ反映 | `SetOverlayMaterial` + DMI（１マテリアルで全員に効く） | 各キャラのマテリアルに手を入れる | 設定の手間・将来のキャラ追加コストを下げる |
| カウントダウンの実装 | `TimeDilation=0` + `FApp::GetDeltaTime()` | Tick で `bIsCountingDown` フラグ＆別タイマー | ワールド自体を止められるので物理／音／演出の整合性が崩れない |
| LeapMotion → C++ ブリッジ | BP で受けて `UpdateHandData` 呼び出し | 直接 LeapC を C++ から叩く | ハード差し替え時に C++ 再ビルド不要にしたい |
| 写真撮影 | `SceneCapture` + `RT_Photo` + 汚れ正規化座標重ね | スクリーンショット API | UMG 内に表示する都合上 RT 経由が必要。汚れも同じ正規化座標系で扱える |
| パッケージング | モノリシック（per-module DLL なし） | 動的リンク | ３００MB で済むなら配布シンプル優先 |

---

## 9. 用途別 Q&A サンプル（ES 自己 PR に流用可）

### Q. このゲームで一番こだわった点は？
A. **「自分の手で物理的にレンズを拭ける」というフィジカル操作感**。LeapMotion で取得した手の位置と速度を、フレームレート差を吸収しつつ意味のある「拭く」アクションに正規化することで、デバイスのノイズに引きずられないゲーム体験を作った。

### Q. 一番苦労したのは？
A. **２画面表示の安定化**。最初は単一ウィンドウのスパン方式で実装したが、環境ごとの DPI／解像度／Windows 配置に振り回され、画面引き伸ばし不具合が頻発した。最終的に C++ から第二の SWindow を動的生成する方式に切り替えることで、設定項目を解像度３ペアに集約し、配布のたびに発生していた数十分の再調整作業を数十秒で済むようにした。

### Q. 技術選定で工夫したことは？
A. UE には大規模マルチディスプレイ用の **nDisplay** プラグインがあるが、本作の「メインとスマホでまったく別のシーン（別 RenderTarget）を出力する」要件には**過剰**だと判断した。`FSlateApplication::Get().AddWindow()` で `SWindow` を１枚追加するだけで要件を満たせる以上、巨大プラグインを採用すれば配布サイズ・依存ライブラリ・保守コスト・学習コストすべてが膨れる。**「枯れた API の組み合わせで要件を満たす」のが最適解**だと結論した。

### Q. ゲーム開発を通して学んだことは？
A. **「技術が完成しても、体験が届くとは限らない」**ということ。デュアルウィンドウ表示は技術的に完成したが、試遊会では「画面が２つあることに気づかれない」というフィードバックが複数あった。技術課題と UX 課題は別物で、両輪で改善する必要があると実感した。

### Q. C++ で UE を触っていて良かったことは？
A. Blueprint だけでは届かないレイヤ（Slate `SWindow` の動的生成、`UMaterialInstanceDynamic` の毎フレーム再注入、フレーム単位の SE エッジ検出など）に手が届く。**ハード起因の不具合や、エンジンの想定外挙動に対しても、回避策を自分で実装できる**のが C++ を選んでいる最大の理由。

### Q. これから直したい点は？
A. **1P がスマホ画面に視線を移しづらい UX 問題**。技術的にはデュアルウィンドウで安定したが、プレイヤーが「画面が２つあること」に気づかないケースがある。物理配置の工夫（モニター下端に固定）／メイン画面への PiP 表示／カウントダウン時の視線誘導演出のいずれかで、近いうちに改善したい。

---

## 10. ファイル構成（参考）

```
tamatoo/
├── Tomato.uproject                   # プロジェクト定義 + プラグイン明示
├── Source/Tomato/
│   ├── Tomato.Build.cs               # モジュール依存（UMG, Slate, SlateCore, EnhancedInput）
│   ├── TomatinaGameMode.{h,cpp}      # 状態機械、スコア、スタイリッシュ、シンクロ率
│   ├── TomatinaPlayerPawn.{h,cpp}    # 1P カメラマン、ズームスイープ、ウィンドウレイアウト
│   ├── TomatinaHUD.{h,cpp}           # 全 Widget 管理、第二 SWindow 動的生成
│   ├── TomatinaTowelSystem.{h,cpp}   # LeapMotion 入力受け、耐久値、拭き音ループ
│   ├── TomatoDirtManager.{h,cpp}     # 汚れ中央管理（追加・拭き取り・特殊トマト）
│   ├── TomatinaCrowdManager/Member.{h,cpp}    # 観客 NPC
│   ├── TomatinaThrowerSpawner/Thrower.{h,cpp} # トマトを投げる NPC
│   ├── TomatinaProjectileSpawner/Projectile.{h,cpp} # トマト弾
│   ├── TomatinaTargetBase.{h,cpp} / TomatinaTargetSpawner.{h,cpp} # 撮影対象
│   └── TomatinaFunctionLibrary.{h,cpp} # ユーティリティ
├── Plugins/UltraleapTracking_ue5_4-5.0.1/  # LeapMotion プラグイン
├── Content/                           # アセット（BP, Widget, Material, Texture, Sound）
├── HARDWARE_CHALLENGES.md            # ハードウェア課題と対処詳細
└── ES_GAME_OVERVIEW.md               # 本ファイル
>>>>>>> 3d651346c6e35aacce63e4ee5f2b056db33db6b0
```

---

<<<<<<< HEAD
## 11. ES 用 Claude への申し送り（このドキュメントの使い方）

### 11.1 やってほしいこと

1. **ゲームの説明文を書くとき**: §0〜§4 を要約源にする。技術寄り過ぎないよう、§2.2「視線と手が交差する」のような体験ベースの言葉を優先する。
2. **「直面した困難 / 工夫」を書くとき**: §6 Top 1〜5 から 1〜2 個ピック。複数を平面に並べるより、1 つを深掘りするほうが読み応えが出る。
3. **「失敗から学んだこと」を書くとき**: §7 A（スマホ視認性 UX）を必ず使う。これが本作で最も「正直に書ける」コンテンツ。
4. **長さの目安**: ES 1 項目 200〜400 字 想定。情報密度を上げ、固有名詞（`SWindow` / `FSlateApplication::AddWindow` / `UMaterialInstanceDynamic` / nDisplay / LeapMotion）を **適度に**残す（多すぎると読まれない）。

### 11.2 やってほしくないこと

- **「完璧な完成品」として書かない**: §7 の残課題に触れず「動いた」だけの ES は弱い。
- **技術用語を羅列するだけにしない**: 「なぜその選択をしたか」を 1 行は必ず添える。
- **チーム規模・担当範囲を勝手に断定しない**: 不明な場合は本人に「個人開発か、チーム開発か / 担当範囲はどこか」と確認する。
- **数値を盛らない**: 「展示先で 1 時間級 → 5 分」「Tomato.exe 約 297MB」「Sticky 連続ダッシュ 4 回」などは資料根拠あり、それ以外を勝手に作らない。

### 11.3 不明点（本人に質問してから書くべきこと）

| 項目 | 確認すべき内容 |
|------|---------------|
| プロジェクト規模 | 個人開発か、チーム開発か。チームなら何人 / 自分の担当範囲 |
| 開発期間 | 着手日〜現在（または完成日） |
| 試遊会の規模 | 何人にプレイしてもらったか / どこで実施したか |
| 想定プレイ時間 | 1 セッション何分か |
| 応募先 | Cygames インターン以外にも使うか（汎用 ES として書くか、企業特化で書くか） |
| 提出フォーマット | 文字数制限、Web フォームか PDF か |

### 11.4 ES 文章のスタイル指針

- 一人称: 「私」（ですます）か「自分」（である）かは、提出企業のフォーマットに合わせる。
- 文末: 同じ語尾を 3 回続けない（「〜ました。〜ました。〜ました。」を避ける）
- 構造: **「課題 → 判断 → 実装 → 結果 → 学び」** の 5 拍子で組むと刺さりやすい
- NG: 「頑張った」「成長した」など抽象語の単独使用。必ず行動・数値・固有名詞を添える

---

## 12. 補助資料

- 技術深掘りが必要なときは `HARDWARE_CHALLENGES.md` を参照。
- セットアップ手順が必要なときは `TOMATINA_SETUP.md`（要確認: 内容の最新性）。
- 公開向け Zenn ドラフトは `ZENN_CYGAMES_ES.md`（こちらは技術記事フォーマット、ES とは別軸）。

---

> **ES 用 Claude へ**: このドキュメントを読み終えたら、まず本人に「どの企業の何の項目（自由記述 / ガクチカ / 強み）に何字で書くのか」を確認してから本文生成に入ること。
=======
## 11. ES 用 Claude への注意事項

- **作者は個人開発者**。チーム規模を聞かれたら「主要設計・実装は個人、アセットは購入素材を一部使用」と回答可
- **想定読者は技術知識ありの人事／面接官**だが、UE / LeapMotion を知らない相手にも噛み砕いて説明できるよう、まずコンセプト → ハード構成 → 技術課題の順で書くこと
- **「nDisplay を使わなかった」**は技術判断のエピソードとして強い。「巨大ライブラリに頼らず最小構成で要件を満たした」という方向で展開可能
- **「スマホ画面を見てもらえない UX 問題」**は隠さない方が好印象。「技術完成 ≠ 体験完成」のリフレクションは ES 評価が高い
- ガクチカ／自己 PR では、本ファイル §6 の「解決した技術課題」５つから１〜２つを核に、§9 の Q&A をリライトする形で組み立てるのが最短
- 「２人協力プレイ」「自分の手で拭く」「祭りの賑やかさ」の三点はゲームの**情緒的な訴求ポイント**なので、技術エピソードと並べて伝えると人柄も出る
>>>>>>> 3d651346c6e35aacce63e4ee5f2b056db33db6b0
