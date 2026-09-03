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
~/.guild/                       GUILD_HOME, overridable for tests
  CLAUDE.d/                     shared prompt fragments, pulled in by @import
  .claude/skills/               shared skill pool, reached with --add-dir
    format-code/SKILL.md
    review-cpp/SKILL.md
  agents/
    john/                       mounted at /work
      CLAUDE.md                 identity; the CLI assembles it itself
      .claude/
        settings.json           model, permissions
        agents/                 john's own subagents
        skills/                 john's own skills
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

The directory *is* the agent's identity, and it is an ordinary Claude Code
project. Measured: switching the working directory switches which `CLAUDE.md` and
which skills the CLI assembles, and two agent directories never see each other's.

No file here is a Guild format. Every one of them is a file the CLI already knows
how to read, which also means the agent can edit its own configuration with the
tools it already has.

There is no central roster file. The list of agents is the list of directories
under `agents/`, so two agents creating a third at the same time cannot corrupt
a shared file.

### Model

The `model` key in the agent's `.claude/settings.json`. That file sits inside the
mounted directory, so the CLI reads it as project settings and Guild passes no
flag at all. `--model` overrides it per invocation if a caller ever needs to, and
`ANTHROPIC_MODEL` overrides both.

### Skills: shared pool, per-agent selection

A skill is a capability, not property of one agent. Three agents needing
`format-code` should not mean three copies that drift apart. So a shared pool at
`~/.guild/.claude/skills/`, plus `agents/<name>/.claude/skills/` for what is
genuinely one agent's own.

The shared pool reaches the CLI through `--add-dir`. Normally that flag grants
file access rather than configuration discovery, but skills and commands are an
explicit exception: Claude Code loads `.claude/skills/` and `.claude/commands/`
from every added directory. So the pool gets mounted read-only next to the agent
and named with `--add-dir`. No symlinks, no copies, no `--plugin-dir`.

Selecting a subset per agent is still open. Skill metadata is re-sent with every
request, so a large pool is not free — but `--add-dir` is all-or-nothing, and
per-skill selection exists only in a subagent's `skills:` frontmatter.

## Transcript in segments

Compaction never rewrites. It closes the live segment, writes a summary into
`HEAD`, and opens the next one. Old segments stay untouched.

This gives three things from one mechanism:

- short-term memory — the live segment
- compaction — the summary in `HEAD`
- conversation history — the old segments, which nobody reads until asked

Starting without history costs nothing: one segment, no compaction, empty `HEAD`.
History arrives later by simply not deleting old segments, with no format change.

## Borrowed formats, borrowed mechanisms

Earlier plan was to invent an `agent.md` carrying both identity and plumbing.
Dropped. The agent directory is a Claude Code project instead, and every format
in it is one the CLI already reads:

- `CLAUDE.md` — the identity. The CLI discovers it under the working directory,
  which is exactly what is wanted once the container guarantees nothing else is
  discoverable. `@path` imports come for free, so `CLAUDE.d` needs no mechanism
  of its own.
- `.claude/settings.json` — model and permissions. JSON, which Qt parses without
  a dependency; the TOML-or-YAML question disappears with it.
- `.claude/skills/<name>/SKILL.md` — `name` and `description` required,
  `allowed-tools` and `disable-model-invocation` optional. Loading is staged:
  metadata always, body on trigger, bundled files only when read.
- `.claude/agents/<name>.md` — the agent's own subagents. `name` and
  `description` required; the body replaces the system prompt rather than
  extending it.

Two levels of delegation then coexist without competing. A subagent is a helper
that dies with the turn that spawned it. A Guild agent outlives the invocation
and owns a transcript and a mailbox. Borrowing the file format does not mean
borrowing the lifetime.

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
   problem. `claude setup-token` produces a year-long subscription token
   (`sk-ant-oat01-…`), injected as `CLAUDE_CODE_OAUTH_TOKEN` — the one thing that
   must get inside. That variable takes precedence over the keychain, so it
   belongs in a file read at launch and passed to the container, never exported
   into a shell where it would silently change how `claude` authenticates
   everywhere else.
2. **The network stays open** to the Anthropic API, so this is not network
   isolation.
3. **Container startup costs.** Invoking the binary once per turn adds that cost
   every turn. One long-lived container per agent with `docker exec` per turn
   avoids it — and fits the model anyway, since the agent already owns a
   directory and a lifetime.

Point 3 argues for the container rather than against it: it becomes the natural
boundary of an agent.

## Nothing left to configure

Once the formats are borrowed and the paths come from `Workspace` and
`AgentDirectory`, the run recipe has no configuration file behind it:

| what `docker run` needs | where it comes from |
|---|---|
| image | `GUILD_IMAGE`, defaulting to `guild-agent:latest` |
| container name | derived, `guild-<agent>` |
| mount at `/work` | `AgentDirectory::path()` |
| shared skills, read-only | `Workspace::sharedSkillsPath()` |
| `--add-dir` | that same path inside the container |
| working directory | fixed, `/work` |
| `HOME` | fixed, an empty directory |
| token | `CLAUDE_CODE_OAUTH_TOKEN`, passed through |
| detach, removal, limits | policy in code |
| model, permissions, skills, subagents | the agent's own `.claude/`, which Guild never reads |

Two environment variables and two classes. A `guild.json` was drafted for the
image and the mounts and then turned out to carry nothing that is not either
derivable or global.

One thing genuinely resists this: **which project an agent works on**. It is the
only per-agent fact with no place to live. Deferred rather than solved — an agent
gets its own directory and nothing else until there is an agent with real work to
do. When that comes, the answer in keeping with everything above is a symlink,
`agents/john/projects/guild`, since that is configuration through the file system
again.

## Open

- Whether Docker will bind-mount onto a path that is a symlink inside the mounted
  directory, which decides whether the project symlink above can work at all.
- Giving an agent a subset of the shared pool rather than all of it, given that
  `--add-dir` is all-or-nothing.
- Whether `@path` imports resolve across a read-only mount and outside the
  project directory. Untested.
- Compaction trigger, memory retrieval, parallelism, failure semantics — see the
  open questions in [idea.md](idea.md).
