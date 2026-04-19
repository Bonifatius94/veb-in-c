---
name: refine
description: Turn a GitHub issue into a concrete implementation plan for the veb-in-c codebase. Given an issue number, fetch the issue, map it onto the current code, and produce a step-by-step plan with affected files, invariants to preserve, and tests to add. Use when the user wants to start work on a tracked item from ROADMAP.md or the issue tracker.
---

# /refine

Takes a GitHub issue number (e.g. `/refine 3` for lazy allocation), promotes a raw-draft issue into a refined plan detailed enough for an agent to execute autonomously, and replaces the issue body with it. It does **not** write code.

Issues in this repo live in one of two states:

- **Raw draft** — the original idea, sometimes terse, sometimes a brain-dump. Not necessarily actionable.
- **Refined plan** — the issue body, rewritten to the template below, ready to hand to an autonomous agent.

`/refine` is what moves an issue from the first state to the second.

## Steps

1. Fetch the issue with `gh issue view <N>`. Read the body and any comments.
2. Read `CLAUDE.md` for architecture, coding standards, and the list of invariants that must be preserved.
3. Read `ROADMAP.md` to understand how this issue relates to the others — especially dependencies and suggested ordering.
4. Read `include/vebtrees.h`. Grep for any `TODO` comments or pre-existing scaffolding relevant to the issue (e.g. `VEBTREE_FLAG_LAZY` for #3, the `assert(false)` stub in `vebtree_predecessor` for #15).
5. Read `test/unit_tests.c` and `test/sorting_benchmark.c` to understand current coverage around the affected surface.
6. Produce the plan (see template below) and write it to `./plans/<N>.md` at the repo root so the user can review it on disk. The `plans/` directory is gitignored — it's a local scratch area, not versioned state.
7. Show the plan to the user and point them at `./plans/<N>.md`. Iterate on that file until they approve. Then **replace the issue body** with it: `gh issue edit <N> --body-file ./plans/<N>.md`. The refined plan *is* the new issue body — not a comment on top of the draft. Leave `./plans/<N>.md` in place after posting; the user may want to reference it during `/implement`. Do not save plans anywhere else (no hidden memory, no scratch files outside `plans/`).

## Output template

```
## /refine #<N> — <issue title>

**Goal.** <one-sentence restatement in the project's vocabulary>

**Affected code.**
- include/vebtrees.h:<line> — <what changes here>
- test/unit_tests.c:<line> — <what to add>
- (new) <path> — <purpose, if adding files>

**Approach.**
- <design bullet>
- <design bullet>
- ...

**Invariants to preserve** (from CLAUDE.md):
- <e.g. "low of an internal node is never present in any subtree">
- ...

**Test plan.**
- <unit test name> — <what it covers>
- <bench or integration change, if applicable>

**Risks / open questions.**
- <anything ambiguous the user should decide before coding>

**Dependencies** (per ROADMAP.md):
- Must land first: <issue numbers or "none">
- Unblocks: <issue numbers or "none">
```

## Rules

- Do **not** start implementing. This skill ends once the refined plan replaces the issue body; the user approves, redirects, or edits before any code is written.
- The refined plan is the new issue body — overwrite, don't append as a comment. Raw-draft content is superseded, not preserved alongside.
- The ticket is the canonical home of the plan; `./plans/<N>.md` is a local review copy (gitignored). Keep them consistent: if the user edits the local file after posting, re-run `gh issue edit <N> --body-file ./plans/<N>.md`. Never store plans in hidden memory or anywhere outside `plans/`.
- The plan should be detailed enough that an agent picking up the issue cold can execute it without further clarification.
- If the issue is genuinely ambiguous, prefer surfacing open questions to guessing — the plan is cheap to iterate on.
- Prefer concrete file:line references over vague pointers. Don't say "the init function" when you can say "`_vebtree_init` at vebtrees.h:352".
- Keep the plan short. A good plan is 30–60 lines; a plan that runs multiple pages usually means the issue needs to be broken up first.
