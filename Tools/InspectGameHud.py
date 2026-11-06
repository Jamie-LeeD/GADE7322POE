"""Dump WBP_GameHUD and WBP_DefenderBarBase widget tree / parent class."""
import unreal

PATHS = [
    "/Game/UI/WBP_GameHUD",
    "/Game/UI/Bases/WBP_DefenderBarBase",
    "/Game/UI/Bases/WBP_DefenderSelectBase",
]


def dump(path: str) -> None:
    bp = unreal.EditorAssetLibrary.load_asset(path)
    unreal.log(f"==== {path} type={bp.get_class().get_name() if bp else None} ====")
    if not bp:
        return
    try:
        parent = bp.parent_class
        unreal.log(f"  parent_class={parent.get_name() if parent else None}")
    except Exception as exc:
        try:
            parent = bp.get_editor_property("parent_class")
            unreal.log(f"  parent_class={parent.get_name() if parent else None}")
        except Exception as exc2:
            unreal.log(f"  parent err {exc} / {exc2}")

    short = path.split("/")[-1]
    cls = unreal.load_class(None, f"{path}.{short}_C")
    if not cls:
        unreal.log("  no generated class")
        return
    cdo = unreal.get_default_object(cls)
    # Widget tree from CDO is empty for designer widgets; list bound props by name guesses
    for name in [
        "Overlay", "RootOverlay", "Panel", "PanelBorder", "Background", "BackgroundBorder",
        "DefenderBox", "HeadingText", "InfoButton", "Border", "Image", "CanvasPanel",
        "SelectPanel", "DefenderPanel", "HUDOverlay", "ContentOverlay",
    ]:
        try:
            val = cdo.get_editor_property(name)
            unreal.log(f"  prop {name} = {val}")
        except Exception:
            pass

    # SCS-like: WidgetTree via Blueprint
    try:
        wt = bp.widget_tree
        unreal.log(f"  widget_tree={wt}")
        if wt:
            root = wt.root_widget
            unreal.log(f"  root={root.get_name() if root else None} type={root.get_class().get_name() if root else None}")

            def walk(w, depth=0):
                if not w:
                    return
                pad = "  " * (depth + 1)
                extra = ""
                if isinstance(w, unreal.Border):
                    extra = " [Border]"
                if isinstance(w, unreal.Overlay):
                    extra = " [Overlay]"
                if isinstance(w, unreal.Image):
                    extra = " [Image]"
                unreal.log(f"{pad}{w.get_class().get_name()} '{w.get_name()}'{extra}")
                # children
                try:
                    if hasattr(w, "get_all_children"):
                        for c in w.get_all_children():
                            walk(c, depth + 1)
                    elif hasattr(w, "get_children_count"):
                        for i in range(w.get_children_count()):
                            walk(w.get_child_at(i), depth + 1)
                except Exception:
                    pass
                # PanelWidget
                try:
                    if isinstance(w, unreal.PanelWidget):
                        for i in range(w.get_children_count()):
                            walk(w.get_child_at(i), depth + 1)
                except Exception:
                    pass

            walk(root)
    except Exception as exc:
        unreal.log(f"  tree err {exc}")


def main():
    for p in PATHS:
        dump(p)
    unreal.log("DONE inspect HUD")


if __name__ == "__main__":
    main()
