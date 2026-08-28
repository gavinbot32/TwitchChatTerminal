# TwitchChatTerminal

A fast, cross-platform **terminal chat client for Twitch IRC** written in modern **C++ 20**.  
Log in and connect to Twitch chat, send and receive messages. Colors, badges, highlights, and more!

---

##  Features

| Category           | Details |
|--------------------|---------|
| Live chat          | Asynchronous SSL connection to `irc.chat.twitch.tv:6697` via **stand-alone ASIO** |
| Colours & badges   | 24-bit → 256/16-colour down-sampling, badge abbreviations (`MOD`, `VIP`, etc.) |
| JSON settings      | Local **`config/`** folder next to the binary; auto-created on first run (`user-settings.json`, `credentials.json`) |
| Token validation   | Verifies your OAuth token against Twitch’s `/oauth2/validate` endpoint at start-up |
| Slash commands     | `/help`, `/clear`, `/quit`, `/set`, `/badges`, `/highlight`, plus extensible command registry |
| Highlights         | Per-user or badge highlight colours (stored in `user-settings.json`) |
| Portable build     | No system installs beyond **OpenSSL**; `nlohmann/json` fetched automatically; ASIO vendored |

---

##  Requirements

| Tool | Version |
|------|---------|
| **CMake** | ≥ 3.31 |
| **C++ compiler** | C++ 20 capable (g++ 11+, clang 13+, MSVC 19.36+) |
| **OpenSSL dev libs** | 1.1+ |

---

##  Initial Run

```bash
./TwitchConsoleViewer
```

1. **Username** – enter your Twitch username.  
2. **OAuth token** – generate one at <https://twitchtokengenerator.com> (`chat:read` scope is enough).  
3. **Channel** – the channel to join.

These are saved to `config/credentials.json`.  
Chat starts immediately; future launches read the stored credentials.

---

##  Built-in Commands

| Command | Description |
|---------|-------------|
| `/help` | Display interactive help table |
| `/join` | Join a channel |
| `/clear` | Clears terminal screen |
| `/quit` | Gracefully disconnect & exit |
| `/set channel <name>` | Change default channel in `credentials.json` |
| `/set channel_color <#hex or named>` | Sets the channel color |
| `/badges` | List recognised badges |
| `/highlight` | Show active highlights |
| `/highlight add "<highlight>" <"user"or"badge"> <#hex>` | Highlight a user or badge |
| `/highlight remove "<highlight>"` | Delete a highlight |


*(Commands are extensible – add new classes inheriting `Command` and register them in `CommandRegistry`.)*

---

##  Colour Support

- True-colour terminals get exact hex values.  
- 256-/16-colour terminals fall back automatically.  
- Named colours (`red`, `blue`, …) → hex via internal map.

---

## Why a chat client in the terminal?

I thought it would be a fun project and a great way to learn C++.
