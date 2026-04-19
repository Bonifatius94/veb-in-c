---
name: implement
description: Pick up a refined GitHub issue and implement it end-to-end using strict red-green-refactor TDD. Creates a branch, cycles through Red/Green/Refactor with one commit per phase, and stops at a green local build. Use when the user hands you a ticket number that has already been through /refine.
---

# /implement

Takes a GitHub issue number whose body is a **refined plan** (see `/refine`) and executes it using **strict red-green-refactor TDD**. Every non-trivial change goes through all three phases, in order, with an explicit gate between each. The agent may not skip a phase, combine phases in one commit, or proceed past a gate without satisfying it.

## Why the rigid structure

This project is a single-header data-structure library where regressions are silent — a subtle bug in `successor`/`delete` won't crash the build, it'll just return wrong answers. TDD is the main defense:

- **Red first** forces the test to actually run and actually fail for the right reason, proving the test can detect the bug. Tests added after the fix often pass even without the fix — they're worthless.
- **Green with minimum code** keeps the diff small and reviewable, and prevents the agent from smuggling in unrelated "improvements" under the cover of a real fix.
- **Refactor under a green bar** means cleanup is mechanically safe — the tests are the safety net.
- **One commit per phase** makes the history a literal record of the methodology. A reviewer (or future agent) can see the failing test, the fix, and the cleanup separately.

If you find yourself wanting to skip or merge phases, stop and re-read this section. The methodology is the point.

## Preconditions

- The issue body must be a **refined plan**, not a raw draft. Refined plans follow the template from `/refine` (Goal / Affected code / Approach / Invariants / Test plan / Risks / Dependencies).
- If the issue is still a raw draft, **stop** and tell the user to run `/refine <N>` first. Do not start guessing at a plan from a one-line draft.
- Check the plan's **Dependencies** section. If it lists blocker issues that aren't done yet, stop and flag them.

## Setup (before the RGR loop)

1. **Fetch and validate the plan.** `gh issue view <N>`. Confirm it matches the refined-plan template. Bail with a clear message if it doesn't.
2. **Read the guardrails.** `CLAUDE.md` (invariants, coding standards, gotchas) and `ROADMAP.md` (context for where this fits). Do this every run — do not assume memory of them.
3. **Check working tree is clean.** `git status`. If there are uncommitted changes unrelated to this issue, stop and ask the user what to do.
4. **Work on `main` directly — do not create a feature branch.** This is a solo-dev project and the RGR commit sequence (`red:` / `green:` / `refactor:`) is the review trail; PRs and feature branches add no value. The red/green/refactor prefixes already make the methodology legible in the linear history.

If the plan's "Test plan" section is empty or vague, **stop** — you can't run TDD without a concrete test. Go back and `/refine` the issue.

## The RGR loop

The plan's **Test plan** section is a list of test cases. For each test case, run the full Red → Green → Refactor cycle. Do not batch: finish one cycle (commit the refactor) before starting the next.

---

### Phase 1 — RED

**Goal.** Add *exactly one* failing test that pins down the behaviour the plan says is missing or wrong.

**Reason.** A test that has never failed cannot be trusted to catch regressions. Writing the test first, and seeing it fail in the way you predicted, is the only proof that the test is connected to the behaviour you care about.

**Steps.**

1. Add the test to `test/unit_tests.c` (or a new file wired into `test/CMakeLists.txt`). Match the existing assertion style.
2. Wire the test into `main()` so CTest picks it up.
3. Run `./build.sh`.

**Gate — all must hold before moving on:**

- [ ] The build compiles. A compilation failure is not a red test — fix it and re-run.
- [ ] The new test runs and fails. `assert` aborts the binary and CTest reports the failing target.
- [ ] The failure mode matches what you predicted. If the test fails for a different reason (e.g. segfault when you expected a wrong return value), stop and investigate — the test is measuring the wrong thing.
- [ ] No other existing test broke. If something unrelated went red, you wrote too much too fast; revert and try again with a narrower test.

**Commit.** `red: <one-line test description> (#<N>)` — include only the test file(s) and any minimal scaffolding needed for them to compile.

**Bail conditions.** If you cannot write a test that fails for the right reason — because the behaviour is impossible to observe from outside, or the setup is too involved — stop and report. The plan likely needs to expose a narrower seam first.

---

### Phase 2 — GREEN

**Goal.** Write the **smallest** change to `include/vebtrees.h` (or wherever the implementation lives) that turns the red test green, without breaking any other test.

**Reason.** "Smallest" is a constraint, not a suggestion. It forces the commit to contain only the logic the test actually demands, which (a) makes the diff reviewable, (b) keeps refactoring and feature work separate, and (c) prevents the agent from dragging in unrelated changes that weren't in the plan.

**Steps.**

1. Change only the code that *must* change for the new test to pass.
2. Respect every invariant from `CLAUDE.md` §Invariants. A green test is not a licence to violate an invariant.
3. Respect C89 style: declarations at top of block, `/* */` comments only, no mixed decls/statements, no new external dependencies.
4. Run `./build.sh`.

**Gate — all must hold before moving on:**

- [ ] The new test passes.
- [ ] All previously passing tests still pass.
- [ ] The diff for this phase is minimal — if you're tempted to justify "while I'm here, I also cleaned up X", stop. That belongs in Refactor.
- [ ] No CLAUDE.md invariant was traded away for the green bar.

**Commit.** `green: <one-line behaviour description> (#<N>)` — include only the implementation change.

**Bail conditions.** If making the test green requires a structural change (new helper, new type, new allocation path) that wasn't in the plan's "Approach" section, stop. Either the plan is underspecified or the approach is wrong; don't improvise.

---

### Phase 3 — REFACTOR

**Goal.** Improve the code's internal quality — clarity, duplication, naming — without changing its externally observable behaviour.

**Reason.** Without a dedicated refactor step, green commits accumulate duplication and confusing names that nobody ever cleans up. The refactor commit is the only place in the cycle where cleanup is both safe (tests are green) and authorised (scope is explicit).

**Steps.**

1. Identify one concrete improvement made possible by the new code — e.g. a helper that now has three similar callers, a magic constant that deserves a name, a comment that became wrong.
2. Apply it. Keep the change mechanical; no new behaviour, no new tests.
3. Run `./build.sh`.

**Gate — all must hold before moving on:**

- [ ] All tests (old and new) still pass.
- [ ] No new test was added. (New behaviour → new Red phase.)
- [ ] The diff only touches internal structure, not observable behaviour.
- [ ] You did not "refactor" by deleting or weakening tests.

**Commit.** `refactor: <one-line change description> (#<N>)`.

**If there is genuinely nothing to refactor** — e.g. the green change was a single line that doesn't create duplication or obscure names — record that explicitly in the report back to the user, but **do not fabricate a refactor commit**. A skipped-because-unnecessary refactor is fine; a made-up one is noise.

---

## After the loop

1. **Report back.** The red/green/refactor commit sequence (with short hashes and subjects) on `main`, test status, any deviations from the plan and why.
2. **Stop.** Do not push, do not open a PR, do not close the issue. Those require explicit user confirmation. This is a solo-dev repo — PRs are not part of the workflow.

## Rules (apply throughout)

- **The plan is the contract.** If reality doesn't match the plan — the approach is wrong, an invariant is at risk, a test case reveals the plan's wrong — **stop and report**. Update the issue body via `/refine` rather than silently deviating.
- **One phase per commit. Non-negotiable.** Don't combine red+green "because the test was trivial". Don't bundle refactor into green "to save a commit". The separation is the methodology.
- **Never skip hooks or signing.** If a pre-commit hook fails (including the `Co-Authored-By` check), fix the underlying issue, don't bypass.
- **Never force-push, reset --hard, or delete branches** without explicit user instruction.
- **No scope creep.** If you find an unrelated bug or TODO, note it in the report but do not fix it here — open a separate issue.

## When to bail

- Plan is still a raw draft → tell the user to `/refine <N>` first.
- Plan lists unmet dependencies → surface them, stop.
- Plan's Test plan section is empty or too vague for RGR → `/refine` first.
- Working tree is dirty → ask the user.
- A Red phase gate can't be satisfied (test won't fail for the right reason) → stop; the plan needs a narrower seam.
- A Green phase needs structural work not in the plan → stop; the plan is underspecified.
- Build goes red in an unrelated way you can't diagnose quickly → stop, show the failure, ask.
- An invariant from CLAUDE.md would have to be violated to follow the plan → stop; the plan itself needs revision.
