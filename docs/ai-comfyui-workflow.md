Url: https://www.reddit.com/r/ClaudeCode/comments/1uqww2g/to_those_people_building_cool_animations_with/

As promised — repos are up:

🎬 ai-director — https://github.com/lnguyen503/ai-director
The workflow/pattern itself: two Claude Code sessions (a "director" that writes briefs + reviews, an "engine" that runs the renders) coordinating through plain markdown files, with the human holding all the money/publish decisions. No framework, no server — just files you can read.

🏗️ backlot — https://github.com/lnguyen503/backlot
The engine half: wraps a local ComfyUI install with a typed workflow registry + web studio + MCP server so an AI agent can drive it. Blender bridge for the depth→restyle stuff from my earlier post. 100% local, $0/generation.

Quick start:

backlot: git clone → pip install -e '.[dev]' → point config/engine.yaml at your ComfyUI → python -m backlot.web.app → studio at 127.0.0.1:8765. Test suite runs in ~20s with no GPU. CI is green on Linux + Windows.

ai-director: no install — copy the templates into a relay/ folder and paste the two bootstrap prompts from docs/getting-started.md into two Claude Code windows. Full walkthrough in there.

Both MIT. The docs include the production lessons (identity drift, garbled text, A/V sync traps) that cost me real render hours to learn.

Also, if you find this helpful, help a bro out and subscribe to my YouTube channel — my videos are made with this exact workflow: https://www.youtube.com/@GatherRoundStory

Happy to answer questions if anything breaks.