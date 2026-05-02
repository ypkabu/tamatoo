# AI_WORKLOG

Codex App / Claude Code が行った作業の結果ログ。最新が上。
タスクの状態管理は `AI_TASKS.md`、設計・調査メモは `AI_NOTES.md`。

---

## 書き方ルール

- 日付は `YYYY-MM-DD`（JST）
- 1 セッション = 1 ブロック。複数タスクをまとめて書いてもよい
- ビルド・PIE で確認した内容は必ず「UE5 Editor での確認手順」に書く
- 確認していないなら「未確認」と明記する（ウソをつかない）
- `.uasset` / `.umap` の編集は禁止。BP 側で必要な手順は本文に書き、人間に渡す

---

## テンプレ（コピーして使う）

```
## YYYY-MM-DD <セッション名 or タイトル>

### 対応タスク
- T-XXX: <タスク名>（状態 [ ] → [x] / [~] / [!]）

### 変更ファイル
- Source/Tomato/Xxx.h
- Source/Tomato/Xxx.cpp

### 実装内容
- <差分の要約 1〜3 行>
- <設計判断のうち非自明なもの>

### UE5 Editor での確認手順
1. Editor / Visual Studio を閉じてクリーンビルド（`Binaries/`, `Intermediate/` 削除 → Generate VS files → Build）
2. PIE 解像度 `3584 x 1600`（要確認: 現行値）で起動
3. <タスク固有の確認手順>
4. Output Log で `<期待ログ>` を確認

### Blueprint 側で人間がやること
- <BP_XXX の Details で YY を ZZ に設定> など。なければ「なし」と書く

### リスク
- <影響範囲が大きい変更 / 触っていないが波及しうる箇所>
- <パッケージビルドでのみ起こりうる挙動>

### 次にやること
- T-YYY を着手 / T-XXX を BLOCKED 解除待ち / 人間に <X> を確認 など
```

---

## 2026-05-02 ドキュメント基盤の整備（雛形）

### 対応タスク
- なし（このセッションは Docs 三点セットの初期作成）

### 変更ファイル
- Docs/AI_TASKS.md（新規）
- Docs/AI_WORKLOG.md（新規・このファイル）
- Docs/AI_NOTES.md（新規）

### 実装内容
- Codex App との長時間タスク運用のための土台ドキュメントを作成
- 既存 `AGENTS.md` / `COPILOT_HANDOFF.md` / `TODO.md` の内容と矛盾しないように整合
- 実装・修正は一切なし

### UE5 Editor での確認手順
- 不要（ドキュメントのみ）

### Blueprint 側で人間がやること
- なし

### リスク
- `AI_TASKS.md` のタスク候補（T-001〜T-005）はドラフト。受け入れ条件・優先度は人間レビュー必須

### 次にやること
- 人間が `AI_TASKS.md` のタスク候補を確認 → 優先度確定
- 直近マイルストーン（展示日 / ES 提出日）を `AI_NOTES.md` に追記
