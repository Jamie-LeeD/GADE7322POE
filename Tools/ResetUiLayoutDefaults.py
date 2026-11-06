"""Force WBP Class Defaults UI|Layout sizes to the larger values (clears stale overrides)."""
import unreal

SIZES = {
    "/Game/UI/WBP_StartMenu": {
        "TitleMaxSize": unreal.Vector2D(980.0, 360.0),
        "PlayButtonMaxSize": unreal.Vector2D(760.0, 260.0),
        "StrategyPanelMaxSize": unreal.Vector2D(980.0, 860.0),
        "StrategyButtonMaxSize": unreal.Vector2D(680.0, 220.0),
        "BackButtonMaxSize": unreal.Vector2D(600.0, 200.0),
    },
    "/Game/UI/WBP_GameInfo": {
        "PanelMaxSize": unreal.Vector2D(1320.0, 940.0),
        "CloseButtonMaxSize": unreal.Vector2D(150.0, 150.0),
    },
    "/Game/UI/Bases/WBP_DefenderBarBase": {
        "InfoButtonMaxSize": unreal.Vector2D(320.0, 120.0),
    },
}


def main():
    for path, props in SIZES.items():
        if not unreal.EditorAssetLibrary.does_asset_exist(path):
            unreal.log_warning(f"Missing {path}")
            continue
        bp = unreal.EditorAssetLibrary.load_asset(path)
        if not bp:
            continue
        # WidgetBlueprint generated class CDO
        gen = bp.generated_class() if hasattr(bp, "generated_class") else None
        if not gen:
            try:
                gen = bp.get_editor_property("generated_class")
            except Exception:
                gen = None
        if not gen:
            unreal.log_warning(f"No generated class for {path}")
            continue
        cdo = unreal.get_default_object(gen)
        for name, value in props.items():
            try:
                cdo.set_editor_property(name, value)
                unreal.log(f"{path}.{name} = {value}")
            except Exception as exc:
                unreal.log_warning(f"Failed {path}.{name}: {exc}")
        unreal.EditorAssetLibrary.save_asset(path)
    unreal.log("DONE: reset UI layout defaults")


if __name__ == "__main__":
    main()
