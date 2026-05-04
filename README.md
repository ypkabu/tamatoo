# Wipe&Snap

スペインのトマト祭り「La Tomatina」をモチーフにした、2人協力プレイの体験型撮影ゲーム。1Pはカメラマンとして自作カメラ型コントローラーを構え、群衆の中からお題の被写体を探して撮影する。2PはLeap Motion Controllerで手を動かし、トマトで汚れたレンズを拭き取って1Pの撮影を支援する。

## プロジェクト概要

- **タイトル**: Wipe&Snap (リポジトリ名 `Wipe-Snap`)
- **ジャンル**: 2人協力 / 体験型 / ハードウェア連動
- **想定設置環境**: モニター + スマートフォン画面(デュアルディスプレイ) + Leap Motion Controller
- **プレイ人数**: 2人 (1P=カメラマン、2P=タオル拭き取り)
- **想定プレイ時間**: 約3分
- **用途**: 学内展示 / 試遊会

## デモ

- プレイ動画: URL
- Zenn 技術記事: URL
- 写真 / GIF: 追加予定

## 自分の担当

1P/2Pの操作体験まわりのクライアント実装を担当した。

- スマホ表示用 `SWindow` の生成
- `SceneCapture2D` + `RenderTarget` によるズームファインダー
- Leap Motion Controllerの手の速度を使った拭き取り判定
- ズームカメラの壁抜け対策
- `ProfileGPU` による描画負荷確認とトマト影設定の改善
- 展示環境での表示安定化

|担当者  |主な担当                                                                                                              |
|-----|------------------------------------------------------------------------------------------------------------------|
|自分   |ゲーム体験の企画、UE5/C++実装、スマホ表示用SWindow、SceneCapture2D + RenderTargetによるズームファインダー、Leap Motion Controllerによる拭き取り判定、UI/UX調整|
|メンバーA|3Dモデル、アニメーション調整、モデルアタッチ、ワールドへのアクター配置、街並み作成、Blueprintでの敵キャラの動き調整                                                   |
|メンバーB|イラスト、背景作成                                                                                                         |
|メンバーC|ゲームルールの検討、制作補助、調整作業、カメラ型筐体製作                                                                                      |

筐体制作、3Dモデル、ステージ制作、サウンド、ゲームルール検討は主に他メンバーが担当した。チーム制作のため、他メンバーの成果と自分の成果を区別できるよう、担当範囲を分けて記載している。

## 設計判断

クライアント側の実装は、1Pの撮影体験 (`ATomatinaPlayerPawn`)、2Pの拭き取り体験 (`ATomatinaTowelSystem`)、汚れの状態管理 (`ATomatoDirtManager`)、別ウィンドウ表示の管理 (`ATomatinaHUD`) を独立したクラスに分けた。1Pと2Pの操作体験をクラス単位で切り離したことで、試遊フィードバックを受けて片方のパラメータを調整しても、もう片方の挙動に影響しにくい構成にできた。

別ウィンドウ生成を `ATomatinaHUD` に置き、`ATomatinaPlayerPawn` から切り離したのは、シングルウィンドウ + Widgetスパン方式から `SWindow` 方式へ切り替えた経緯があるため。表示形態の変更がゲームロジック側に漏れないように責務を分けた。`ATomatinaTowelSystem` が `ATomatoDirtManager` に直接書き込まず、`WipeDirtAt(NormPos, Radius, Amount)` 経由で操作する形にしたのも同じ理由で、汚れの表現方式を変更しても2P側の判定処理を変えずに済むようにしている。

### 操作概要

|役割      |入力デバイス                                     |主な操作                  |
|--------|-------------------------------------------|----------------------|
|1P カメラマン|キーボード/コントローラ (Enhanced Input)              |視点移動・トマト発射・スマホ画面でズーム確認|
|2P 拭き取り |Leap Motion Controller (Ultraleap Tracking)|手のジェスチャでメイン画面の汚れを拭く   |

## Unreal Engine のバージョン

- **UE 5.7** (`Tomato.uproject` の `EngineAssociation` で確認)
- ビルド構成: C++ + Blueprint
- パッケージング: Monolithic (`Tomato.exe` 約 297MB)

## 主な実装と改善の成果

試遊・計測を踏まえて改善まで完了した項目を先にまとめる。

|項目               |内容                                                                                                        |
|-----------------|----------------------------------------------------------------------------------------------------------|
|マルチディスプレイ表示      |シングルウィンドウ + Widgetスパン方式から `SWindow` 方式に切り替え。展示前の画面位置調整を、長いときの約1時間から数分程度に短縮                                |
|ズームファインダー表示の安定化  |`RenderTarget` を直接Brushに渡す代わりに `UMaterialInstanceDynamic` のTexture Parameter経由で表示することで、サブウィンドウ側の更新反映を安定させた|
|描画負荷の改善          |`ProfileGPU` で `ShadowDepths` がボトルネックであることを特定し、トマト影を無効化することでFrameを約31.7msから約23.7msへ改善                     |
|Leap Motion入力の安定化|EMA / One Euro Filterによるスムージング、`InputGraceTime` 内での入力保持、座標Clamp、画面端での拭き取り半径補正を実装 (PIE確認済み)                |
|ズームカメラの壁抜け対策     |Sphere Sweep + Safety Marginによる位置クランプと、`SkyFallbackDistance` による空方向の仮想ヒット処理                               |

### 60fps目標と現状

現状Frameは約23.7msで、60fps相当の16.6msには未達。直近の打ち手として `SceneCapture2D` をズーム中のみ更新する変更を進めており、続いて `Postprocessing` の軽量化と `RenderTarget` 解像度の調整を検討する。

## 現在実装されている機能

### ゲームプレイ

- 1P: トマト発射、視点移動、スマホファインダー越しのズーム撮影
- 2P: Leap Motion Controllerによる拭き取り (`UTomatinaTowelSystem`)
- トマト発射 (`ATomatinaProjectile`) と着弾位置への汚れ生成
- 汚れ管理 (`ATomatoDirtManager`)
  - 通常トマト (赤): 円形範囲のOpacity漸減
  - 特殊トマト (黄、Sticky): 連続ダッシュ × 4回で剥がれる
- スタイリッシュランク (C / B / A / S / SSS) とSync Rate計測
- ミッション制ゲーム進行 (`ATomatinaGameMode` 内Missions配列)
- 全汚れクリア演出 → 最終リザルトへの遷移

### ハードウェア / 描画

- デュアルウィンドウ表示: `FSlateApplication::AddWindow` + 動的 `SWindow` 生成によりメイン画面とスマホ画面を別ウィンドウで描画
- スマホ画面に `RT_Zoom` レンダーターゲットを表示 (`ConfigureZoomImageContent`、DMI経由)
- DPIスケーリング補正 (`UWidgetLayoutLibrary::GetViewportScale()`)
- UE 5.7 SceneCapture自動キャプチャ問題の回避 (Tick内で `CaptureScene()` 手動呼び出し)
- ズームカメラの壁抜け対策 (Sphere Sweep + Safety Margin + `CustomNearClippingPlane`)
- 空・無限遠ヒット対策 (`SkyFallbackDistance` で仮想ヒット点)
- キャラクタ全体に汚れマスクを乗せる `SetOverlayMaterial` (UE 5.0+)

### UI / 演出

- ポーズ可能なUI (`SetGlobalTimeDilation(0)` + `FApp::GetDeltaTime()`)
- 拭き取りSEのループ・エッジ検出 (`UpdateWipeSound`)
- BeginPlay前のプロパティ注入 (`SpawnActorDeferred` + `FinishSpawning`)

## 既知の問題

優先度の高い課題を3つに絞った。

|#|課題                  |現状                                                            |改善方針                                                                            |
|-|--------------------|--------------------------------------------------------------|--------------------------------------------------------------------------------|
|1|スマホファインダーへの視線誘導が弱い  |試遊では8人中5人が、ズーム中もメインモニターを見続けた。スマホ画面を見なくても最低限プレイできるため、見る必然性がまだ弱い|スマホ側にのみ表示される構図補助や高得点判定のヒントを追加し、見るメリットをゲームルール側から強める。ズーム開始時に視線誘導演出を入れる            |
|2|レベル開始時のスポーン処理によるカクつき|トマト・観客・投げる人を開始時に一括生成しているため、処理が一時的に集中する                        |スポーン処理のフレーム分散、動きの少ないオブジェクトの事前配置、トマト・エフェクトのオブジェクトプール化                            |
|3|60fps未達             |トマト影OFF後もFrameは約23.7msで、60fpsの16.6msに届いていない                   |`SceneCapture2D` のズーム中のみ更新、`Postprocessing` の軽量化、スマホ用 `RenderTarget` 解像度の調整を順に検討|

## 開発中・改善予定の機能

|#|項目                                         |目的                               |状況   |
|-|-------------------------------------------|---------------------------------|-----|
|1|スマホ側のみの構図補助・高得点ヒント                         |スマホ画面を見るメリットをルール側から強める           |設計検討中|
|2|ズーム開始時の視線誘導演出                              |プレイヤーがスマホ画面へ視線を移すきっかけを作る         |設計検討中|
|3|`SceneCapture2D` のズーム中のみ更新                 |ズーム外でのGPU負荷を削減する                 |実装中  |
|4|`Postprocessing` 軽量化 / `RenderTarget` 解像度調整|60fps到達に向けて描画負荷を下げる              |改善予定 |
|5|スポーン処理のフレーム分散 / オブジェクトプール化                 |開始時のカクつきを解消する                    |改善予定 |
|6|スマホ用 `SWindow` の自動配置                       |接続ディスプレイ情報に合わせてウィンドウ位置を自動決定する    |改善予定 |
|7|カメラ型コントローラー筐体の改善                           |段ボール製の現行筐体を、3Dプリンターで持ちやすい形状に置き換える|検討中  |

## ライセンス

このリポジトリ内のコードおよび自作素材のライセンスは未設定。第三者アセットは各配布元・購入元のライセンスに従う。

### 使用アセット・外部素材

|種別                                      |配置場所 / ファイル                                                                                                                                                                                                                                                            |出典・ライセンス                                                                                             |
|----------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------|
|Unreal Engine テンプレート / Starter系素材       |`Content/FirstPerson/`, `Content/LevelPrototyping/`, `Content/Characters/Mannequins/`, `Content/Input/`                                                                                                                                                                |Epic Games / Unreal Engine付属コンテンツ。Unreal Engine EULA / Epic Content Licenseの範囲で使用                    |
|Fab / Unreal Marketplace / Quixel系3Dアセット|`Content/Fab/Gorilla/`, `Content/Fab/Megascans/3D/Trash_Bag_Pack_ve2hddjga/`, `Content/MSPresets/`, `Content/Fantastic_Dungeon_Pack/`, `Content/LowPolyMarket/`, `Content/ModularBuildingSet/`, `Content/RPGHeroSquad/`, `Content/Scanned3DPeoplePack/`, `Content/UFO/`|Fab Standard Licenseまたは取得時のMarketplaceライセンスに従う。プロジェクトへ組み込んだ形での商用/私的利用は許可、アセット単体の再配布・再販売は禁止           |
|Fab / Megascans                         |`Content/mono/gomi/`, `Content/mono/Firehydrant/`, `Content/mono/gomibako/`                                                                                                                                                                                            |`Trash_Bag_Pack_ve2hddjga` 等                                                                         |
|Ultraleap Tracking Plugin               |`Plugins/UltraleapTracking_ue5_4-5.0.1/`                                                                                                                                                                                                                               |Ultraleap Unreal Plugin。GitHub版はApache License 2.0。実機利用にはUltraleap Tracking Softwareが必要              |
|Freesound 環境音                           |`Content/Sound/755969__lastraindrop__evening-food-market-atmosphere-at-street-side.uasset`                                                                                                                                                                             |Freesound: “Evening food market atmosphere at street side” by `lastraindrop`、Creative Commons 0 (CC0)|
|ニコニ・コモンズ                                |`Content/Sound/nc146963.uasset`                                                                                                                                                                                                                                        |ニコニ・コモンズ素材、素材ページの利用条件に従って使用                                                                          |
|Pngtree 画像素材                            |`Content/Assets/—Pngtree—white_crumpled_towel_after_use_13244949.uasset`, `Content/Tomato_Asset/pngtree-blood-splatter-drop-png-image_13534558.uasset`                                                                                                                 |Pngtree License Termsに従う                                                                             |
|イラストくん素材                                |`Content/Assets/illustkun-03200-tomato.uasset`, `Content/Assets/illustkun-03200-tomato_Sprite.uasset`                                                                                                                                                                  |イラストくん利用規約に従う。個人・法人利用、商用利用は規約範囲内で可。素材自体の再配布・販売は禁止                                                    |
|The Noun Project アイコン                   |`Content/Assets/noun-focus-point-4695835.uasset`                                                                                                                                                                                                                       |The Noun Project素材                                                                                   |
|SunoAI BGM                              |`Content/Sound/` 配下のBGMアセット                                                                                                                                                                                                                                            |SunoAIで生成したBGMを使用。ゲームの雰囲気に合うものを選定し、音量や使用場面を調整                                                        |
|生成AIによる汚れ画像素材                           |レンズ汚れ・トマト汚れ表現に使用している画像アセット                                                                                                                                                                                                                                             |image2で生成した汚れ画像を、ゲーム内のレンズ汚れ表現として使用。見た目や透明度を調整し、視認性を確認                                                |

### 参照した主なライセンス情報

- Fab Standard License: https://www.fab.com/eula
- Fab / Quixel Megascans: https://www.fab.com/sellers/Quixel%20Megascans/about
- Ultraleap Unreal Plugin: https://github.com/ultraleap/UnrealPlugin
- Freesound `755969`: https://freesound.org/people/lastraindrop/sounds/755969/
- Pngtree License Terms: https://pngtree.com/legal/terms-of-license
- イラストくん ご利用について: https://illustkun.com/about-use/
- The Noun Project license help: https://help.thenounproject.com/hc/en-us/articles/200509798-What-licenses-do-you-offer-for-icons

## 生成AIの利用について

本作では、Claude Code と Codex を、コード修正案の作成、既存コードの整理、バグ原因の仮説出し、READMEや記事構成の整理に使った。

実装時にはAIの提案も参考にしたが、最終的な仕様判断、プロジェクトへの組み込み、実機での動作確認、パラメータ調整、採用する実装方針の決定は自分で行った。

特に、スマホファインダー表示、`RenderTarget` 表示、Leap Motion入力、`ProfileGPU` による負荷確認では、AIの提案をそのまま使うのではなく、要件に合わせて修正・検証した。

BGM素材の一部にSunoAI、レンズ汚れ表現の画像素材作成にimage2を使用した。生成した素材はそのまま使うのではなく、ゲームの雰囲気や視認性に合うかを確認し、音量・見た目・透明度などを調整した。

本リポジトリにはAIによるコミットが含まれているが、提出にあたっては自分の担当範囲とAI利用範囲を明記している。