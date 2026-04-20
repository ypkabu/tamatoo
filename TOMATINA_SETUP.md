# Tomatina — リファクタリング後セットアップ手順書

C++ 側の全面刷新が完了しました。以下の順で作業してください。

---

## 1. クリーンビルド手順（Live Coding は使わない）

1. **Unreal Editor を完全に閉じる**（タスクトレイも確認）。
2. **Visual Studio も閉じる**。
3. 以下のフォルダをエクスプローラから**削除**：
   - `Binaries/`
   - `Intermediate/`
   - `DerivedDataCache/`（任意。残してもよい）
   - `.vs/`（存在すれば）
4. `Tomato.uproject` を**右クリック → Generate Visual Studio project files**。
5. `Tomato.sln` を Visual Studio 2026 で開く。
6. ソリューション構成を **Development Editor / Win64** に。
7. `Tomato` プロジェクトを右クリック → **Build**（リビルドではなくビルドで OK）。
8. 成功したら `Tomato.uproject` をダブルクリックで起動。

> ⚠ ビルドエラーが出たら、まず `Saved/Logs/` の最新ログを確認してください。

---

## 2. Blueprint 側の設定（ここが一番重要）

### 2-1. BP_TomatinaPlayerPawn の Details

**Input カテゴリ**
| プロパティ | 値 |
|---|---|
| Default Mapping Context | **IMC_Default** |
| IA Right Mouse | **IA_RightMouse** |
| IA Left Mouse | **IA_LeftMouse** |
| IA Look | **IA_Look** |

**Screen カテゴリ（4 サイズ一元化の源泉）**
| プロパティ | 推奨値 |
|---|---|
| Main Width | `2560` |
| Main Height | `1600` |
| Phone Width | `1024` |
| Phone Height | `768` |

**Debug カテゴリ**
| プロパティ | 値 |
|---|---|
| b Test Mode | ☑ **true**（iPhone なしの開発時） |

**Zoom カテゴリ**
| プロパティ | 推奨値 |
|---|---|
| Default FOV | `90` |
| Zoom FOV | `30` |
| Zoom Speed | `5`（FInterpTo の速度） |
| Move Speed | `500`（ズーム中のマウスルック感度） |

**コンポーネント**
- `SceneCapture_Zoom` → Details → **Texture Target** = `RT_Zoom`

---

### 2-2. BP_TomatinaGameMode の Details

**Mission カテゴリ**
- `Missions` 配列を展開し、各要素に `FMissionData` を設定：
  - `Target Type` … 例: `"Gorilla"`（ターゲットの `MyType` と完全一致）
  - `Display Text` … 例: `"ゴリラを撮れ！"`
  - `Time Limit` … `15.0`
  - `Target Class` … `BP_Target_Gorilla` など
  - `Spawn Count` … `1`
  - `Spawn Profile Name` … `"Gorilla"`（TargetSpawner の SpawnProfiles と一致）
  - `Target Image` … プレビュー用 Texture2D
- `b Random Order` = **true**（ランダム順）

**Photo / Audio カテゴリ**
| プロパティ | 値 |
|---|---|
| RT Photo | **RT_Photo** |
| Shutter Sound | シャッター SE（SoundWave/SoundCue） |
| BGM | メイン BGM |

**Result カテゴリ**
| プロパティ | 推奨値 |
|---|---|
| Result Display Time | `3.0` |
| Mission Result Display Time | `2.0` |

**Score カテゴリ**
| プロパティ | 推奨値 |
|---|---|
| Dirt Penalty Per Splat | `-5`（汚れ 1 個あたりの減点。0 で減点しない） |

---

### 2-3. BP_TomatinaHUD の Details

**Widgets カテゴリ（すべて設定必須）**
| プロパティ | 値 |
|---|---|
| View Finder Widget Class | **WBP_ViewFinder** |
| Cursor Widget Class | **WBP_Cursor** |
| Dirt Overlay Widget Class | **WBP_DirtOverlay** |
| Photo Result Widget Class | **WBP_PhotoResult** |
| Mission Display Widget Class | **WBP_MissionDisplay** |
| Mission Result Widget Class | **WBP_MissionResult** |
| Final Result Widget Class | **WBP_FinalResult** |
| Shutter Flash Widget Class | **WBP_ShutterFlash** |
| Countdown Widget Class | **WBP_CountdownDisplay** |
| Test Pip Widget Class | **WBP_TestPip** |

**Material カテゴリ**
| プロパティ | 値 |
|---|---|
| Zoom Display Material | **M_ZoomDisplay** |
| Photo Display Material | **M_PhotoDisplay** |
| Dirt Texture | 汚れ用テクスチャ（PNG） |

**Photo カテゴリ**
| プロパティ | 推奨値 |
|---|---|
| Photo Display Width | `1024`（WBP_PhotoResult の SplatContainer 幅と一致させる） |
| Photo Display Height | `768`（同、高さ） |

> ⚠ `WBP_PhotoResult` には **IMG_Photo** とまったく同じ位置・サイズで重なる `UCanvasPanel` を追加し、名前を **SplatContainer** にしてください。撮影時の汚れはこの中に動的生成されます。`PhotoDisplayWidth/Height` はこの SplatContainer のサイズに合わせること。

---

### 2-4. World Settings

- 現在のマップを開いて **Window → World Settings**
- **Game Mode Override** = **BP_TomatinaGameMode**
- **HUD Class** = **BP_TomatinaHUD**（GameMode の HUD Class でも OK）
- **Default Pawn Class** = **BP_TomatinaPlayerPawn**

---

### 2-5. レベルに配置するアクター（Defolt_Game.umap）

必要なインスタンス数：

| アクター | 配置数 |
|---|---|
| `BP_TomatinaTargetSpawner` | 1 |
| `BP_TomatoDirtManager` | 1 |
| `BP_TomatinaTowelSystem` | 1 |
| `BP_TomatinaProjectileSpawner` | 1 |

**BP_TomatinaTargetSpawner** には `SpawnProfiles` 配列にミッション別のプロファイル（SpawnLocations 等）を必ず設定。

---

### 2-6. 各 BP_Target_* の Details

| プロパティ | 値 |
|---|---|
| My Type | `"Gorilla"` など（FMissionData.TargetType と一致） |
| Movement Type | `DepthHideAndSeek` / `RunAcross` / `FlyArc` / `FloatErratic` / `BlendWithCrowd` |
| Move Speed, HiddenY, ShownY 等 | 動作に応じて調整 |
| Skeletal Mesh のソケット | 可能なら `HeadSocket` / `RootSocket` を追加（無くても動作可） |

---

### 2-7. BP_TomatinaPlayerPawn 側で LeapMotion → 2P 手データを流す

- BP_TomatinaTowelSystem の `Event BeginPlay` で Ultraleap イベントにサブスクライブ。
- 毎フレーム `UpdateHandData(bDetected, HandPos01, HandSpeed)` を呼び出す。
- C++ 側は LeapMotion に一切依存していないため、BP の結線のみで完結します。

---

## 3. 期待されるログ（Output Log / [Warning] レベル）

PIE 開始直後から撮影 1 回までに、以下の順でログが流れれば全部正常です：

```
ATomatinaPlayerPawn::BeginPlay 開始 Main=(2560x1600) Phone=(1024x768) bTestMode=1
ATomatinaPlayerPawn: MappingContext 追加完了
ATomatinaPlayerPawn::SetupPlayerInputComponent
ATomatinaPlayerPawn: Input バインド R=OK L=OK Look=OK
ATomatinaHUD::BeginPlay 開始
ATomatinaHUD: サイズ取得 Main=(2560x1600) Phone=(1024x768) bTestMode=1
ATomatinaHUD::BeginPlay 完了
ATomatinaGameMode::BeginPlay 開始
ATomatinaGameMode: TargetSpawner 取得
ATomatinaGameMode: サイズ取得 Main=(2560x1600) Phone=(1024x768)
ATomatinaHUD::ShowCountdown 3
ATomatinaGameMode::BeginPlay: カウントダウン開始 Missions=N
ATomatinaHUD::ShowCountdown 2
ATomatinaHUD::ShowCountdown 1
ATomatinaHUD::HideCountdown
ATomatinaGameMode::StartMission Index=0
ATomatinaHUD::ShowMissionDisplay '○○を撮れ！'
```

右クリックで：
```
ATomatinaPlayerPawn::OnRightMousePressed
OnRightMousePressed: ズーム開始 Offset=(...)
ATomatinaHUD::ShowCursor
ATomatinaPlayerPawn::Tick: カーソルを iPhone 中央(3072,384)へ移動
ATomatinaPlayerPawn::Tick: ズーム完了（マウスルック有効）
```

左クリックで：
```
ATomatinaPlayerPawn::OnLeftMousePressed bIsZooming=1
ATomatinaGameMode::TakePhoto 開始
CopyZoomToPhoto 開始
CalculatePhotoScore 開始 Mission=Gorilla Targets=3
 Target=... Head=1 Root=1 Score=100
CalculatePhotoScore 結果: Score=100 BestTarget=...
TakePhoto: Score=100 Total=100
ATomatinaHUD::PlayShutterFlash
ATomatinaHUD::ShowResult Score=100
```

3 秒後：
```
ATomatinaHUD::HideResult
ATomatinaGameMode::StartMission Index=1
```

---

## 4. 変更した C++ ファイル一覧（全面書き直し）

| ファイル | 変更 |
|---|---|
| `Source/Tomato/TomatinaFunctionLibrary.h` | 全面書き直し（Head/Root 2 点採点仕様） |
| `Source/Tomato/TomatinaFunctionLibrary.cpp` | 全面書き直し |
| `Source/Tomato/TomatinaPlayerPawn.h` | 全面書き直し（CurrentLookInput 蓄積方式） |
| `Source/Tomato/TomatinaPlayerPawn.cpp` | 全面書き直し（Tick 内の未定義 EIC バグ除去） |
| `Source/Tomato/TomatinaGameMode.h` | 全面書き直し |
| `Source/Tomato/TomatinaGameMode.cpp` | 全面書き直し（MissionResult も FApp 実時間計測） |
| `Source/Tomato/TomatinaHUD.h` | 全面書き直し（位置・サイズ原則不触） |
| `Source/Tomato/TomatinaHUD.cpp` | 全面書き直し（Countdown/Flash 専用 Widget 分離） |

**保持（変更なし）**:
- `TomatinaTargetBase.*`
- `TomatinaTargetSpawner.*`
- `TomatinaProjectile.*`
- `TomatinaProjectileSpawner.*`
- `TomatoDirtManager.*`
- `TomatinaTowelSystem.*`
- `Tomato.Build.cs`（既に UMG/Slate/SlateCore/EnhancedInput を含む）

---

## 5. 仕様チェックリスト

- [x] Build.cs 依存：Core, CoreUObject, Engine, InputCore, UMG, Slate, SlateCore, EnhancedInput
- [x] C++ は Widget の位置・サイズを一切触らない（例外は UpdateCursorPosition と UpdateDirtDisplay のみ）
- [x] bTestMode=true で TestPip を自動生成
- [x] Enhanced Input 完全移行（古い GetInputAxisValue 等は不使用）
- [x] IA_Look は Triggered → CurrentLookInput 蓄積 → Tick で消費
- [x] Ultraleap は C++ から直接依存しない
- [x] CopyZoomToPhoto は TextureTarget 差し替え方式（RHI 非使用）
- [x] サイズ変数は BP_TomatinaPlayerPawn の 4 変数のみが真のソース
- [x] Time Dilation 0 中はすべて `FApp::GetDeltaTime()` を使う（カウントダウン・撮影結果・ミッション結果・シャッターフラッシュ）
- [x] GetWidgetFromName の要素名はすべて仕様書通り（TXT_Mission, IMG_TargetPreview, TXT_Timer, TXT_TotalScore, IMG_Crosshair, SplatContainer, PB_TowelStamina, TXT_SwapMessage, IMG_Photo, TXT_Score, TXT_Comment, TXT_FinalScore, TXT_Rank, TXT_Countdown, IMG_ZoomView）
- [x] 主要関数先頭に UE_LOG(Warning) & nullptr チェック

---

## 6. トラブル時のチェックポイント

| 症状 | 確認 |
|---|---|
| カウントダウンが出ない | HUD の `CountdownWidgetClass` に `WBP_CountdownDisplay` を設定したか |
| お題が表示されない | HUD の `MissionDisplayWidgetClass` と WBP 内の `TXT_Mission`/`IMG_TargetPreview` 要素名を確認 |
| ズームしてもカーソルが iPhone 側に行かない | Pawn の 4 サイズ変数が実際のウィンドウと一致しているか（MainWidth は 2560 など） |
| 撮影しても点数 0 | `FMissionData.TargetType` と `ATomatinaTargetBase.MyType` が大文字小文字含めて一致しているか |
| 撮影写真に汚れが重ならない | `WBP_PhotoResult` 内に `SplatContainer`（UCanvasPanel）を IMG_Photo と同サイズで重ねて配置したか／`PhotoDisplayWidth/Height` が合っているか |
| 汚れ減点が効かない | `BP_TomatinaGameMode.DirtPenaltyPerSplat` が 0 になっていないか／レベルに `BP_TomatoDirtManager` が配置されているか |
| iPhone 側に汚れが映らない | `WBP_DirtOverlay` の `SplatContainer` が画面全体（メイン + iPhone）をカバーする大きさか／HUD の `MainWidth/PhoneWidth` が正しいか |
| 撮影後も時間が止まったまま | `ResultDisplayTime`（3.0 推奨）、HUD が Tick する設定になっているか |
| タオル減点がされない | レベルに `BP_TomatinaTowelSystem` を 1 個配置しているか、`bTowelInZoomView` が BP 側で更新されているか |
