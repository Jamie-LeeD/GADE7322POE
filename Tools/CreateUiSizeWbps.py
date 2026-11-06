"""Create WBP_StartMenu and WBP_GameInfo parented to C++ so Class Defaults expose UI|Layout sizes."""
import unreal

UI_DIR = "/Game/UI"


def ensure_wbp(asset_name: str, parent_path: str) -> None:
    dest = f"{UI_DIR}/{asset_name}"
    parent = unreal.load_class(None, parent_path)
    if not parent:
        unreal.log_error(f"Missing parent class {parent_path}")
        return

    if unreal.EditorAssetLibrary.does_asset_exist(dest):
        unreal.log(f"Already exists {dest}")
        return

    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", parent)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name, UI_DIR, unreal.WidgetBlueprint, factory
    )
    if asset:
        unreal.EditorAssetLibrary.save_asset(dest)
        unreal.log(f"Created {dest} (parent={parent.get_name()})")
    else:
        unreal.log_error(f"Failed to create {dest}")


def main():
    if not unreal.EditorAssetLibrary.does_directory_exist(UI_DIR):
        unreal.EditorAssetLibrary.make_directory(UI_DIR)
    ensure_wbp("WBP_StartMenu", "/Script/GADE7322POE.StartMenuWidget")
    ensure_wbp("WBP_GameInfo", "/Script/GADE7322POE.GameInfoWidget")
    unreal.log("DONE: UI size WBPs — open them, Class Defaults → UI | Layout to resize")


if __name__ == "__main__":
    main()
