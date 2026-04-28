# ハードウェア実装の壁と対処まとめ

本ドキュメントは Tomatina プロジェクト（UE 5.7 / C++ + BP）開発中に発生した
**ハードウェア（複数ディスプレイ・LeapMotion・カメラ／RenderTarget まわり）**
に起因する技術的な壁と、その対処、残った課題を整理したものである。

ゲーム仕様の前提：
- 1P：カメラマン役（モニター＋キーボード／マウスで操作）
- 2P：「タオル拭き」役（**LeapMotion** で手を検出し、画面の汚れを拭き取る）
- 1P が見る画面は **メインモニター（2560×1600）と スマホ画面（2024×1152）** の２画面構成
  - メインモニター：通常の三人称ビュー＋お題＋スコア＋ゲームタイマー
  - スマホ画面：1P が右クリックでズームしたときの「ファインダー越しの被写体」（RT_Zoom）

---

## 1. スマホ画面の表示方式（旧スパンウィンドウ方式 → デュアルウィンドウ方式へ移行）

### 旧方式：単一ウィンドウをモニター＋スマホ幅に「スパン」させ、Widget で位置合わせ

最初の実装は以下のようにしていた：

- ゲームウィンドウを `MainWidth + PhoneWidth` 幅（例：2560 + 2024 = 4584）の１枚として開く
- メイン領域（左側 0〜2560）に通常 HUD、右側（2560〜4584）に `IMG_ZoomView` の UMG Widget を置きスマホの位置と物理的に合わせる
- スマホ側 UI（カーソル、汚れ、ファインダー枠）も同じ Widget ツリーに乗せ、CanvasPanelSlot の Position を C++ で操作

### 発生した問題

| 問題 | 内容 |
|---|---|
| 環境依存設定が爆発 | DPI スケーリング、Windows の拡張ディスプレイ配置（Y 揃え／X オフセット）、フルスクリーン／ウィンドウ／ボーダーレス、`r.SetRes`、`UGameUserSettings::SetScreenResolution` ……どれかが噛み合わないと描画ズレ。新しいマシンで動かすたびに数項目を再調整する必要があった |
| 画面引き伸ばし不具合 | UE のビューポートが `MainWidth + PhoneWidth` を 16:9 に正規化しようとして縦が引き伸ばされる／逆に縦に合わせて横が縮む。UMG の DPI Scale Curve を変えるたびに別の場所が壊れる |
| カーソルがスマホ領域に抜ける | 単一ウィンドウなので、ズームしていない状態でもメイン → スマホ領域にマウスが移動できてしまい、毎フレーム Tick で `MainWidth-1` にクランプし続ける必要があった (現行コードにも `TomatinaPlayerPawn.cpp:267-278` に名残あり) |
| Slate のレイアウト時タイミング | BeginPlay 時点でウィンドウサイズが望み通りでないことが多く、`EnsureDualScreenWindowLayout` を 0.5 秒間隔 ×6 回リトライする実装になった |

### 対処：デュアルウィンドウ方式（現行）

`Source/Tomato/TomatinaHUD.cpp:CreatePhoneWindow()` で **第二の `SWindow` を C++ から動的生成**して
スマホ側 UI を独立ウィンドウに分離した。

実装の要点：

```cpp
TSharedRef<SWindow> NewWindow = SNew(SWindow)
    .Type(EWindowType::GameWindow)
    .CreateTitleBar(false)
    .UseOSWindowBorder(false)
    .FocusWhenFirstShown(false)
    .ActivationPolicy(EWindowActivationPolicy::Never)
    .ScreenPosition(ScreenPos)   // スマホの Windows デスクトップ座標
    .ClientSize(ClientSize);     // PhoneWidth × PhoneHeight

FSlateApplication::Get().AddWindow(NewWindow, true);
PhoneWindow->SetContent(PhoneViewWidget->TakeWidget());  // UMG をそのまま流し込む
```

- **メインウィンドウ**：純粋に `MainWidth × MainHeight` の単独ゲームウィンドウ。ビューポートのアスペクトが乱れない
- **スマホウィンドウ**：別の `SWindow` として OS レベルで分離。タイトルバー・枠なしフルスクリーン風
- **位置合わせ**：`PhoneWindowPositionOverride`（FVector2D）１変数で完結。Windows のディスプレイ配置に合わせて `(2560, 0)` などを入れるだけ。`bOverridePhoneWindowPosition` のフラグで自動 / 手動を切替

### 結果

- 解像度・配置に関わる **設定項目は実質「`MainWidth/Height`、`PhoneWidth/Height`、`PhoneWindowPositionOverride` の３ペア」だけ**になった
- 画面引き伸ばしは消滅（メインウィンドウのアスペクトが純粋になったため）
- カーソルがスマホ領域に抜ける問題は OS のウィンドウ境界で自然に解消（メインウィンドウ内で完結）
- 旧スパン方式のコードパスは `bUseSeparatePhoneWindow=false` のフォールバックとして温存（`TomatinaPlayerPawn.h:69-71`、`TomatinaHUD.cpp:78-92, 624-635`）

### なぜ nDisplay にしなかったのか

nDisplay は **Unreal の業務向けマルチディスプレイ同期出力プラグイン** で、LED ウォール／CAVE／フライトシミュ等の
「**１つのシーンを複数ディスプレイで分担して描画する**」ユースケースを想定している。
Tomatina の要件と合わない理由：

| 観点 | nDisplay | Tomatina の要件 |
|---|---|---|
| 出力内容 | 同一シーンをカメラオフセットで分担描画 | **メインとスマホでまったく別の絵**（メイン＝三人称ビュー、スマホ＝ズームファインダー＝SceneCapture の RT_Zoom）|
| 構成ファイル | `nDisplayConfig` (.uasset) を専用エディタで設計 | UPROPERTY 1〜2 個で済ませたい |
| 同期方式 | クラスタ用に専用ノードを立てて nDisplayLauncher 経由で起動 | 単一実行ファイル `Tomato.exe` で完結したい |
| プラグイン依存 | nDisplay + DisplayCluster 系一式 | 余計なランタイムを増やしたくない（UltraleapTracking と OpenXR で既に重い）|
| 用途規模 | プロ用・大規模イベント | 学内展示・卒制レベルの２画面 |

要するに **「同一シーンを多面出力」する nDisplay と、「別シーン（別 RenderTarget）を別ウィンドウに出す」Tomatina とは設計思想が別物**。
SWindow を１枚追加するだけの本実装の方が、保守コスト・配布コスト・依存ライブラリの観点で圧倒的に妥当だった。

---

## 2. DPI スケーリングで `IMG_ZoomView` がビューポート外へ飛ぶ

### 課題

縦 1600 のメインウィンドウで Windows DPI が ≒1.48 倍になっていると、`CanvasPanelSlot::SetPosition` に
ピクセル値（例：`MainWidth=2560`）を渡しても**実際にはその 1.48 倍の widget-space 座標で配置されてしまう**ため、
Zoom Image がビューポート外に飛び出して描画されない。

### 対処（`TomatinaHUD.cpp:LayoutPhoneZoomImage` 332-349）

```cpp
const float DPIScale = UWidgetLayoutLibrary::GetViewportScale(Widget);
const float Safe = (DPIScale > KINDA_SMALL_NUMBER) ? DPIScale : 1.f;
const float PhoneX = MainWidth / Safe;
const float SlotW  = PhoneWidth / Safe;
```

ピクセル値を `DPIScale` で割って widget-space に正規化してから渡すように修正。これは旧スパン方式の名残対策で、
現行のデュアルウィンドウでは別 `SWindow` 内なので問題自体が発生しないが、レガシー経路として残してある。

---

## 3. UE 5.7 で `SceneCapture2D` が `bCaptureEveryFrame=true` でも自動キャプチャしないことがある

### 課題

`SceneCapture_Zoom`（ズーム用カメラ）の `bCaptureEveryFrame` を true にしているのに、
RT_Zoom が更新されず一面真っ黒になる症状が UE 5.7 で再現した。BP 側で意図せず false になっているのではなく、
true のままでも自動キャプチャが走らないケースが観測された。

### 対処（`TomatinaPlayerPawn.cpp:Tick` 169-172）

```cpp
if (SceneCapture_Zoom->TextureTarget)
{
    SceneCapture_Zoom->CaptureScene();  // 毎フレーム明示呼び出し
}
```

毎フレーム明示的に `CaptureScene()` を叩く。二重キャプチャになっても 1 シーン分なので体感差はなし。
副次的に BeginPlay で `CaptureSource` が `SCS_SceneColorHDR`（UMG 上で真っ黒になりやすい）のままだと
`SCS_FinalColorLDR` に自動補正する保険も入れた（`TomatinaPlayerPawn.cpp:107-112`）。

---

## 4. 第二 SWindow で UImage の RT ブラシが再サンプルされない

### 課題

スマホウィンドウ（第二 `SWindow`）に直接 `UTextureRenderTarget2D` を `SetBrushFromTexture` 相当で割り当てると、
**初回フレームの内容で固まる**（または描画されない）。Slate がセカンダリウィンドウ向けに RT ブラシのリサンプルを
スキップする既知系の挙動と思われる。

### 対処（`TomatinaHUD.cpp:ConfigureZoomImageContent / Tick` 431-507, 567-598）

RT を直接 `UImage` に貼らず、**`UMaterialInstanceDynamic`（DMI）経由**でサンプリングする。
さらに毎フレーム RT パラメータを再注入してマテリアルを再評価させる：

```cpp
// 第二ウィンドウでは material 経由に強制
const bool bPreferMaterial = bUseSeparatePhoneWindow;
if (bPreferMaterial && ZoomDisplayMaterial)
{
    UMaterialInstanceDynamic* DMI = UMaterialInstanceDynamic::Create(ZoomDisplayMaterial, ImageWidget);
    DMI->SetTextureParameterValue(TEXT("RT_Zoom"), ZoomRT);  // 候補名総当たり
    ImageWidget->SetBrushFromMaterial(DMI);
    PhoneZoomDMI = DMI;
}

// HUD::Tick 毎フレーム
if (PhoneZoomDMI && ZoomRT)
{
    PhoneZoomDMI->SetTextureParameterValue(TEXT("RT_Zoom"), ZoomRT);  // 再注入で invalidate
}
```

マテリアル側のパラメータ名のブレ（`RT_Zoom` / `Texture` / `MainTex` / `Zoom` / `SceneTex`）に対しても
総当たりで `SetTextureParameterValue` を呼んでロバスト化している。

---

## 5. ズーム時カメラのめり込み（壁 / 床）

### 課題

右クリックで被写体方向にカメラオフセットを伸ばすズームを実装したが、被写体が壁際 / 床際だと
オフセット先の SceneCapture がジオメトリ内側に入り込んで描画が破綻する（黒画面、内側面描画）。

### 対処（`TomatinaPlayerPawn.cpp:OnRightMousePressed` 326-372）

1. **球形スイープでオフセット先を遮蔽チェック**：
   ```cpp
   GetWorld()->SweepSingleByChannel(
       SweepHit, StartLoc, EndLoc, FQuat::Identity,
       ECC_Visibility,
       FCollisionShape::MakeSphere(ZoomOffsetSweepRadius));  // 既定 15cm
   ```
2. **ヒットしたら安全距離マージン分手前にクランプ**：
   ```cpp
   const float SafeDist = FMath::Max(0.f, HitDist - ZoomOffsetSafetyMargin);  // 既定 25cm
   TargetOffset *= (SafeDist / FullDist);
   ```
3. **保険として `CustomNearClippingPlane` を広めに**（既定 20cm）設定し、回転でめり込んだケースでも内側面が描画されないようにする

---

## 6. 「空（虚空）」ヒット時にズームが起動しない

### 課題

ライントレースが空に伸びてヒットしないと、ズーム自体が走らず鳥などの遠距離オブジェクトが撮影できない。

### 対処（`TomatinaPlayerPawn.cpp:OnRightMousePressed` 311-321）

虚空ヒット時は `SkyFallbackDistance`（既定 3000cm）の位置に**仮想ヒット点**を置いてズームを継続する。

```cpp
if (!bHit)
{
    const float SkyDist = FMath::Max(100.f, SkyFallbackDistance);
    Hit.ImpactPoint  = WorldLoc + WorldDir * SkyDist;
    Hit.ImpactNormal = -WorldDir;
}
```

---

## 7. LeapMotion がパッケージ版で動かない（調査中／部分対処済）

### 課題

エディタ実行（PIE）では LeapMotion 経由で 2P が手をかざすと汚れが拭ける。
**パッケージビルド（Tomato.exe）では同じ操作で汚れが消えない**。

### これまでの調査と対処

| 確認項目 | 結果 |
|---|---|
| Manifest_NonUFSFiles_Win64.txt | LeapC.dll が出力に含まれている ✓ |
| Manifest_UFSFiles_Win64.txt | UltraleapTracking のコンテンツが pak に入っている ✓ |
| Tomato.exe サイズ | 297MB（モノリシック）= per-module DLL 不要 ✓ |
| プラグインの DLL ロードパス | `Plugin->GetBaseDir() + Binaries/Win64/LeapC.dll` でステージング先と一致 ✓ |
| `.uproject` のプラグイン明示 | **元々書かれていなかった** → `Tomato.uproject` に明示追加（後述）|

#### 対処１：`Tomato.uproject` に UltraleapTracking を明示

UE は `<Project>/Plugins/` 配下のプラグインを自動検出するが、**Cook 時の依存解析は `.uproject` の明示エントリの方が信頼性が高い**。

```json
"Plugins": [
    { "Name": "OpenXR",                "Enabled": true },
    { "Name": "ModelingToolsEditorMode","Enabled": true, "TargetAllowList": ["Editor"] },
    { "Name": "GameplayStateTree",      "Enabled": true },
    { "Name": "UltraleapTracking",      "Enabled": true }
]
```

#### 対処２：診断ログを `TomatinaTowelSystem.cpp` 全域に注入

パッケージ版でも残るよう **Warning レベル**で、1 秒間隔スロットル付きの `[TowelDiag]` ログを下記 4 箇所に追加：

1. `BeginPlay` 起動マーカー（このログが出ない＝レベルにアクター未配置）
2. `UpdateHandData` 受信ログ（出ない＝BP 側が C++ を呼んでいない＝LeapC.dll が動いていない）
3. `WipingState` ON/OFF 遷移ログ（HandSpeed と MinSpeedToWipe の単位ミスマッチ判別用）
4. `Wipe Amount` と `DirtManager NULL` 警告（DirtManager がレベルに配置されていない症状の特定）

### 残課題

- ログ取得待ち。`Saved/StagedBuilds/Windows/Tomato/Tomato/Saved/Logs/Tomato.log` の `[TowelDiag]` 出力パターンから真因を確定する
- 想定される真因の候補：
  1. プラグイン未明示で Cook がスキップしていた（対処１で解消見込み）
  2. BP 側が `UpdateHandData` を呼ぶグラフをパッケージで実行していない
  3. `MinSpeedToWipe` の単位ミスマッチ（PIE と Cooked で正規化基準が違う）
  4. `ATomatoDirtManager` がレベルに未配置

---

## 残った課題（未解決）

### A. **【最優先・未解決】1P がスマホ画面を見ながら操作するのが物理的に難しい**

これがまだ最大のハードウェア／UX 問題。**現状の設計では、1P はモニターを見つつスマホ画面（ズームファインダー）も同時に確認しながらキーボード／マウスで操作する**ことが要求されるが、

- スマホは机の上に水平 or 斜めに置く形になるため、視線移動が大きく疲れる
- ズーム中は被写体に集中しなければならないのにスマホを見ないと撮れない（メインモニターでは三人称ビューしか見えない）
- 実際にプレイしてもらうと「スマホ画面の存在を忘れる」「視線をスマホに固定すると敵トマトが見えなくなる」
- 試遊会で **「画面が２つあるのに気づかなかった」** というフィードバックが複数回

**技術的には完成済**（デュアルウィンドウで安定描画）だが、**ゲーム体験としてまだ届いていない**。検討中の打ち手：

| 案 | メリット | デメリット |
|---|---|---|
| スマホをモニター下端に「直接貼り付ける」物理配置にする | 視線移動が最小化 | 実機ハード固定が必要、設置時間がかかる |
| メインモニターにもズームビューを **PiP（小さなファインダー枠）** で重ねる | 物理スマホ無しでもプレイ可能 | 「スマホで撮る」というゲームのコア体験が薄れる |
| ズーム中だけメインモニターを暗転させスマホを目立たせる | 自然と視線が動く | 画面真っ暗になるのは違和感 |
| カウントダウン中・お題提示中にスマホ側で派手な演出を入れて「視線誘導」する | 体験を壊さず視線移動を学習させる | プレイ毎に効果は薄れる |

現在は決定打なし。**プレイヤー教育（チュートリアル）と物理配置の両輪で改善する方向**で検討中。

### B. LeapMotion パッケージ版動作問題（§7、ログ待ち）

### C. 第二ウィンドウの**マルチモニター環境ごとの位置調整が手作業**

`PhoneWindowPositionOverride` を BP 側で人間が入れる必要がある。Windows の API でディスプレイ列挙して
「メインモニターの右隣にあるサブディスプレイの左上座標」を自動取得する処理は未実装。
試遊会のたびに `(2560, 0)` / `(1920, 0)` / `(0, 1080)` のいずれかを当て込んでいる。

### D. 高 DPI 環境で UMG の `GetCachedGeometry().GetLocalSize()` が初回 0 になる

`TomatinaHUD::UpdateDirtDisplay` で初回呼び出し時はピクセルフォールバックする保険（`TomatinaHUD.cpp:1147-1150`）を入れているが、
本来はレイアウト確定 1 フレーム待ってから走らせる方が正しい。

---

## 参考：主要ファイル

| 機能 | ファイル | 主要行 |
|---|---|---|
| デュアルウィンドウ生成 | `Source/Tomato/TomatinaHUD.cpp` | `CreatePhoneWindow` 161-257 |
| 旧スパンウィンドウ（互換用） | `Source/Tomato/TomatinaPlayerPawn.cpp` | `EnsureDualScreenWindowLayout` 469-526 |
| ZoomImage の DPI 補正 | `Source/Tomato/TomatinaHUD.cpp` | `LayoutPhoneZoomImage` 301-350 |
| RT 強制再評価 | `Source/Tomato/TomatinaHUD.cpp` | `Tick` 567-598 |
| ズームスイープ | `Source/Tomato/TomatinaPlayerPawn.cpp` | `OnRightMousePressed` 326-372 |
| LeapMotion 受け口 | `Source/Tomato/TomatinaTowelSystem.cpp` | `UpdateHandData` 51-69 |
| プラグイン明示 | `Tomato.uproject` | `Plugins` 配列 |
