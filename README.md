# Wipe&Snap

スペインのトマト祭り「La Tomatina」をモチーフにした、2人協力プレイの体験型ゲーム。
1P がカメラマンとしてトマトを撃ち込み、2P が LeapMotion で「タオル」を動かして汚れを拭き取る。

## プロジェクト概要

- **タイトル**: Wipe&Snap（リポジトリ名 `tamatoo`）
- **ジャンル**: 2人協力 / 体験型 / ハードウェア連動
- **想定設置環境**: モニター + スマートフォン画面（デュアルディスプレイ）+ LeapMotion
- **プレイ人数**: 2 人（1P=カメラマン、2P=タオル拭き取り）
- **想定プレイ時間**: 3分ほど
- **用途**: 学内展示 / 試遊会 

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

- [x] 1P トマト発射（`ATomatinaProjectile`）と着弾位置への汚れ生成
- [x] 2P LeapMotion 拭き取り（`UTomatinaTowelSystem`）
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

## 開発中の機能

- [ ] **LeapMotion パッケージビルドでの認識問題の調査**
  - `Tomato.uproject` に `UltraleapTracking` を明示済み
  - `TomatinaTowelSystem.cpp` に診断ログ仕込み済み
  - 次工程: パッケージ後 `Saved/Logs/Tomato.log` を確認
- [ ] **最終リザルト前の「計測中...」ウィジェット**（`BeginFinalResultBuildup` フェーズ）
- [ ] スマホ画面の視認性改善（後述「既知の問題」参照）

## 既知の問題

| # | 内容 | 重要度 | 状況 |
|---|------|--------|------|
| 1 | **スマホ画面が 1P から見にくい**：1P がメイン画面とスマホを同時に見ながら操作することが試遊会で困難と判明 | 高 | 設置レイアウト or UI 設計の見直しが必要 |
| 2 | LeapMotion パッケージビルドで認識しない可能性 | 中 | ログ取得待ち |
| 3 | スマホウィンドウ位置を手動指定している（解像度依存） | 中 | 自動配置ロジック化を検討 |
| 4 | DPI キャッシュが初回 0 になることがある（`LayoutPhoneZoomImage`） | 低 | 1 フレーム遅延で回避中 |

詳細は `HARDWARE_CHALLENGES.md` を参照（要確認: 別途作成予定）。

## ブランチ運用ルール

要確認（現状 `main` 直コミット運用に見えます）。複数人開発に切り替える場合は以下を提案:

- `main`: 動作確認済み・展示で使えるバージョン
- `feature/<topic>`: 機能追加・改修
- `fix/<topic>`: 不具合修正
- マージは PR 経由、コミットメッセージは要件を 1 行で
- パッケージ済みバイナリは Git に含めない（要確認: 現状 `.gitignore` 設定）

## AI 運用ルール

このリポジトリで AI（Claude Code, Codex 等）を使う場合は **必ず `AGENTS.md` を参照してください**。
（要確認: `AGENTS.md` は未作成のため別途用意が必要）

そこに以下が定義されている前提です:

- 触ってよいファイル / 触ってはいけないファイル
- コーディング規約（C++ 命名、Blueprint との分担）
- パッケージ化前に走らせるべきチェック
- ハードウェア依存のテスト方法

関連ドキュメント:

- `AGENTS.md` — AI への運用指示（要確認: 未作成）
- `HARDWARE_CHALLENGES.md` — ハードウェア実装で踏んだ壁と対処（要確認: 未作成）
- `ES_GAME_OVERVIEW.md` — ES 提出用の総合解説（要確認: 未作成）
- `ZENN_CYGAMES_ES.md` — Cygames インターン ES 用 Zenn 記事ドラフト
- `TOMATINA_SETUP.md` — セットアップ手順
- `COPILOT_HANDOFF.md` — 旧 AI 引き継ぎ資料

## ライセンス

要確認
