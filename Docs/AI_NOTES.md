# AI_NOTES

調査メモ・設計メモ・未確定事項の置き場。タスク管理は `AI_TASKS.md`、作業結果は `AI_WORKLOG.md`。

このファイルは Claude Code / Codex App が読み書きする。人間も自由に追記してよい。
古くなった情報は消すのではなく「（YYYY-MM-DD 古い）」と注記して残す（経緯の追跡用）。

---

## プロジェクト調査メモ

### 構成サマリ（2026-05-02 時点、コードベースから読み取り）

- **エンジン**: UE 5.7（`Tomato.uproject` の `EngineAssociation`）
- **モジュール**: `Tomato`（Runtime）
- **依存**: `Core`, `CoreUObject`, `Engine`, `InputCore`, `UMG`, `Slate`, `SlateCore`, `EnhancedInput`
- **プラグイン**: `OpenXR`, `ModelingToolsEditorMode`(Editor only), `GameplayStateTree`, `UltraleapTracking`
- **C++ クラス**（`Source/Tomato/`）
  - 中核: `TomatinaGameMode`, `TomatinaPlayerPawn`, `TomatinaHUD`, `TomatinaFunctionLibrary`
  - ターゲット: `TomatinaTargetBase`, `TomatinaTargetSpawner`
  - 群衆: `TomatinaCrowdMember`, `TomatinaCrowdManager`
  - 投擲: `TomatinaProjectile`, `TomatinaProjectileSpawner`(旧), `TomatinaThrower`, `TomatinaThrowerSpawner`
  - 汚れ: `TomatoDirtManager`, `TomatinaTowelSystem`
  - 未使用候補: `MyActor.{h,cpp}`, `unko.{h,cpp}`（要確認: 後者は `COPILOT_HANDOFF.md` に記載なし）
- **Content/**: `Chiled_BP`, `Throw_BP`, `Tatget_BP`, `Title`, `UI`, `Tomato_Asset` など多数

詳細仕様は `COPILOT_HANDOFF.md` に網羅されている。差分が出たらこちらを更新。

### 関連ドキュメント

- `AGENTS.md` — AI 運用ルール（触ってよい/いけないファイル、必ず報告する 5 項目）
- `README.md` — プロジェクト概要・既知問題
- `TOMATINA_SETUP.md` — クリーンビルド手順 / BP 必要設定
- `COPILOT_HANDOFF.md` — 引き継ぎ仕様書（最終更新 2026-04-23）
- `HARDWARE_CHALLENGES.md` — 要確認（README に「別途作成予定」とある。実在するなら内容反映）
- `ES_GAME_OVERVIEW.md` / `ZENN_CYGAMES_ES.md` — ES 用文書

---

## 設計メモ

### 画面サイズの真のソース

`ATomatinaPlayerPawn` の 4 変数（`MainWidth`, `MainHeight`, `PhoneWidth`, `PhoneHeight`）が Single Source of Truth。
GameMode と HUD は BeginPlay で Pawn から取得する。BP 側で値を上書きしている可能性があるので、不具合調査時はまず BP インスタンス値を確認する。

要確認: README は `2560×1600` + `1024×768`、`COPILOT_HANDOFF.md` §15-3 は `2560×1600` + `2256×1179` と記述が割れている。
→ 現行コードのデフォルトと、`BP_TomatinaPlayerPawn` のインスタンス値、どちらが「展示で使う値」か人間に確認。

### 衝突方針

- トマト = `QueryOnly` + `Pawn = Overlap` + `WorldStatic = Overlap`
- `bAimedAtPlayer = true` のトマトはプレイヤー Pawn のみヒット、他は貫通
- `HidingProp` タグ付きアクターは可視性判定で「遮蔽」扱い

### LeapMotion の取り扱い

- C++ からは触らない（`Tomato.Build.cs` に `"LeapMotion"` を入れない）
- BP（`BP_TomatinaPlayerPawn`）で `Leap Component` を持ち、`OnLeapTrackingData` で手データを受信
- C++ 側 `WipeDirtAt` / `Update Towel Position` を BP から呼ぶ
- 手なし判定は `Hands.Length <= 0`（`OnLeapHandLost` がプラグイン版によっては存在しない）

### スコア計算の急所

- `CalculatePhotoScore` は Head / Root の 2 点で `CheckVisibility` を取り、両見え=100 / 上半身=50 / 足のみ=10 / 写ってない=0
- ターゲットが `bIsPosing` のとき `Score *= PoseScoreMultiplier`
- 汚れ減点は `DirtPenaltyPerSplat` × 写った汚れ数

### Time Dilation = 0 と FApp::GetDeltaTime

ポーズ可能な UI のため `SetGlobalTimeDilation(0)` 中に時間を進めたい処理は `FApp::GetDeltaTime()` を使う（カウントダウン、撮影結果、ミッション結果、シャッターフラッシュ）。Codex App は新規にタイマー処理を書くときも同じ方針を踏襲する。

---

## 未確定事項（要確認）

- [ ] 直近マイルストーン: 次の展示会日 / ES 提出日 / 目標品質ライン
- [ ] 展示 PC のディスプレイ構成（プライマリ位置、解像度、複数ディスプレイの並び）
- [ ] スマホ画面の正しい解像度（`1024x768` か `2256x1179` か）
- [ ] `HARDWARE_CHALLENGES.md` の所在（実在するか / 内容）
- [ ] `unko.{h,cpp}` の用途（残すか削除するか）
- [ ] 旧 `TomatinaProjectileSpawner` をレベルから外して問題ないか（BP 参照確認）
- [ ] `.gitattributes` の現状（`*.uasset` `*.umap` が `binary` 指定されているか / Git LFS 適用か）
- [ ] ライセンス表記（README に「要確認」と記載）
- [ ] ブランチ運用（README は `main` 直コミット運用と推測）

---

## 人間に確認すること

優先度高いもの上に。回答が来たら「未確定事項」から消し、「設計メモ」or「調査メモ」へ移す。

1. **マイルストーン**: 次の締切日と何が必要か。これにより `AI_TASKS.md` の P1/P2/P3 を確定できる。
2. **スマホ解像度の正**: `BP_TomatinaPlayerPawn` インスタンス値を見せてほしい（あるいは Editor 側スクリーンショット）。
3. **LeapMotion 実機**: 開発 PC に常時接続されているか / 動作確認のたびに接続するか。Codex App が実機テストを依頼してよいタイミングを決める。
4. **Blueprint 編集の代行**: `.uasset` を直接編集できない代わりに、変更手順を Markdown で書き出す形でよいか。

---

## Codex App へ渡す注意点

長時間タスクで Codex App に作業を任せるとき、最初のメッセージに含めるべきこと:

- このリポジトリでは **AGENTS.md のルールを最優先**で守る（特に `.uasset` / `.umap` 不可）。
- タスク開始前に `Docs/AI_TASKS.md` から該当 ID を選び、状態を `[~]` に更新してから着手する。
- 完了したら以下を順に行う:
  1. `Docs/AI_WORKLOG.md` に当日のセッションブロックを追記
  2. `Docs/AI_TASKS.md` の該当タスクを `[x]` にし、Done セクションへ移動（ID と日付のみ）
  3. 必要なら `Docs/AI_NOTES.md` の「未確定事項」「設計メモ」を更新
- **Live Coding ではなくクリーンビルド**を要求する変更（USTRUCT メンバ追加など）の場合、`AI_WORKLOG.md` の確認手順にその旨を明記。
- **Clang 偽陽性は無視**（`'CoreMinimal.h' file not found` 等）。実際のエラーは VS Build / Live Coding の結果で判断。
- **数値はマジックナンバーにせず `UPROPERTY(EditAnywhere)`** に追い出す（既存コードのスタイル踏襲）。
- **ログは UE_LOG(LogTemp, Warning, ...)**。要所に入れて後追いしやすくする。
- **C++ で骨組み、BP で見た目調整**の分業を守る。Widget の位置・サイズは原則 C++ から触らない（既存例外: `UpdateCursorPosition`, `UpdateDirtDisplay`, `UpdateTowelPosition`）。
- 困ったら勝手に判断せず、`AI_TASKS.md` のタスクを `[!]` BLOCKED にして「人間に確認すること」へ追記。
