# What the CLI actually puts in the context

Measurements against the real `claude` binary, to find out what a stateless
invocation drags in and which flags remove it.

**Environment.** Claude Code `2.1.259`, Linux, subscription auth (no
`ANTHROPIC_API_KEY`). Probe model `sonnet` for every measurement, so the numbers
are comparable to each other but not to another model or another machine — the
user-level config here contributes its own skills and `CLAUDE.md`.

## Method

A throwaway project in `tmp/probe/`:

- `CLAUDE.md` containing `MAGIC_TOKEN is ZQ7-KOALA`
- `.claude/skills/probe-skill/SKILL.md`
- `.claude/agents/probe-agent.md`

The probe asks the model to report what is literally present in its context and
returns it through `--json-schema`, so the answer is parseable rather than prose:

```
claude -p --model sonnet --no-session-persistence --output-format json \
  --json-schema "$SCHEMA" <flags under test> < question.txt
```

Prompt size is read from `usage.cache_creation_input_tokens +
cache_read_input_tokens + input_tokens` in the result envelope. Nothing is
written to the real environment: `--no-session-persistence` keeps these runs out
of `~/.claude/projects/`.

## The session log is not the request

Before measuring, a look at `~/.claude/projects/*/*.jsonl`. It is a log of what
changed, not a copy of what was sent.

| | in the `.jsonl`? |
|---|---|
| system prompt | no |
| `CLAUDE.md`, any level | no, injected at request time |
| tool schemas | no |
| skill listing | yes, `attachment` record, once per session |
| subagent listing, deferred tool names | yes, `attachment` records |
| `nested_memory` | only when a `CLAUDE.md` appears or changes mid-session |

`nested_memory` is worth spelling out because it is easy to misread. Across ~200
sessions of one project it occurs 7 times, and in every case deep inside the
session — lines 85, 362, 479, 690, 852 of files thousands of lines long, never at
the start. It is a mid-session delta, not the initial memory load.

Consequence: a request cannot be reconstructed from the log alone.

## Which flag removes `CLAUDE.md`

The intuitive guess is wrong. `--system-prompt` replaces the system prompt but
leaves memory injection untouched — memory rides along in the first user
message, not in the system prompt.

| flags | `MAGIC_TOKEN` visible |
|---|---|
| *(none)* | yes |
| `--system-prompt` | **yes** |
| `--setting-sources ''` | no |
| `--setting-sources ''` + `--system-prompt` | no |
| `--setting-sources 'user,project,local'` + `--system-prompt` | yes |

`--setting-sources ''` is the switch, and it works independently of the system
prompt. A side effect confirms it fires: the sandbox warning printed by the
loaded settings disappears.

## Flag matrix

Two rounds, same probe. `skills` is the model's own count and carries about ±1 of
noise; treat it as approximate. `proj md` is the project `CLAUDE.md`, `global md`
the user-level one.

| variant | tokens | proj md | global md | probe skill | skills | probe agent | tools |
|---|---|---|---|---|---|---|---|
| default | 44281 | yes | yes | yes | 18 | yes | 28 |
| `--system-prompt` | 35238 | yes | — | yes | 19 | yes | 28 |
| `--append-system-prompt` | 44421 | yes | — | yes | 19 | yes | 28 |
| `--setting-sources ''` | 42381 | no | no | no | 17 | no | 28 |
| `--setting-sources ''` + `--system-prompt` | 33338 | no | — | no | 17 | no | 27 |
| `--setting-sources 'project'` | 44233 | yes | yes | yes | 18 | yes | 27 |
| `--setting-sources 'user'` | 43168 | no | yes | no | 18 | no | 28 |
| `--disable-slash-commands` (+ empty sources, custom prompt) | 35922 | no | — | no | **0** | no | 11 |
| `--strict-mcp-config` (+ empty sources, custom prompt) | 33178 | no | — | no | 17 | no | 27 |
| `--exclude-dynamic-system-prompt-sections` | 42242 | no | — | no | 17 | no | 28 |
| `--tools Read Bash` (+ empty sources, custom prompt) | **6173** | no | no | no | 0 | no | 3 |
| `--tools ''` (+ empty sources, custom prompt) | **1137** | no | no | no | 0 | no | 1 |

Notes on individual rows:

- **`--setting-sources`** selects which layers load. `'project'` keeps the
  project `CLAUDE.md`, its skills and its agents; `'user'` keeps only the
  user-level ones. Empty removes all of it.
- **`--tools` is variadic.** `--tools Read,Bash "$PROMPT"` swallows the prompt as
  another tool name and the CLI then errors with *Input must be provided either
  through stdin or as a prompt argument*. Pass the prompt on stdin, or put
  `--tools` last with space-separated names.
- **`--disable-slash-commands`** is the only thing that zeroes the skill listing.
  `--setting-sources ''` merely drops the user and project skills; the built-in
  ones stay.
- **`StructuredOutput`** is always present when `--json-schema` is used — that is
  the tool the model answers through, which is why the `--tools ''` row still
  reports one tool.

Two results did not come out as expected and are recorded unexplained rather than
smoothed over:

- `--setting-sources 'project'` still reported the user-level `CLAUDE.md` as
  present, although `--setting-sources ''` did not. Memory discovery may not map
  cleanly onto setting layers.
- `--disable-slash-commands` measured *larger* than the same command without it
  (35922 vs 33338) despite dropping 17 skills and 16 tools. Either measurement
  noise or something else grows to compensate.

## Where the tokens actually go

Subtracting adjacent rows gives the cost of each layer:

| layer | tokens | share of default |
|---|---|---|
| tool schemas (full set vs `Read`+`Bash`) | ~27,200 | 61% |
| built-in system prompt (vs a one-line custom one) | ~9,000 | 20% |
| `Read` + `Bash` schemas alone | ~5,000 | 11% |
| settings: `CLAUDE.md` + project skills + project agent | ~1,900 | 4% |
| floor: custom prompt, no tools, no settings | ~1,100 | 2% |

The boilerplate is overwhelmingly **tool schemas**, not memory and not skills.
Trimming `CLAUDE.md` to save context is not worth doing; cutting the tool set is.
Going from the full set to `Read` + `Bash` removes more than half the prompt.

## A clean stateless invocation

```
claude -p --output-format stream-json --verbose \
  --setting-sources '' \
  --system-prompt "<own prompt>" \
  --disable-slash-commands \
  --strict-mcp-config \
  --no-session-persistence \
  --tools Read Write Edit Bash
```

Nothing from `~/.claude/` or the project reaches the model, nothing is written
back, and the prompt lands near the measured floor.

Note on `--bare`, which advertises exactly this behaviour: it also disables OAuth
and keychain reads, leaving `ANTHROPIC_API_KEY` or `apiKeyHelper` as the only
auth. On a subscription that means paying for API usage separately. The flag
combination above reaches the same isolation while subscription auth keeps
working — verified across every run recorded here.

## Per-agent working directories

If every agent gets its own directory, can the CLI assemble that agent's context
by itself — and only that agent's? Two directories under `tmp/agents/`, each with
its own `CLAUDE.md` and its own `.claude/skills/`, invoked with the working
directory set to the agent's own:

| run | JOHN | MARY | global `~/.claude` | parent dir | john-skill | mary-skill | user skill |
|---|---|---|---|---|---|---|---|
| john, `--setting-sources 'project'` | ALPHA-7 | — | **yes** | **yes** | yes | — | no |
| mary, `--setting-sources 'project'` | — | BETA-9 | **yes** | **yes** | — | yes | no |
| john, default | ALPHA-7 | — | yes | yes | yes | — | yes |
| john, `--setting-sources ''` | — | — | no | no | no | — | no |
| john, `--setting-sources 'project,local'` | ALPHA-7 | — | yes | yes | yes | — | no |

**Agents are cleanly separated from each other.** John never sees Mary's
`CLAUDE.md` or Mary's skill, and the reverse holds. Switching the working
directory is enough to switch identity.

Isolating each source on its own shows how the layers map onto memory:

| source | JOHN | global `~/.claude` | parent dir | john-skill | user skill |
|---|---|---|---|---|---|
| `'local'` | — | no | no | no | no |
| `'user'` | — | yes | **no** | no | yes |
| `'project'` | ALPHA-7 | yes | yes | yes | no |
| `''` | — | no | no | no | no |

`'local'` is equivalent to `''` here — 33291 tokens against 33338 for the same
prompt — because it is the layer of `.claude/settings.local.json`, which the
directory does not have, and neither memory nor skills belong to it.

`'user'` narrows things down: it brings the global `CLAUDE.md` but not the
parent-directory ones. So parent directories arrive with `'project'`, while
`~/.claude/CLAUDE.md` arrives whenever either `'project'` or `'user'` is enabled.
The `'project'` case reproduced across three independent runs, so it is not
sampling noise — it looks as though the user-level memory file terminates the
same discovery chain that walks up from the working directory.

No combination yields the agent's own memory without the global one.

**They are not separated from everything else.** `--setting-sources 'project'`
drops user-level *skills* — `user_skill` is false — but the project's `CLAUDE.md`
arrives together with the user-level one and with every `CLAUDE.md` found walking
up the directory tree. Memory discovery does not follow the setting-source
layers.

The upward walk does not stop at a repository boundary either: with `git init`
run inside the agent's own directory, the parent project's `CLAUDE.md` was still
present.

Isolating the home directory by moving it fails, because auth lives there too:

| attempt | result |
|---|---|
| `CLAUDE_CONFIG_DIR=<scratch>` | `Not logged in · Please run /login` |
| `HOME=<scratch>` | `Claude configuration file not found`, then not logged in |

So the choice is binary, with no middle setting:

- `--setting-sources 'project'` — the CLI assembles the agent's memory and
  skills, and also pulls in the user's `CLAUDE.md` and every parent one.
- `--setting-sources ''` — nothing at all is assembled, and the caller composes
  the system prompt itself from the agent's own files.

The second is the only way to get "this directory and nothing else". Its cost is
that skills cannot be injected through `--system-prompt`; they are a settings-layer
feature. `--plugin-dir`, which loads a plugin for one session only and can carry
skills, is the obvious candidate for recovering them without reopening the
settings layers — untested so far.

## Caveats

- The model reports its own context. Structured output makes the answers
  parseable, not necessarily true; counts drifted by ±1 between identical runs.
- The agent directories used for the isolation test sit inside this repository,
  so the "parent dir" column is this project's own `CLAUDE.md`. A directory with
  no `CLAUDE.md` anywhere above it was not tested.
- One machine, one CLI version, one probe model. The absolute token numbers
  reflect this user's config; the ratios should travel better than the totals.
- `--tools ''` was measured with `--json-schema`, which forces one tool in
  regardless.
