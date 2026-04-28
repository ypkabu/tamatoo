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

---

## 2. ゲームコンセプト

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

---

## 3. ゲームプレイ詳細

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
```

### 3.2 1P の操作（カメラマン）

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

---

## 4. ハードウェア構成

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

---

## 5. 技術スタック

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
```

---

## 11. ES 用 Claude への注意事項

- **作者は個人開発者**。チーム規模を聞かれたら「主要設計・実装は個人、アセットは購入素材を一部使用」と回答可
- **想定読者は技術知識ありの人事／面接官**だが、UE / LeapMotion を知らない相手にも噛み砕いて説明できるよう、まずコンセプト → ハード構成 → 技術課題の順で書くこと
- **「nDisplay を使わなかった」**は技術判断のエピソードとして強い。「巨大ライブラリに頼らず最小構成で要件を満たした」という方向で展開可能
- **「スマホ画面を見てもらえない UX 問題」**は隠さない方が好印象。「技術完成 ≠ 体験完成」のリフレクションは ES 評価が高い
- ガクチカ／自己 PR では、本ファイル §6 の「解決した技術課題」５つから１〜２つを核に、§9 の Q&A をリライトする形で組み立てるのが最短
- 「２人協力プレイ」「自分の手で拭く」「祭りの賑やかさ」の三点はゲームの**情緒的な訴求ポイント**なので、技術エピソードと並べて伝えると人柄も出る
