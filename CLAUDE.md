# CLAUDE.md

Behavioral guidelines to reduce common LLM coding mistakes. Merge with project-specific instructions as needed.

**Tradeoff:** These guidelines bias toward caution over speed. For trivial tasks, use judgment.

## 1. Think Before Coding

**Don't assume. Don't hide confusion. Surface tradeoffs.**

Before implementing:
- State your assumptions explicitly. If uncertain, ask.
- If multiple interpretations exist, present them - don't pick silently.
- If a simpler approach exists, say so. Push back when warranted.
- If something is unclear, stop. Name what's confusing. Ask.

## 2. Simplicity First

**Minimum code that solves the problem. Nothing speculative.**

- No features beyond what was asked.
- No abstractions for single-use code.
- No "flexibility" or "configurability" that wasn't requested.
- No error handling for impossible scenarios.
- If you write 200 lines and it could be 50, rewrite it.

Ask yourself: "Would a senior engineer say this is overcomplicated?" If yes, simplify.

## 3. Surgical Changes

**Touch only what you must. Clean up only your own mess.**

When editing existing code:
- Don't "improve" adjacent code, comments, or formatting.
- Don't refactor things that aren't broken.
- Match existing style, even if you'd do it differently.
- If you notice unrelated dead code, mention it - don't delete it.

When your changes create orphans:
- Remove imports/variables/functions that YOUR changes made unused.
- Don't remove pre-existing dead code unless asked.

The test: Every changed line should trace directly to the user's request.

## 4. Goal-Driven Execution

**Define success criteria. Loop until verified.**

Transform tasks into verifiable goals:
- "Add validation" → "Write tests for invalid inputs, then make them pass"
- "Fix the bug" → "Write a test that reproduces it, then make it pass"
- "Refactor X" → "Ensure tests pass before and after"

For multi-step tasks, state a brief plan:
```
1. [Step] → verify: [check]
2. [Step] → verify: [check]
3. [Step] → verify: [check]
```

Strong success criteria let you loop independently. Weak criteria ("make it work") require constant clarification.

---

**These guidelines are working if:** fewer unnecessary changes in diffs, fewer rewrites due to overcomplication, and clarifying questions come before implementation rather than after mistakes.

---

## 项目记忆（Project Memory）

本仓库的项目记忆、工具链命令、RTX51 Tiny / SC95F7617B 硬核事实与工作约定，统一维护在
**`AGENTS.md`**（会自动加载）。动手前请先读它，尤其是：

- 仓库里**并排 4 套重叠代码**的归属区分（`docs/CODE_FLOW.md` §1）；
- `soc/**/*.c|*.H` 为 **GBK**，用 `grep` 工具会失败，需用 PowerShell `Select-String -Encoding Default`；
- RTX51 Tiny：Timer0 被内核独占（`interrupt 1`/`0x0B`）、内核变量占用寄存器组 1（用户中断禁 `using 1`）、
  `os_wait` 的 `K_TMO`/`K_IVL` 差异、多任务共享非重入函数会触发 `L15`；
- 交付产物与完整使用手册见 `soc/RTX51_Tiny_Demo/`（`USAGE.md` 为 RTX51 Tiny 使用手册）。
