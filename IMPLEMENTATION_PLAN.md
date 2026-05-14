# PlotEngine 実装計画書

> 最終更新: 2026-05-09
> 目標: 構造を保ったまま小説をAIに出力させるIDE風エディタ

---

## 全体ロードマップ

```
Phase 1: AI統合レイヤー（基盤）
  ↓
Phase 2: 構造の可視化・編集
  ↓
Phase 3: 推敲・バージョン管理
  ↓
Phase 4: 書き出し・品質
```

推奨順序:
```
1.1 → 1.2 → 1.3 → 1.4 → 1.5 → 1.6
  ↓
2.3 → 2.1 → 2.2 → 2.4
  ↓
3.1 → 3.2 → 3.3
  ↓
4.2 → 4.1 → 4.3
```

---

## Phase 1: AI統合レイヤー（基盤）

**目標**: AIと対話しながらエピソードを生成・推敲できる状態にする

### 1.1 AI API抽象化インターフェース

| 項目 | 内容 |
|---|---|
| 優先度 | high |
| 新規ファイル | `src/ai/airequest.h`, `src/ai/airequest.cpp`, `src/ai/iaiprovider.h`, `src/ai/iaiprovider.cpp` |
| モジュール | `PlotEngine.Core.AiProvider` (ixx) |
| 依存 | なし |

**実装内容**:
- `AiRequest` — プロンプト、システムメッセージ、温度、最大トークン数を保持するリクエスト構造体
- `AiResponse` — 生成テキスト、使用トークン数、終了理由を保持するレスポンス構造体
- `IAiProvider` — `generate()`, `generateStream()` を持つQtスタイルの抽象インターフェース（Verdigris `W_OBJECT` 使用）
- シグナル: `responseReceived(AiResponse)`, `streamChunk(QString)`, `error(QString)`

### 1.2 OpenAI + Anthropic プロバイダー実装

| 項目 | 内容 |
|---|---|
| 優先度 | high |
| 新規ファイル | `src/ai/openaiprovider.h/cpp`, `src/ai/anthropicprovider.h/cpp` |
| 依存 | 1.1 |

**実装内容**:
- `QNetworkAccessManager` を使ったHTTP通信
- OpenAI: `POST /v1/chat/completions` (streaming: `stream: true`)
- Anthropic: `POST /v1/messages` (streaming: `stream: true`)
- JSONパーシング、エラーハンドリング、リトライロジック
- APIキーは設定から取得（ハードコード禁止）

### 1.3 AI設定パネル

| 項目 | 内容 |
|---|---|
| 優先度 | high |
| 新規ファイル | `src/panels/aipanel.h/cpp`, `src/panels/aipanel.ui`（任意） |
| 変更ファイル | `src/mainwindow.h/cpp`（メニューに設定追加） |
| 依存 | 1.2 |

**実装内容**:
- プロバイダー選択（OpenAI / Anthropic / ローカルLLM）
- APIキー入力（QSettingsに暗号化保存）
- モデル選択ドロップダウン（gpt-4o, claude-sonnet, etc.）
- 温度・最大トークンのスライダー
- 接続テストボタン
- 設定は `QSettings` で永続化

### 1.4 コンテキストビルダー

| 項目 | 内容 |
|---|---|
| 優先度 | high |
| 新規ファイル | `src/ai/contextbuilder.ixx` |
| モジュール | `PlotEngine.Core.ContextBuilder` |
| 依存 | 1.1, `NovelProject` |

**実装内容**:
- `NovelProject` からAI用コンテキストを自動構築
- キャラクター情報（名前、役割、説明、ノート）
- 場所情報（名前、説明、ノート）
- 章の要約と前後のエピソードコンテキスト
- スタイルガイド（ユーザー定義の文体指示）
- `buildContext()` → 完成したシステムプロンプトを返す
- `buildEpisodePrompt()` → 特定エピソード用の生成プロンプト

### 1.5 ストリーミング出力をエディタに統合

| 項目 | 内容 |
|---|---|
| 優先度 | high |
| 変更ファイル | `src/editor/noveleditor.h/cpp`, `src/mainwindow.h/cpp` |
| 依存 | 1.2, 1.3, 1.4 |

**実装内容**:
- `NovelEditor` に `startAiGeneration(AiRequest)` メソッド追加
- ストリーミングチャンクをリアルタイムでテキストに挿入
- 生成中はエディタを部分的にロック（ユーザー編集は一時停止）
- 生成完了後、リビジョンとして自動保存（Phase 3.1 へ接続）
- キャンセルボタン対応
- ツールバーに「AI生成」ボタン追加

### 1.6 バッチ生成（複数エピソード一括）

| 項目 | 内容 |
|---|---|
| 優先度 | medium |
| 新規ファイル | `src/ai/batchgenerator.h/cpp`, `src/panels/batchpanel.h/cpp` |
| 依存 | 1.4, 1.5 |

**実装内容**:
- 章全体のあらすじから複数エピソードを一括生成
- 各エピソードの要約→本文の2段階生成
- 進捗バー表示（何エピソード目／全体）
- 生成結果のプレビュー＆個別承認/却下
- キュー管理（直列実行、失敗時のリトライ）

---

## Phase 2: 構造の可視化・編集

**目標**: Scrivener級の構造管理＋AIがメタデータを参照可能に

### 2.3 シーンメタデータ（POV/時間帯/感情）

> 2.1 より先に実装。メタデータ構造が他のパネルの基盤になる。

| 項目 | 内容 |
|---|---|
| 優先度 | high |
| 変更ファイル | `src/core/novelproject.h/cpp`, `src/core/novelproject.ixx` |
| 依存 | なし |

**実装内容**:
- `Episode` にメタデータ追加:
  - `QString povCharacter` — 視点キャラクターID
  - `QString timePeriod` — 時間帯（朝/昼/夕/夜/深夜）
  - `int emotionalIntensity` — 感情カーブ（1-10）
  - `QString sceneType` — シーン種別（導入/対立/クライマックス/解決）
  - `int targetWordCount` — 目標文字数
- JSONシリアライズ/デシリアライズ更新
- `StructurePanel` にメタデータ表示カラム追加

### 2.1 アウトラインビュー (Corkboard)

| 項目 | 内容 |
|---|---|
| 優先度 | high |
| 新規ファイル | `src/panels/outlineview.h/cpp` |
| 変更ファイル | `src/mainwindow.h/cpp` |
| 依存 | 2.3 |

**実装内容**:
- `QGraphicsView` またはカスタムウィジェットでカード表示
- 各カード = エピソード1つ（タイトル、要約、メタデータアイコン）
- ドラッグ&ドロップで順序変更（`sortOrder` を更新）
- カードクリックでエディタに開く
- グリッド表示 / リスト表示の切り替え
- 色分け（シーン種別または感情レベル）

### 2.2 タイムラインパネル

| 項目 | 内容 |
|---|---|
| 優先度 | medium |
| 新規ファイル | `src/panels/timelinepanel.h/cpp` |
| 依存 | 2.1, 2.3 |

**実装内容**:
- 横軸=時間、縦軸=キャラクターのガントチャート風表示
- エピソードを時間帯に配置
- キャラクターの登場/退場をビジュアル表示
- 時間軸のズームイン/アウト
- 矛盾検出（同一キャラクターが同時刻に複数箇所に出現）

### 2.4 キャラクター相関図パネル

| 項目 | 内容 |
|---|---|
| 優先度 | medium |
| 新規ファイル | `src/panels/charactergraph.h/cpp` |
| 依存 | 2.3 |

**実装内容**:
- `QGraphicsScene` でノード（キャラクター）とエッジ（関係性）を描画
- ノードドラッグで配置変更
- エッジラベルで関係性説明（友人/敵対/恋愛 etc.）
- エピソード内で共演したキャラクター間に自動でエッジ生成
- クリックでキャラ詳細パネルを開く

---

## Phase 3: 推敲・バージョン管理

**目標**: AI出力のbefore/afterを追跡・比較可能に

### 3.1 リビジョン履歴システム

| 項目 | 内容 |
|---|---|
| 優先度 | high |
| 新規ファイル | `src/core/revisionmanager.ixx` |
| 変更ファイル | `src/core/novelproject.h/cpp` |
| モジュール | `PlotEngine.Core.RevisionManager` |
| 依存 | Phase 1 全体 |

**実装内容**:
- `Revision` 構造体: タイムスタンプ、内容、変更タイプ（手動/AI生成/インポート）、メモ
- `Episode` が `QVector<Revision>` を保持
- 保存時に自動リビジョン作成（内容が変更された場合）
- AI生成時は特別マーカー（`RevisionType::AiGenerated`）
- リビジョンの復元機能
- 古いリビジョンの自動パージ（直近50件保持等）

### 3.2 比較ビュー (diff表示)

| 項目 | 内容 |
|---|---|
| 優先度 | high |
| 新規ファイル | `src/editor/diffviewer.h/cpp` |
| 変更ファイル | `src/mainwindow.h/cpp` |
| 依存 | 3.1 |

**実装内容**:
- 2つのリビジョンを左右に並べて表示
- 差分をハイライト（追加=緑、削除=赤、変更=黄）
- 行単位/文字単位の切り替え
- 「この変更を適用」ボタン（部分的なマージ）
- `QSplitter` でペインサイズ調整
- キーボードショートカットでリビジョン間移動

### 3.3 グローバル検索/置換

| 項目 | 内容 |
|---|---|
| 優先度 | medium |
| 新規ファイル | `src/panels/searchpanel.h/cpp` |
| 変更ファイル | `src/mainwindow.h/cpp` |
| 依存 | なし（Phase 1-2 と独立） |

**実装内容**:
- 全エピソード、キャラクターノート、場所ノート横断検索
- 正規表現サポート
- 大文字小文字区別/あいまい検索オプション
- 検索結果リスト（クリックで該当エピソードを開く）
- 一括置換（確認つき）
- キャラクター名/場所名の一括変更リネーマー

---

## Phase 4: 書き出し・品質

**目標**: 作品を完成品として出力＋執筆習慣の可視化

### 4.2 自動保存システム

> 4.1 より先に実装。データ保護の基盤。

| 項目 | 内容 |
|---|---|
| 優先度 | high |
| 新規ファイル | `src/core/autosaver.ixx` |
| 変更ファイル | `src/mainwindow.h/cpp` |
| モジュール | `PlotEngine.Core.AutoSaver` |
| 依存 | 3.1 |

**実装内容**:
- `QTimer` で定期保存（デフォルト60秒、設定可能）
- コンテンツ変更時のみ保存（dirtyフラグチェック）
- 保存前にリビジョン作成（3.1 と連携）
- クラッシュリカバリ（起動時に未保存リビジョンを確認）
- バックアップディレクトリ（`.plotproj.bak/`）
- ユーザーに「最後の自動保存: XX秒前」をステータスバー表示

### 4.1 エクスポート (EPUB/PDF/テキスト)

| 項目 | 内容 |
|---|---|
| 優先度 | high |
| 新規ファイル | `src/export/exporter.h/cpp`, `src/export/epubexporter.ixx`, `src/export/pdfexporter.ixx`, `src/export/textexporter.ixx` |
| 変更ファイル | `src/mainwindow.h/cpp`（エクスポートメニュー） |
| モジュール | `PlotEngine.Core.Export.*` |
| 依存 | Phase 2 全体（メタデータ構造） |

**実装内容**:
- **テキスト**: 章→エピソード順に連結、区切り文字挿入、UTF-8
- **EPUB**: 目次自動生成、チャプター分割、メタデータ注入、表紙サポート
- **PDF**: Qt の `QPrinter` / `QPdfWriter` でレンダリング、フォント埋め込み（日本語対応）
- エクスポート設定ダイアログ（範囲選択、フォーマット、出力先）
- プレビュー機能

### 4.3 執筆アナリティクスダッシュボード

| 項目 | 内容 |
|---|---|
| 優先度 | low |
| 新規ファイル | `src/panels/analyticspanel.h/cpp` |
| 依存 | 4.2 |

**実装内容**:
- 日次ワードカウント（棒グラフ）
- 執筆セッションの記録（開始/終了時間、集中時間）
- 目標達成率（目標文字数 vs 実績）
- キャラクター出現頻度分析
- 感情カーブの推移（ストーリー全体の起伏）
- 連続執筆日数（ストリーク）
- データはローカルSQLiteまたはJSONで保存

---

## 技術方針

### モジュール境界
- `PlotEngine.Core.*` — ドメインロジック、シリアライズ、AIインターフェース
- `PlotEngine.Editor.*` — エディタコンポーネント（今後のモジュール化対象）
- `PlotEngine.Export.*` — エクスポートロジック
- UIコード（`QWidget` 派生）は当面ヘッダー/ソースのまま

### 依存関係
- Qt6 + ADS は変更なし
- AI通信: `QNetworkAccessManager`（追加依存なし）
- EPUB: ZIP圧縮が必要 → `minizip` を vcpkg で追加検討
- PDF: `QPdfWriter`（Qt標準）
- グラフ表示: カスタム `QGraphicsScene` で実装、外部依存なし

### 設定管理
- `QSettings` で一元管理
- 暗号化が必要な項目（APIキー等）は簡易難読化またはOSのキーチェーン検討

### テスト方針
- ドメインロジック（モジュール）はユニットテスト対象
- `ProjectManager`, `ContextBuilder`, `TextUtils` を優先
- UIテストは後回し（手動QAでカバー）

---

## リスクと対策

| リスク | 対策 |
|---|---|
| AI APIのレートリミット | リトライ＋エクスポネンシャルバックオフ、キュー管理 |
| 大規模プロジェクトのメモリ使用量 | エピソードの遅延読み込み、コンテンツのオンデマンドロード |
| C++20 Modules + Qt の相性 | Verdigris を活用、Qtヘッダーはモジュール外に保持 |
| EPUB生成の複雑さ | 最初はシンプルテキスト連結から始め、段階的にEPUB対応 |
| 日本語PDFの文字化け | フォント埋め込み必須、Noto Sans JP をバンドル検討 |
