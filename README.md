# Wipe&Snap

スペインのトマト祭り「La Tomatina」をモチーフにした、2人協力プレイの体験型撮影ゲームです。  
1Pはカメラマンとして自作カメラ型コントローラーを構え、群衆の中からお題の被写体を探して撮影します。  
2PはLeap Motion Controllerで手を動かし、トマトで汚れたレンズを拭き取って1Pの撮影を支援します。

## プロジェクト概要

- **タイトル**: Wipe&Snap（リポジトリ名 `Wipe&Snap_UE5`）
- **ジャンル**: 2人協力 / 体験型 / ハードウェア連動
- **想定設置環境**: モニター + スマートフォン画面（デュアルディスプレイ）+ LeapMotion
- **プレイ人数**: 2 人（1P=カメラマン、2P=タオル拭き取り）
- **想定プレイ時間**: 3分ほど
- **用途**: 学内展示 / 試遊会 

## デモ

- プレイ動画: URL
- Zenn: URL
- スクリーンショット / GIF: 追加予定

## 自分の担当

私は主に、1P/2Pの操作体験に関わるクライアント実装を担当しました。

- スマホ表示用 `SWindow` の生成
- `SceneCapture2D` + `RenderTarget` によるズームファインダー
- Leap Motion Controllerの手速度を使った拭き取り判定
- ズームカメラの壁抜け対策
- `ProfileGPU` による描画負荷確認とトマト影設定の改善
- 展示環境での表示安定化

筐体制作、3Dモデル、ステージ制作、サウンド、ゲームルール検討は主に他メンバーが担当しました。

### 操作概要

| 役割 | 入力デバイス | 主な操作 |
|------|--------------|----------|
| 1P カメラマン | キーボード/コントローラ（Enhanced Input） | 視点移動・トマト発射・スマホ画面でズーム確認 |
| 2P 拭き取り | LeapMotion（Ultraleap Tracking） | 手のジェスチャでメイン画面の汚れを拭く |

## Unreal Engine のバージョン

- **UE 5.7**（`Tomato.uproject` の `EngineAssociation` で確認済み）
- ビルド構成: C++ + Blueprint
- パッケージング: Monolithic（`Tomato.exe` 約 297MB）

## 現在実装されている機能

### ゲームプレイ

- [x] 1P 
- [x] 2P LeapMotion 拭き取り（`UTomatinaTowelSystem`）
- [x] トマト発射（`ATomatinaProjectile`）と着弾位置への汚れ生成
- [x] 汚れ管理（`ATomatoDirtManager`）
  - 通常トマト（赤）: 円形範囲の Opacity 漸減
  - 特殊トマト（黄、Sticky）: 連続ダッシュ × 4 回で剥がれる
- [x] スタイリッシュランク（C / B / A / S / SSS）と Sync Rate 計測
- [x] ミッション制ゲーム進行（`ATomatinaGameMode` 内 Missions 配列）
- [x] 全汚れクリア演出 → 最終リザルトへの遷移

### ハードウェア / 描画

- [x] **デュアルウィンドウ表示**: `FSlateApplication::AddWindow` + 動的 `SWindow` 生成によりメイン画面とスマホ画面を別ウィンドウで描画
- [x] スマホ画面に `RT_Zoom` レンダーターゲットを表示（`ConfigureZoomImageContent`、DMI 経由）
- [x] DPI スケーリング補正（`UWidgetLayoutLibrary::GetViewportScale()`）
- [x] UE 5.7 SceneCapture 自動キャプチャ問題の回避（Tick 内で `CaptureScene()` 手動呼び出し）
- [x] ズームカメラの壁抜け対策（Sphere Sweep + Safety Margin + `CustomNearClippingPlane`）
- [x] 空・無限遠ヒット対策（`SkyFallbackDistance` で仮想ヒット点）
- [x] キャラクタ全体に汚れマスクを乗せる `SetOverlayMaterial`（UE 5.0+）

### UI / 演出

- [x] ポーズ可能な UI（`SetGlobalTimeDilation(0)` + `FApp::GetDeltaTime()`）
- [x] 拭き取り SE のループ・エッジ検出（`UpdateWipeSound`）
- [x] BeginPlay 前のプロパティ注入（`SpawnActorDeferred` + `FinishSpawning`）

## 開発中・改善予定の機能

現在、展示や試遊で得られた課題をもとに、以下の改善を進めています。

| # | 項目 | 目的 | 状況 |
|---|------|------|------|
| 1 | スマホファインダーへの視線誘導改善 | 1Pがメイン画面だけでなく、自然にスマホファインダーを見るようにする | 検討中 |
| 2 | スマホ側のみの構図補助・高得点ヒント表示 | スマホ画面を見るメリットをゲームルール側から強める | 設計検討中 |
| 3 | ズーム開始時・撮影直前の視線誘導演出 | プレイヤーがスマホ画面へ視線を移すタイミングを作る | 設計検討中 |
| 4 | `SceneCapture2D` の更新頻度制御 | スマホファインダー表示の安定性を保ちつつ描画負荷を下げる | 改善予定 |
| 5 | `RenderTarget` 解像度の調整 | スマホ表示に必要な画質を維持しながら描画負荷を下げる | 改善予定 |
| 6 | スポーン処理のフレーム分散 | レベル開始時にトマト・観客・投げる人を一括生成することによるカクつきを減らす | 改善予定 |
| 7 | トマト・エフェクトのオブジェクトプール化 | 大量生成・破棄による負荷を抑える | 改善予定 |
| 8 | 大量生成オブジェクトの影設定見直し | `ProfileGPU` で確認した `ShadowDepths` の負荷を下げる | 一部対応済み |
| 9 | Leap Motion Controllerのパッケージビルド動作確認 | エディタ上だけでなく、パッケージ版でも安定して入力を取得できるか確認する | ログ確認中 |
| 10 | スマホ用 `SWindow` の自動配置 | 展示環境ごとのウィンドウ位置調整をさらに減らす | 改善予定 |
| 11 | カメラ型コントローラー筐体の改善 | 現在の段ボール製筐体から、より持ちやすく見た目の良い形状に改善する | 3Dプリンター活用を検討中 |

## 既知の問題・今後の改善

現時点で把握している課題と、今後の改善方針です。

| # | 課題 | 重要度 | 現状・原因 | 改善方針 |
|---|------|--------|------------|----------|
| 1 | スマホファインダーへの視線誘導が弱い | 高 | 試遊では、1Pがズーム中もメインモニターを見続ける場面が多かった。スマホ画面を見なくても最低限プレイできるため、スマホファインダーを見る必然性がまだ弱い。 | スマホ側にのみ表示される構図補助や高得点判定のヒントを追加し、スマホ画面を見るメリットを強める。ズーム開始時や撮影直前に視線誘導演出を入れる。 |
| 2 | レベル開始時のスポーン処理によるカクつき | 高 | トマト・観客・投げる人などを開始時にまとめて生成しているため、処理が一時的に集中する。 | スポーン処理をフレーム分散する。動きの少ないオブジェクトは事前配置にする。トマトやエフェクトはオブジェクトプール化を検討する。 |
| 3 | 大量生成オブジェクトの影による描画負荷 | 高 | `ProfileGPU` で `ShadowDepths` が大きいことを確認した。大量にスポーンするトマトの影が負荷に影響していた。 | トマトの影を無効化したところ、Frameが約31.7msから約23.7msへ改善した。今後は他の大量生成オブジェクトの影設定も見直す。 |
| 4 | `SceneCapture2D` の更新負荷 | 中 | スマホファインダー映像の安定表示を優先し、現状は `CaptureScene()` を明示的に呼んでいる。表示は安定する一方、描画負荷が増える可能性がある。 | ズーム中のみ更新する、2フレームに1回更新する、`RenderTarget` 解像度を調整するなど、体験を損なわない範囲で負荷を下げる。 |
| 5 | Leap Motion Controllerのパッケージビルドでの認識確認 | 中 | エディタ上では動作しているが、パッケージビルド時の認識やログ確認が必要。 | `Saved/Logs/Tomato.log` を確認し、`UltraleapTracking` プラグインの読み込み状況や入力取得状況を検証する。 |
| 6 | スマホウィンドウ位置の手動調整 | 中 | 現状はスマホ用 `SWindow` の位置とサイズを手動指定しているため、展示環境によって微調整が必要になる。 | 接続ディスプレイ情報を取得し、スマホ用ウィンドウの自動配置ロジックを検討する。 |
| 7 | カメラ型コントローラー筐体の完成度 | 低 | 現在の筐体は段ボール製で、操作体験の検証を優先した試作段階である。 | 今後は3Dプリンターを活用し、持ちやすさや見た目を改善する。 |

これらの課題は、現時点で未完成な点を隠すためではなく、試遊・計測を通して見えた改善対象として整理しています。特にスマホファインダーへの視線誘導と描画負荷は、今後の優先改善項目です。

## ライセンス

このリポジトリ内のコードおよび自作素材のライセンスは未設定です。第三者アセットは各配布元・購入元のライセンスに従い、本 README の記載はライセンス条件そのものを置き換えるものではありません。

### 使用アセット・外部素材

| 種別 | 配置場所 / ファイル | 出典・ライセンス確認 |
|------|---------------------|----------------------|
| Unreal Engine テンプレート / Starter 系素材 | `Content/FirstPerson/`, `Content/LevelPrototyping/`, `Content/Characters/Mannequins/`, `Content/Input/` | Epic Games / Unreal Engine 付属コンテンツ。Unreal Engine EULA / Epic Content License の範囲で使用。 |
| Fab / Unreal Marketplace / Quixel 系 3D アセット | `Content/Fab/Gorilla/`, `Content/Fab/Megascans/3D/Trash_Bag_Pack_ve2hddjga/`, `Content/MSPresets/`, `Content/Fantastic_Dungeon_Pack/`, `Content/LowPolyMarket/`, `Content/ModularBuildingSet/`, `Content/RPGHeroSquad/`, `Content/Scanned3DPeoplePack/`, `Content/UFO/` | Fab Standard License または取得時の Marketplace ライセンスに従う。Fab Standard License は、プロジェクトへ組み込んだ形での商用/私的利用を許可する一方、アセット単体の再配布・再販売は禁止。 |
| Fab / Megascans 由来と思われる街小物 | `Content/mono/gomi/`, `Content/mono/Firehydrant/`, `Content/mono/gomibako/` | `Trash_Bag_Pack_ve2hddjga` は Megascans 系と一致。消火栓・ゴミ箱は出典名がファイル名だけでは未特定のため、公開前に取得元を再確認すること。 |
| Ultraleap Tracking Plugin | `Plugins/UltraleapTracking_ue5_4-5.0.1/` | Ultraleap Unreal Plugin。GitHub 版は Apache License 2.0。実機利用には Ultraleap Tracking Software / SDK 側の条件も確認すること。 |
| Freesound 環境音 | `Content/Sound/755969__lastraindrop__evening-food-market-atmosphere-at-street-side.uasset` | Freesound: “Evening food market atmosphere at street side” by `lastraindrop`。Creative Commons 0 (CC0)。 |
| ニコニ・コモンズ由来と思われる音源 | `Content/Sound/nc146963.uasset` | `nc146963` という ID からニコニ・コモンズ素材と推定。素材ページの利用条件を公開前に再確認すること。 |
| その他サウンド | `Content/Sound/*.uasset`, `Content/Sounds/*.uasset`, `Content/Sounds/Background/BGM182-161031-banbanjikenpou-wav.uasset` | `MainBGM`, `Title_BGM`, `Dram`, `Exelent`, `Nice`, `rakutan`, `Shutter`, `Bubble`, `LaserGun`, `Shot`, `Squish`, `Waping` など。ファイル名だけでは配布元を特定できないものがあるため、公開・配布前に取得元と利用条件を確認すること。 |
| Pngtree 画像素材 | `Content/Assets/—Pngtree—white_crumpled_towel_after_use_13244949.uasset`, `Content/Tomato_Asset/pngtree-blood-splatter-drop-png-image_13534558.uasset` | Pngtree License Terms に従う。プラン種別や利用範囲により条件が変わるため、配布・公開前にアカウント側の取得ライセンスを確認すること。 |
| イラストくん素材 | `Content/Assets/illustkun-03200-tomato.uasset`, `Content/Assets/illustkun-03200-tomato_Sprite.uasset` | イラストくん利用規約に従う。個人・法人利用、商用利用は規約範囲内で可。素材自体の再配布・販売は禁止。 |
| The Noun Project アイコン | `Content/Assets/noun-focus-point-4695835.uasset` | The Noun Project 素材。無料利用時は CC BY 表記が必要、購入/サブスクリプション取得時はロイヤリティフリー条件になるため、取得形態を確認すること。 |
| Google Gemini 生成画像 | `Content/Assets/Gemini_Generated_Image_*.uasset` | Gemini で生成した画像。Google の利用規約・禁止用途ポリシーに従う。公開時は、第三者の権利侵害がないか目視確認すること。 |
| 出典未特定の画像 / 3D モデル | `Content/Assets/23005347*.uasset`, `Content/Assets/background02.uasset`, `Content/Assets/Home.uasset`, `Content/pictures/*.uasset`, `Content/Tomato_Asset/uploads_files_5545752_tomato_oshinchan_model*.uasset`, `Content/Title/Asset/*.uasset` など | 現時点ではファイル名だけで配布元・利用条件を確定できない。外部公開・配布前に、元データの取得元とライセンスを確認すること。 |

### 参照した主なライセンス情報

- Fab Standard License: https://www.fab.com/eula
- Fab / Quixel Megascans: https://www.fab.com/sellers/Quixel%20Megascans/about
- Ultraleap Unreal Plugin: https://github.com/ultraleap/UnrealPlugin
- Freesound `755969`: https://freesound.org/people/lastraindrop/sounds/755969/
- Pngtree License Terms: https://pngtree.com/legal/terms-of-license
- イラストくん ご利用について: https://illustkun.com/about-use/
- The Noun Project license help: https://help.thenounproject.com/hc/en-us/articles/200509798-What-licenses-do-you-offer-for-icons
- Gemini 画像生成ヘルプ: https://support.google.com/gemini/answer/14286560

### 公開前チェック

- `Content/Sound/` と `Content/Sounds/` の音源について、配布元とクレジット要否をチーム内で再確認する。
- `Content/Assets/`, `Content/pictures/`, `Content/Title/Asset/`, `Content/Tomato_Asset/` の出典未特定素材は、配布元 URL または自作であることを確認する。
- Fab / Marketplace アセットは、リポジトリ公開時にアセット単体を取得できる形で再配布してよいかを確認する。必要なら公開リポジトリから第三者 `.uasset` を除外する。
