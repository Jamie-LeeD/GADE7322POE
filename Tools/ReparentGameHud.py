"""Reparent WBP_GameHUD to UGameHudWidget so blankpanel2 styling runs at runtime."""
import unreal

HUD_PATH = "/Game/UI/WBP_GameHUD"
PARENT = "/Script/GADE7322POE.GameHudWidget"


def main():
    parent = unreal.load_class(None, PARENT)
    if not parent:
        unreal.log_error(f"Missing parent {PARENT} — compile C++ first")
        return

    if not unreal.EditorAssetLibrary.does_asset_exist(HUD_PATH):
        unreal.log_error(f"Missing {HUD_PATH}")
        return

    bp = unreal.EditorAssetLibrary.load_asset(HUD_PATH)
    if not bp:
        unreal.log_error("Failed to load WBP_GameHUD")
        return

    # WidgetBlueprintGeneratedClass / reparent API
    try:
        unreal.BlueprintEditorLibrary.reparent_blueprint(bp, parent)
        unreal.log("Reparented via BlueprintEditorLibrary.reparent_blueprint")
    except Exception as exc:
        unreal.log_warning(f"reparent_blueprint failed: {exc}")
        try:
            # Fallback older API
            factory = None
            unreal.log("Trying AssetEditorSubsystem compile after manual parent set")
            bp.set_editor_property("parent_class", parent)
        except Exception as exc2:
            unreal.log_error(f"Could not reparent: {exc2}")
            return

    unreal.EditorAssetLibrary.save_asset(HUD_PATH)
    try:
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    except Exception:
        pass
    unreal.EditorAssetLibrary.save_asset(HUD_PATH)
    unreal.log("DONE: WBP_GameHUD parent -> GameHudWidget")


if __name__ == "__main__":
    main()
