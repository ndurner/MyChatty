# Code Interpreter Notes

MyChatty's local Code Interpreter uses Qt `QJSEngine`.

## Runtime

The model-facing tool is `eval_javascript`. It evaluates short JavaScript snippets for arithmetic, JSON, date handling, string processing, and small table work. The runtime exposes:

- `print(...)` for tool output.
- `fs` for conversation-scoped file access.

It does not expose arbitrary host filesystem, process, Python, shell, or network APIs.

## Conversation file store

Code Interpreter files live under `QStandardPaths::AppDataLocation` in a per-conversation directory:

- `uploads/` contains copies of user-provided files.
- `tmp/` contains script-generated scratch files.
- `repos/` is reserved for future Git/repository inspection helpers.

The script API uses virtual paths such as `/uploads/data.csv` and `/tmp/result.txt`; host paths are not part of the model contract.

## Guardrails

`QJSEngine::evaluate()` is guarded by a watchdog that interrupts long-running scripts. File reads and writes are size-limited, writes are constrained to `tmp/`, and path resolution rejects traversal outside the conversation root.
