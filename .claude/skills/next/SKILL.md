---
name: next
description: Generate a self-contained handoff prompt for the next Claude session to continue this session's work. Captures issue context, branch state, what's done, what's left, and gotchas. The user copies the output and pastes it as the opening prompt of the next session. Use at the end of a work session — especially when stopping mid-task.
---

# /next

Produces a handoff prompt that the user pastes into the next session. The next session should be able to pick up cold, without re-exploring the repo or re-reading the whole conversation.

## Output destination

**Print the prompt to the terminal.** Do not write it to a scratch file in the repo. Do not write it to hidden memory. Do not append it to the issue body (the issue body is the refined plan; handoffs are ephemeral session state and belong to the user).

Wrap the prompt in a single fenced code block so the user can copy it cleanly.

## What to gather (before writing)

Run these in parallel and pull together the facts. Never fabricate — if you don't know something, say "unknown" in the handoff so the next session investigates rather than assumes.

- `git status` — dirty files, untracked files
- `git log --oneline main..HEAD` (or `@{u}..HEAD` if tracking a remote) — commits made this session
- `git branch --show-current` — current branch
- `git diff --stat` — scope of uncommitted changes
- If the work is tied to an issue: `gh issue view <N>` for title and refined plan. Ask the user for the number if you don't already know it from the session.
- Last build/test status from the conversation — did `/build` pass most recently? If unclear, flag it.

## Output template

```
You are continuing work on veb-in-c (van Emde Boas tree, single-header C89). Start by reading CLAUDE.md and ROADMAP.md for invariants, coding standards, and the project's direction.

## Context
- Issue: #<N> — <title>  (https://github.com/Bonifatius94/veb-in-c/issues/<N>)
- Branch: <branch-name>
- Goal: <one-sentence restatement of what we are trying to accomplish>

## State at handoff
- Commits this session: <short-hash subject, one per line, or "none yet">
- Uncommitted changes: <file list with +/- line counts, or "working tree clean">
- Last build result: <green on <commit> / red: <failure summary> / not yet run>
- Tests: <N/N passing / which ones failing / not yet run>

## What's left
- <concrete next step, referencing file:line where possible>
- <concrete next step>
- ...

## Gotchas discovered this session
- <anything surprising that isn't in CLAUDE.md yet — e.g. a subtle invariant, a confusing call site, a tooling quirk>
- (omit the section entirely if nothing to report)

## Where to start
1. `git checkout <branch-name>`
2. `/build` to confirm starting state matches "Last build result" above
3. <specific first action, e.g. "open vebtrees.h:370 and continue the lazy-alloc branch in _vebtree_init">
```

## Rules

- **Be specific.** "Continue the lazy allocation" is useless; "vebtrees.h:370, finish the `if (vebtree_is_lazy(tree)) return;` branch by implementing on-demand `_init_subtrees` inside insert/delete" is useful.
- **Include file:line references.** The next session shouldn't have to re-grep to find where you left off.
- **Don't paraphrase CLAUDE.md or ROADMAP.md.** Point the next session at them. They'll be read fresh.
- **Flag uncertainty honestly.** If the last build was red and you didn't finish diagnosing, say so. A handoff that pretends everything is green is worse than no handoff.
- **Don't commit the handoff, don't post it anywhere.** It's a prompt for the user to paste into their next session. That's it.
