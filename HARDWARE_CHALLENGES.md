# ハードウェア実装で踏んだ壁と対処

Tomatina（La Tomatina モチーフの 2 人協力体験ゲーム）を Unreal Engine 5.7 で開発するうえで、**「画面 2 枚 + LeapMotion」というハード構成** に起因して踏んだ壁を時系列・原因別に整理する。コードの該当箇所もあわせて示す。

> 本ドキュメントは「**ゲームの仕様（=デュアルディスプレイ + LeapMotion 操作）を維持したまま、どうやってハード周りの不安定さを潰したか**」の記録。ゲームロジック側の話（スコア、ランク、ミッション制）はここでは扱わない。

---

## 0. 全体像 ― このゲームのハード仕様

```
┌──────────────────────────┐
│   メインモニター 2560×1600  │  1P 視界 / 2P が手をかざす対象
│   (UE のメインウィンドウ)   │
└─────────────┬────────────┘
              │
   ┌──────────┴──────────┐         ┌────────────────┐
   │ スマートフォン（縦長）  │         │   LeapMotion    │
   │ 拡張ディスプレイとして  │         │  メイン手前下方   │
   │ 接続。RT_Zoom を表示    │         │  2P 用ハンド入力  │
   │ (第二 SWindow)         │         └────────────────┘
   └────────────────────────┘
```

ハード構成として要求されること:

- **2 つの画面に「別の絵」が同時に出る**（メイン = 三人称、スマホ = 望遠ズーム）
- **2P の手の位置が、メイン画面上の正規化座標として汚れに作用** する
- 環境（モニター・スマホ機種・OS のディスプレイ配置）が変わっても、**設定変更が最小限** で展示できる

この 3 つを満たすことが、すべての課題の前提条件になっている。

---

## 1. 〔最大の壁〕「2 つの画面に別の絵」をどう実装するか

### 1.1 旧方式 ― シングルウィンドウ + Widget でスマホ枠を内包

最初の実装は **OS から見たウィンドウは 1 つだけ** で、その内部に Widget でスマホ枠の絵を描き、その上に `RT_Zoom` を貼り付ける方式だった。

具体的には:

- ゲームウィンドウサイズを **「メインモニター幅 + スマホ幅」** に設定（例: `2560 + 2024 = 4584px`）。スマホ実機を物理的に右に並べて、ウィンドウをモニター 2 枚にスパンさせる。
- Widget 上で「左半分 = メインビュー」「右半分 = スマホビュー」を描画。
- スマホ実機の物理位置と Widget 内のスマホ枠が重なるように、X オフセットや幅を環境ごとに微調整。

### 1.2 旧方式で発生した不具合

| 不具合 | 原因 | 試遊会・展示への影響 |
|--------|------|-----------------------|
| **メイン画面が引き伸ばされる** | ウィンドウ幅が「メイン+スマホ」になっているため、UE 側のレンダリングがアスペクト比を歪めて伸ばすケースが多発 | 被写体の見え方が変わり、撮影競技として成立しない |
| **スマホ枠の位置ズレ** | Windows 側のディスプレイ配置（左/右、上下オフセット）に依存して、Widget 内の枠位置を毎回手調整 | 展示先で 30〜60 分の調整時間が発生 |
| **設定項目の爆発** | スマホ実機解像度・モニター解像度・スケーリング・物理位置オフセット・ウィンドウ生成位置… と環境ごとに 5〜6 個調整 | 担当者以外は事故なく起動できない |
| **DPI スケールの非対称** | モニターとスマホで DPI 設定が異なると、Widget 内の同じ正規化座標が物理的にズレる | クロスヘア位置と汚れ位置が一致しない |

「**画面 1 枚分の動作確認をやり直すたびに 1 時間溶ける**」状態だった。**ゲームを別環境に持っていくことが事実上ボトルネック**になっていたため、構造ごと作り直す判断をした。

### 1.3 新方式 ― デュアルウィンドウ（`SWindow` を 2 つ動的生成）

採用した構成:

- メインウィンドウ: **UE 標準のゲームウィンドウ**（`MainWidth × MainHeight`）。三人称視点をそのままレンダリング。
- サブウィンドウ（スマホ画面）: **C++ から `SWindow` を生成し、`FSlateApplication::Get().AddWindow(...)` で OS にもう一つウィンドウを開く**。中身は `SImage` で、`RT_Zoom` を `UMaterialInstanceDynamic` 経由で表示しているだけ。

スイッチは `ATomatinaPlayerPawn::bUseSeparatePhoneWindow` および `ATomatinaHUD::bUseSeparatePhoneWindow`（既定 `true`）。新方式での生成は `ATomatinaHUD::CreatePhoneWindow()` で実行している。

```cpp
// ATomatinaHUD::CreatePhoneWindow（要点）
TSharedRef<SWindow> NewPhoneWindow = SNew(SWindow)
    .Title(FText::FromString(TEXT("Tomatina Phone")))
    .ClientSize(FVector2D(PhoneWidth, PhoneHeight))
    .ScreenPosition(PhoneScreenPos)        // bOverridePhoneWindowPosition で上書き可
    .SupportsMaximize(false)
    .SupportsMinimize(false)
    .UseOSWindowBorder(false)
    .SizingRule(ESizingRule::FixedSize);

NewPhoneWindow->SetContent(
    SNew(SImage).Image(&PhoneZoomBrush)    // PhoneZoomBrush は DMI を参照
);

FSlateApplication::Get().AddWindow(NewPhoneWindow, /*bShowImmediately=*/true);
PhoneWindow = NewPhoneWindow;
```

### 1.4 新方式で得た効果

- **メイン描画が一切影響を受けない**: スパンウィンドウではないので、メインモニター解像度に基づいたピクセル等倍描画。引き伸ばしバグが消えた。
- **設定項目が「`PhoneWidth` ひとつ」に集約**: 解像度を 1 個変えれば対応完了。残るのは `bOverridePhoneWindowPosition` でデスクトップ座標を上書きする任意項目だけ。
- **位置合わせが OS 任せ**: Windows の「設定 → ディスプレイ」でスマホ実機の配置を決めれば、サブウィンドウをそこにドラッグするだけで終わる。Widget 内座標の手調整は不要。
- **展示先での起動時間が大幅短縮**: 1 時間級の調整が、解像度確認 → 起動の 5 分程度に圧縮された。

### 1.5 なぜ nDisplay にしなかったか

UE には複数ディスプレイにシーンを分散描画する公式機能 `nDisplay` がある。検討したが、**目的が合わなかった** ため採用を見送った。

| 観点 | nDisplay の前提 | 本作の要件 |
|------|------------------|-----------|
| 想定ユース | LED ウォール / CAVE / シミュレータなど、**1 つのシーンを複数面で繋げる** | スマホには **別カメラの絵（ズーム）** を出したい |
| カメラ構成 | 全ディスプレイが共通カメラの一部 | メイン = 三人称、スマホ = `USceneCaptureComponent2D` の RT。**別シーン** |
| 必要設定 | nDisplay コンフィグ、Switchboard、Cluster ノード設定 | 本作には全てオーバースペック |
| 持ち運び | 配置先ごとにコンフィグ調整 | 「USB 挿して `Tomato.exe` 起動」で完結したい |

要するに **「同じ世界の続きを 2 画面に分ける」のが nDisplay、「別カメラの絵を 2 個並べる」のが本作**。本作は後者なので、`FSlateApplication::AddWindow` で素直に SWindow を 2 個出すほうが、コード量・依存・運用すべての面で軽かった。

---

## 2. デュアルウィンドウ化で派生した壁

ウィンドウを分離した直後は「真っ黒なスマホ画面」「ズームが時々止まる」「実機解像度ではアイコン極小」など別系統の問題が連鎖した。順番に対処したものを並べる。

### 2.1 DPI スケーリングでスマホ側 UI のサイズが合わない

`SWindow` を分けると、Slate の DPI スケール計算が **メイン Viewport の値とサブ SWindow の値で噛み合わない** 瞬間が出る。`UWidgetLayoutLibrary::GetViewportScale()` を使って逆補正を掛け、`ATomatinaHUD::LayoutPhoneZoomImage()` で **ズーム表示 Image を実ピクセル値で強制レイアウト** することで安定させた。

- 関連: `TomatinaHUD.h` の宣言 `LayoutPhoneZoomImage` / `FindOrCreateZoomImage`
- 既知の小問題: 初回フレームで `GetCachedGeometry()` が 0 を返すことがあり、1 フレーム遅延でフォールバック中（残課題 D）。

### 2.2 UE 5.7 で `bCaptureEveryFrame` が効かない瞬間がある

`USceneCaptureComponent2D::bCaptureEveryFrame = true` でも、サブウィンドウを使う構成だと **キャプチャがスキップされてスマホ画面が止まる** ことがあった。バージョン依存のタイミング問題と判断し、**`Tick` 内で明示的に `CaptureScene()` を呼ぶ** 方式に切り替えた。

```cpp
// ATomatinaPlayerPawn::Tick 内（要点）
if (SceneCapture_Zoom && SceneCapture_Zoom->TextureTarget)
{
    SceneCapture_Zoom->CaptureScene();
}
```

副作用として描画コストは上がるが、**毎フレーム必ずスマホ側に最新フレームが出る** ことが体験上必須なので受け入れた。

### 2.3 第二 SWindow で `RT_Zoom` の Brush が再評価されない

`SImage` に Brush として渡したテクスチャが、**メインウィンドウ側では更新されるのにサブウィンドウ側だけ初期フレームで止まる** 現象が発生した。Slate がサブウィンドウの Brush を「変わっていない」と判定し、テクスチャ参照を更新していないのが原因。

回避策は **`UMaterialInstanceDynamic`（DMI）を介して RT を貼る**。DMI のパラメータを毎フレーム再注入することで Slate に「変化した」と判定させ、確実に再描画させる。

```cpp
// 毎フレーム、PhoneZoomDMI のテクスチャパラメータを再注入
PhoneZoomDMI->SetTextureParameterValue(TEXT("ZoomRT"), ZoomRT);
PhoneZoomBrush.SetResourceObject(PhoneZoomDMI);
```

- 関連: `ATomatinaHUD::PhoneZoomDMI` メンバ、`ConfigureZoomImageContent`
- 「Slate の最適化を意図的に騙す」実装。別解（毎フレーム新規 Brush）もあるが、コスト・可読性で DMI 方式に落ち着いた。

### 2.4 ズームカメラが壁を貫通する

ズーム時、被写体に寄ろうとするとカメラが建物を貫通して内側に入り、世界の裏側が見えてしまう問題があった。古典的だが堅実な 3 段で対処:

1. **Sphere Sweep** で被写体までの間に障害物がないか判定（`ZoomOffsetSweepRadius`、デフォルト 15cm）
2. ヒットしたら手前で止める **Safety Margin**（`ZoomOffsetSafetyMargin`、デフォルト 25cm）
3. それでも貫通気味になる場合のため `CustomNearClippingPlane` を狭める（`ZoomNearClippingPlane`、デフォルト 20cm）

これで「街の建物にめり込まずに最大ズーム」が安定するようになった。`ATomatinaPlayerPawn` の `Zoom*` プロパティ群で全て BP からチューニング可能にしてある。

### 2.5 空・無限遠ヒットでカメラが暴走する

ライントレースが何にもヒットしないケース（空、街の外）では距離が「無限」扱いになり、ズームターゲット位置の計算が崩れてカメラが吹っ飛ぶ。`SkyFallbackDistance`（デフォルト 30m）を持っておき、**ヒットしなかったら仮想的に N メートル先に当たったことにする** ロジックで安定化した。副作用として **「空を飛ぶ鳥」へのズーム撮影もそのまま成立する** ようになり、ゲーム的にも嬉しい結果になった。

### 2.6 LeapMotion がパッケージビルドで認識されない疑い

エディタ実行は通るが、`Tomato.exe` パッケージで稀に LeapMotion を認識しない事象を確認している。

- `Tomato.uproject` の `Plugins` に `UltraleapTracking` を明示列挙済み
- `TomatinaTowelSystem.cpp` 側に **デバイス接続有無 / 手検出の診断ログ** を仕込み済み
- 次工程: パッケージ後 `Saved/Logs/Tomato.log` の解析（残課題 B）

---

## 3. 関連する小さな技術的工夫（ハード文脈で要るもの）

ハード関連で「副次的に必要だった」実装。詳しくは ES 用ドキュメント側に書くが、ここでも触れておく。

- **`SetOverlayMaterial`（UE 5.0+）** で全キャラに汚れマスクを乗せる。トマト命中部位の見た目変化を、メイン・スマホ両方の RT に同時反映させるため。
- **`SetGlobalTimeDilation(0)` + `FApp::GetDeltaTime()`** でリザルト時に世界を止めつつ UI だけ動かす。スマホ側の RT が「止まった世界の絵」になっても破綻しない設計のため。
- **`SpawnActorDeferred` + `FinishSpawning`** で `BeginPlay` 前にプロパティを注入。ハード初期化（ウィンドウ作成、SceneCapture セットアップ）と汚れマネージャ初期化の **順序保証** に必要だった。
- **拭き取り SE のエッジ検出（`UpdateWipeSound`）**。LeapMotion の手位置を毎フレーム読むので、立ち上がり/立ち下がりのエッジを取らないと SE がループしっぱなしになる。

---

## 4. 残った課題

### A. 〔最重要〕スマホ画面が 1P から見にくい

> **設計時の意図**: 1P がスマホを「望遠カメラのファインダー」として覗き込み、構図を決めてから撃つ。
>
> **実態（試遊会フィードバック）**: 1P はメインモニターを注視し続けたままトマトを撃ち、**スマホ画面は視野の端にあるだけで、ほぼ見られない**。

**原因の整理**:

1. スマホが縦長で視野の端にあるため、注視するには首を大きく動かす必要がある
2. トマト発射の照準合わせ中、視線がメインに固定されがち
3. スマホ側を「あったら嬉しい補助情報」として無視してプレイされるケースが多い
4. ハード設計（位置・角度）と UI 設計（情報密度）の橋渡しが甘かった

**対策案（検討中、優先度順）**:

1. **情報設計の組み替え**: スマホ画面を「常時見る」前提から「**決定的瞬間にだけ見る**」前提に。例: シャッター直前にスマホを光らせて視線誘導する。
2. **メイン側の冗長表示**: メインモニターに小さくピクチャ・イン・ピクチャでズーム映像を補助表示。スマホは廃せないが「見ても見なくてもプレイは成立」させる。
3. **物理レイアウトの見直し**: スマホをモニターに近づける / 角度を起こす / 高さを上げる。
4. **報酬設計の修正**: スマホを見ることがスコア的に有利になる仕掛け（ベストショット判定、瞬間アイコンなど）。

> 結論として、**ハード構成を変えるより、ゲームデザイン側で吸収する 1〜2 番が現実解**と考えている。

### B. LeapMotion パッケージビルドの認識問題

- 現状: 診断ログ仕込み済み・再パッケージ + 接続テスト + `Saved/Logs/Tomato.log` 確認待ち。
- 次に確認するログキー: `UltraleapTracking` の初期化、デバイス接続、`UTomatinaTowelSystem` 側の手検出。
- 仮にパッケージで動かない場合の最終手段: `UltraleapTracking_ue5_4-5.0.1` のバンドル DLL を `Binaries/Win64` に手動コピー。

### C. スマホウィンドウの初期位置が手動

- `ATomatinaHUD::bOverridePhoneWindowPosition` + `PhoneWindowPositionOverride` で OS のデスクトップ座標を直接指定する仕組み。
- 既定値は `(MainWidth, 0)` を使う「メインの右隣に出す」想定だが、**OS 側のディスプレイ配置が異なる環境では明示指定が必要**。
- 改善余地: 拡張ディスプレイのうち縦長のものを自動検出して左上に配置する処理。優先度は低（手動指定 1 行で済むため）。

### D. DPI キャッシュの初回 0 問題

- `LayoutPhoneZoomImage` で `GetCachedGeometry()` が初回フレームに 0 を返すケースがあり、1 フレーム遅延でしのいでいる。
- 実害は出ていないが、コードの匂いは残っている。`Tick` ベースで再試行カウントを設けるかの判断は保留（実害が出るまでは触らない方針）。

---

## 5. 参照ファイル早見表

| 役割 | ファイル | 関連シンボル |
|------|----------|--------------|
| カメラマン Pawn・ズーム計算・SceneCapture 強制 | `Source/Tomato/TomatinaPlayerPawn.h/.cpp` | `bUseSeparatePhoneWindow`, `EnsureDualScreenWindowLayout`, `ZoomOffset*`, `SkyFallbackDistance`, Tick 内 `CaptureScene` |
| HUD・スマホ用 SWindow 生成・DPI 補正・DMI 経由 RT | `Source/Tomato/TomatinaHUD.h/.cpp` | `CreatePhoneWindow`, `DestroyPhoneWindow`, `LayoutPhoneZoomImage`, `ConfigureZoomImageContent`, `PhoneZoomDMI`, `bOverridePhoneWindowPosition` |
| LeapMotion 入力受付・拭き取り API 呼び出し | `Source/Tomato/TomatinaTowelSystem.h/.cpp` | 拭き取り判定、診断ログ |
| 汚れデータ管理 | `Source/Tomato/TomatoDirtManager.h/.cpp` | `FDirtSplat`, `AddDirt`, `WipeDirtAt`, `ClearDirtAt`, Sticky 系プロパティ |
| プロジェクト設定（プラグイン明示） | `Tomato.uproject` | `UltraleapTracking` |

---

## 6. ひとことまとめ

このゲームのハード周りで一番効いた判断は **「最初の Widget スパン方式を捨てた」** こと。
動いていたものを捨てるのは怖いが、**「設定項目が環境ごとに増え続ける構造」自体が技術的負債** だと割り切り、`SWindow` を 2 個出すだけのシンプルな構成に作り直した。結果、調整パラメータは「スマホ幅 1 つ」に集約され、その後の派生バグ（DPI / SceneCapture / Brush 再評価 / 壁抜け / 空ヒット）は **個別に 1 件ずつ閉じられる** 状態になった。

> **設計を捨てる勇気と、捨てた後で残るバグを 1 件ずつ閉じきる根気** が、このゲーム制作で得た一番の経験。
