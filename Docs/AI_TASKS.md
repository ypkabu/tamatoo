# AI_TASKS

Codex App が処理するタスクの管理。Claude Code は基本ここを書き換えない（タスク追加・状態更新は人間 or Codex App）。
詳細な作業結果は `AI_WORKLOG.md`、調査・設計メモは `AI_NOTES.md` へ。

---

## 状態の凡例

| 記号 | 意味 |
|------|------|
| `[ ]`     | TODO（未着手） |
| `[~]`     | DOING（着手中） |
| `[x]`     | DONE（完了。`AI_WORKLOG.md` に記録あり） |
| `[!]`     | BLOCKED（後述の条件参照） |
| `[?]`     | 要確認（仕様 or 受け入れ条件が未確定） |

---

## タスク追加ルール

1 タスクは 1 ブロックで書く。テンプレ:

```
### T-XXX: <短いタイトル>
- 状態: [ ]
- 優先度: P1 / P2 / P3
- 担当: Codex App / Claude Code / 人間
- 関連: <他タスクID, Issue, ファイルパス等>

**背景**
<なぜやるか 1〜2 行>

**完了条件 (DoD)**
- [ ] <具体的に観測可能な条件>
- [ ] <UE Editor で確認できること>

**触ってよいファイル**
- Source/Tomato/...

**触ってはいけないファイル**
- Content/**/*.uasset, Content/**/*.umap （原則）
- Binaries/, Intermediate/, Saved/, DerivedDataCache/

**メモ**
- 要確認: <未確定事項>
```

ルール:
- ID は `T-001` から連番。Done に移動しても ID は再利用しない。
- 状態が `[!]` `[?]` のときは理由を **メモ** に書く。
- 完了条件は「観測可能」な形で書く（"動く"ではなく "PIE で X が表示される"）。
- 1 タスクのスコープは半日〜1 日で終わる粒度に分ける。

---

## 優先度

| 記号 | 意味 |
|------|------|
| **P1** | 今のマイルストーン（次の展示 / ES 提出など）に必須 |
| **P2** | 入っていると望ましいが今回見送り可 |
| **P3** | アイデア・将来やるかも |

要確認: 直近のマイルストーンと締切（ES 提出日 / 展示会日）が未記入。決まったらここに追記。

---

## 触ってよいファイル / 触ってはいけないファイル（プロジェクト全体ルール）

`AGENTS.md` のルールを継承。タスク個別に上書きしたい場合のみ各タスクの「触ってよい/はいけないファイル」欄で明示する。

**触ってよい（既定）**
- `Source/Tomato/**/*.{h,cpp}`
- `Source/Tomato/Tomato.Build.cs`
- `Config/Default*.ini`（要確認: 既存値の上書きは事前同意を取る）
- `Docs/**/*.md`
- `TODO.md`, `README.md` 等のルート Markdown

**触ってはいけない（既定）**
- `Content/**/*.uasset`
- `Content/**/*.umap`
- `Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/`, `.vs/`
- `*.generated.h`（自動生成）
- `Plugins/UltraleapTracking_*`（外部プラグイン本体）
- `Tomato.uproject` の構造変更（プラグイン追加など）は事前承認必須

Blueprint 側の作業が必要なときは、編集する代わりに `AI_NOTES.md` の「人間に確認すること」へ手順を記載する。

---

## BLOCKED にする条件

下記いずれかに該当したら `[!]` BLOCKED にし、メモへ理由を書く:

- 仕様が未確定で人間の判断が必要
- 触ってはいけないファイル（`.uasset` / `.umap`）を編集しないと完了できない
- LeapMotion 実機がないと動作確認できない
- 他タスク T-YYY の完了待ち
- 外部プラグイン / エンジンのバグ起因（要確認: 回避策の有無）
- ビルドが通らない（クリーンビルド要求のため人間操作待ち）

BLOCKED は「人間 or 別タスクのアクション待ち」のサイン。Codex App は BLOCKED タスクを勝手に再開しない。

---

## タスク候補

> 下書き。`README.md` / `COPILOT_HANDOFF.md` / `TODO.md` から拾った既知の未対応項目を入れている。
> 着手前に人間が優先度・受け入れ条件を確認すること。

### T-001: LeapMotion パッケージビルド時の認識調査
- 状態: [?]
- 優先度: P1
- 担当: 人間（ログ取得） + Codex App（ログ解析）
- 関連: `README.md` 開発中の機能 / `TomatinaTowelSystem.cpp`

**背景**
パッケージ済み `Tomato.exe` で LeapMotion が認識されない可能性あり。診断ログは仕込み済み。

**完了条件 (DoD)**
- [ ] `Saved/Logs/Tomato.log` から LeapMotion 初期化と毎フレーム手データ取得の有無を確認
- [ ] 失敗していれば原因仮説を `AI_NOTES.md` に記録
- [ ] 必要なら `Tomato.uproject` のプラグイン設定見直し案を提示（実装はしない）

**触ってよいファイル**
- `Docs/AI_NOTES.md`

**触ってはいけないファイル**
- 既定どおり

**メモ**
- 要確認: パッケージ済みの最新ログがどこにあるか（`Saved/Logs/` 直下 or 配布先 PC か）

---

### T-002: スマホ画面（iPhone 側 ZoomView）の視認性改善
- 状態: [?]
- 優先度: P1
- 担当: Codex App
- 関連: `COPILOT_HANDOFF.md` §15-2 / `TomatinaHUD.cpp`

**背景**
試遊会で 1P がメイン画面とスマホを同時に見るのが困難と判明。設置レイアウト or UI 設計の見直しが必要。

**完了条件 (DoD)**
- [ ] 改善案を 2〜3 パターン `AI_NOTES.md` の設計メモに書き出す
- [ ] 1 案を選び、C++ 側で対応可能な部分のみ実装
- [ ] PIE（解像度 3584×1600 or 4816×1600）で iPhone 領域に ZoomView が表示されることを確認

**触ってよいファイル**
- `Source/Tomato/TomatinaHUD.{h,cpp}`
- `Source/Tomato/TomatinaPlayerPawn.{h,cpp}`

**触ってはいけないファイル**
- `WBP_ViewFinder.uasset` 等（既定）

**メモ**
- 要確認: 現状のスマホ解像度は `2256x1179` か、それとも `1024x768` か（README と handoff で記述が割れている）

---

### T-003: スマホウィンドウ位置の自動配置ロジック化
- 状態: [ ]
- 優先度: P2
- 担当: Codex App
- 関連: `README.md` 既知問題 #3

**背景**
現状はウィンドウ位置を手動指定しており解像度依存。

**完了条件 (DoD)**
- [ ] 起動時のディスプレイ構成を取得し、メインモニター右隣にスマホウィンドウを自動配置
- [ ] 設定上書き用フラグ（手動配置に戻すオプション）を `UPROPERTY` で公開

**触ってよいファイル**
- `Source/Tomato/TomatinaPlayerPawn.{h,cpp}` または対応する HUD/Subsystem

**触ってはいけないファイル**
- 既定どおり

**メモ**
- 要確認: 展示 PC のディスプレイ構成（接続順、プライマリ位置）

---

### T-004: 撮影スコアの最低保証点
- 状態: [ ]
- 優先度: P3
- 担当: Codex App
- 関連: `COPILOT_HANDOFF.md` §9 バグ・改善

**背景**
`CalculatePhotoScore` で「写ってない」とき 0 点。展示で連続 0 点だと体験が悪い。

**完了条件 (DoD)**
- [ ] 写ってないときの最低点を `UPROPERTY` で公開（デフォルト 0、推奨 5）
- [ ] PIE で撮影 → 何も写してないときに最低点が反映される

**触ってよいファイル**
- `Source/Tomato/TomatinaFunctionLibrary.{h,cpp}`
- `Source/Tomato/TomatinaGameMode.{h,cpp}`

**触ってはいけないファイル**
- 既定どおり

---

### T-005: `MyActor.*` / 旧 ProjectileSpawner の削除
- 状態: [ ]
- 優先度: P3
- 担当: Codex App
- 関連: `COPILOT_HANDOFF.md` §13

**背景**
未使用ファイルが残っている。

**完了条件 (DoD)**
- [ ] `Source/Tomato/MyActor.{h,cpp}` への参照ゼロを確認してから削除
- [ ] `TomatinaProjectileSpawner.{h,cpp}` も同様に参照ゼロ確認後に削除（要確認: BP からの参照有無）
- [ ] クリーンビルド成功

**触ってよいファイル**
- 上記 C++ ファイル

**触ってはいけないファイル**
- 既定どおり（`.uasset` から参照されている場合は BLOCKED）

**メモ**
- 要確認: ProjectileSpawner はレベルから外せるか（`COPILOT_HANDOFF.md` は「消して OK」と記載）

---

## Done

完了タスクはここに移動する（ID と完了日のみ。詳細は `AI_WORKLOG.md` の該当日付）。

- [ ] （まだなし）
