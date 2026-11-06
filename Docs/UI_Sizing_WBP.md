# Resize UI in Widget Blueprints

## Quick way (recommended)

1. Compile C++ / open the editor.
2. Open **`/Game/UI/WBP_StartMenu`** or **`/Game/UI/WBP_GameInfo`** (or `WBP_DefenderBarBase` for the Info button).
3. In the toolbar open **Class Defaults** (not a selected widget).
4. Find category **UI | Layout**.
5. Change the max sizes (e.g. `Play Button Max Size`).
6. **Compile** the Blueprint, then PIE — art keeps aspect ratio and fits that box.

### Properties

| WBP | Properties |
|-----|------------|
| `WBP_StartMenu` | Title, Play, Strategy panel, Strategy buttons, Back |
| `WBP_GameInfo` | Panel, Close button |
| `WBP_DefenderBarBase` | Info button |

## Designer Size Boxes (visual drag)

1. Open the WBP → **Designer**.
2. Wrap a button in a **Size Box**.
3. Set **Width Override** / **Height Override** on the Size Box.
4. Name the button exactly: `PlayButton`, `CloseButton`, `InfoButton`, etc. (Is Variable on).
5. Compile — runtime reads the Size Box overrides when applying art.

If the WBP has no designer buttons yet, C++ still builds a fallback layout using the Class Defaults sizes.
