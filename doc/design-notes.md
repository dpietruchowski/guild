# Design notes

Working notes, not decisions. Everything here is provisional and some of it will
turn out wrong. The measurements these lean on are in
[cli-context.md](cli-context.md).

## Files, not a database

Transcripts, memory and agent definitions as plain files on disk. Readable with
`cat`, versioned with git, and a compaction shows up as a diff. SQLite stays on
the table as a search index later, if plain files stop being enough — not as the
source of truth.

## One directory per agent

```
workspace/
  guild.toml                    roster: who exists, who reports to whom
  skills/                       shared pool
    format-code/SKILL.md
    review-cpp/SKILL.md
  agents/
    john/
      CLAUDE.md                 identity and system prompt
      .claude/skills/           john's own skills
      memory/
        MEMORY.md               index, always loaded
        prefers-ninja.md        one fact per file
      transcript/
        0000.jsonl              append-only segments
        0001.jsonl
        HEAD                    live segment number + compaction summary
      mailbox/
        inbox.jsonl             envelopes from other agents, later
```

The directory *is* the agent's identity. Measured: switching the working
directory switches which `CLAUDE.md` and which skills the CLI assembles, and two
agent directories never see each other's.

### Skills: shared pool, per-agent selection

A skill is a capability, not property of one agent. Three agents needing
`format-code` should not mean three copies that drift apart. So a shared
`skills/` at workspace level, plus `agents/<name>/.claude/skills/` for what is
genuinely one agent's own. Name resolution: the agent's own directory wins, so an
override costs nothing.

An agent should get only the skills it declares, not the whole pool, because the
skill listing is re-sent with every request. Something like:

```markdown
---
model: sonnet
skills: [format-code, review-cpp]
---
```

How a shared pool reaches the CLI is unsolved — the CLI discovers skills under
the working directory, so a shared pool means symlinks, copies, or `--plugin-dir`
(untested).

## Transcript in segments

Compaction never rewrites. It closes the live segment, writes a summary into
`HEAD`, and opens the next one. Old segments stay untouched.

This gives three things from one mechanism:

- short-term memory — the live segment
- compaction — the summary in `HEAD`
- conversation history — the old segments, which nobody reads until asked

Starting without history costs nothing: one segment, no compaction, empty `HEAD`.
History arrives later by simply not deleting old segments, with no format change.

## Own prompt, borrowed file formats

Split the difference: write the system prompt from scratch, since the built-in
one carries a great deal that is irrelevant to a purpose-built agent, but keep
`SKILL.md`-with-frontmatter as the skill format. It is only a file convention, it
costs nothing, and the model has seen it.

One trap: naming the agent's prompt file `CLAUDE.md` means the CLI discovers it
by itself. That is either exactly what is wanted or a double injection, depending
on which isolation approach below wins.

## Isolation: flags or a container

The problem: an agent should get its own directory and nothing else. Measurements
say flags cannot deliver that. `--setting-sources 'project'` assembles the
agent's memory and skills but also drags in `~/.claude/CLAUDE.md` and every
`CLAUDE.md` found walking up the tree, including across a git boundary.
`--setting-sources ''` assembles nothing at all. There is no middle setting, and
relocating `HOME` or `CLAUDE_CONFIG_DIR` breaks auth, because credentials live
there too.

A container inverts the problem instead of fighting it. Rather than subtracting
what the CLI adds, arrange for there to be nothing to add:

- empty `HOME` in the container, so no global `CLAUDE.md` exists
- nothing above `/work`, so the upward walk finds nothing
- default `--setting-sources`, letting the CLI assemble `/work/CLAUDE.md` and
  `/work/.claude/skills/` on its own
- `--dangerously-skip-permissions` stops being a risk once the container is the
  sandbox
- mount only the agent's directory and whatever project it works on

The whole flag matrix then becomes unnecessary; `--output-format stream-json` is
the only one left that matters.

Three things to know going in:

1. **The token still crosses the boundary.** A container does not remove the auth
   problem. `claude setup-token` produces a long-lived subscription token,
   injected as an environment variable — the one thing that must get inside.
2. **The network stays open** to the Anthropic API, so this is not network
   isolation.
3. **Container startup costs.** Invoking the binary once per turn adds that cost
   every turn. One long-lived container per agent with `docker exec` per turn
   avoids it — and fits the model anyway, since the agent already owns a
   directory and a lifetime.

Point 3 argues for the container rather than against it: it becomes the natural
boundary of an agent.

## Open

- Shared skill pool into a working directory: symlinks, copies, or
  `--plugin-dir`? The last is untested.
- Whether the agent's prompt file should be named `CLAUDE.md` (CLI assembles it)
  or something else (the caller assembles it). Follows from the isolation choice.
- Compaction trigger, memory retrieval, parallelism, failure semantics — see the
  open questions in [idea.md](idea.md).
