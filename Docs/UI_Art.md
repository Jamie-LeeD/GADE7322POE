# Grimm Tower UI Art

Source PNGs and imported textures live in `/Game/UI/Art` (`Content/UI/Art`).

## Wired in C++

| Use | Texture |
|-----|---------|
| Start menu background | `startscreenpanel` |
| Title logo | `gametitle` |
| Play | `playbttn` |
| Strategy panel | `blankpanel2` |
| Budget / Counter buttons | `menubutton` |
| Back | `backbttn` |
| Info HUD button | `infobttn` |
| Info panel frame | `blankpanel1` |
| Info close | `xbttn` |

## Available for Blueprint HUD / Pause / Game Over

`pausebutton`, `pauseiconbutton`, `quitbutton`, `quiticonbutton`, `retrybutton`, `retryiconbutton`, `youwintitle`, `youlosetitle`, `menupanel`, `controlspanel`, `controlsbutton`, `rulesbutton`, `startbutton`, `nextbttn`, icon variants, etc.

In a WBP: add an **Image** or set a **Button → Style → Normal** brush to the matching `/Game/UI/Art/...` texture.

## Re-import

```
python Tools/PrepareUiArtTransparency.py
# then in editor or cmd:
UnrealEditor-Cmd ... -ExecutePythonScript=Tools/ImportUiArt.py
```
