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

---

## 2. ゲームコンセプト

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

---

## 3. ゲームプレイ詳細

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
```

### 3.2 1P の操作（カメラマン）

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

---

## 4. ハードウェア構成

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

---

## 5. 技術スタック

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
```

---

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
