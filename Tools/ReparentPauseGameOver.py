"""Reparent WBP_PauseMenu and WBP_GameOver to OrnateMenuWidget."""
import unreal

PARENT = "/Script/GADE7322POE.OrnateMenuWidget"
PATHS = ["/Game/UI/WBP_PauseMenu", "/Game/UI/WBP_GameOver"]


def reparent(path: str, parent) -> None:
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.log_error(f"Missing {path}")
        return
    bp = unreal.EditorAssetLibrary.load_asset(path)
    if not bp:
        unreal.log_error(f"Failed load {path}")
        return
    try:
        unreal.BlueprintEditorLibrary.reparent_blueprint(bp, parent)
        unreal.log(f"Reparented {path}")
    except Exception as exc:
        unreal.log_error(f"Reparent failed {path}: {exc}")
        return
    try:
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    except Exception:
        pass
    unreal.EditorAssetLibrary.save_asset(path)


def main():
    parent = unreal.load_class(None, PARENT)
    if not parent:
        unreal.log_error(f"Missing {PARENT} — compile C++ first")
        return
    for path in PATHS:
        reparent(path, parent)
    unreal.log("DONE: pause/gameover panels use pausepanel")


if __name__ == "__main__":
    main()
