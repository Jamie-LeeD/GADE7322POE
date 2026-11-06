"""Add UEnemyFabMeshComponent to goblin enemy BPs so Fab skeletal meshes replace cylinders."""
import unreal

BPS = (
    "/Game/Enemies/BP_GoblinEnemy",
    "/Game/Enemies/BP_GoblinBerserker",
    "/Game/Enemies/BP_GoblinShaman",
)
COMP_CLASS = unreal.load_class(None, "/Script/GADE7322POE.EnemyFabMeshComponent")


def ensure_component(bp_path: str) -> None:
    if not unreal.EditorAssetLibrary.does_asset_exist(bp_path):
        unreal.log_warning(f"Missing {bp_path}")
        return
    if not COMP_CLASS:
        unreal.log_error("EnemyFabMeshComponent class not found — compile C++ first")
        return

    bp = unreal.EditorAssetLibrary.load_asset(bp_path)
    sds = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    handles = sds.k2_gather_subobject_data_for_blueprint(bp)

    for h in handles:
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h)
        obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)
        if obj and obj.get_class() == COMP_CLASS:
            unreal.log(f"Already has EnemyFabMesh on {bp_path}")
            return

    # Parent = root / default scene root handle
    parent = None
    for h in handles:
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h)
        if unreal.SubobjectDataBlueprintFunctionLibrary.is_root_component(data) or \
           unreal.SubobjectDataBlueprintFunctionLibrary.is_handle_valid(h):
            # Prefer actor root as attach parent for actor components
            obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)
            if obj and isinstance(obj, unreal.Blueprint):
                continue
            if unreal.SubobjectDataBlueprintFunctionLibrary.is_root_component(data):
                parent = h
                break
    if parent is None and handles:
        parent = handles[0]

    params = unreal.AddNewSubobjectParams()
    params.set_editor_property("parent_handle", parent)
    params.set_editor_property("new_class", COMP_CLASS)
    params.set_editor_property("b_defer_construction", False)
    try:
        new_handle, fail_reason = sds.add_new_subobject(params)
        if fail_reason and str(fail_reason) not in ("", "None"):
            unreal.log_warning(f"add_new_subobject reason={fail_reason} for {bp_path}")
        # Rename
        sds.rename_subobject(new_handle, "EnemyFabMesh")
        unreal.EditorAssetLibrary.save_asset(bp_path)
        unreal.log(f"Added EnemyFabMesh to {bp_path}")
    except Exception as ex:
        unreal.log_warning(f"Failed adding to {bp_path}: {ex}")
        # Fallback: mark dirty note for manual add
        unreal.log_warning("Add EnemyFabMeshComponent manually in BP if needed")


def main():
    for path in BPS:
        ensure_component(path)
    unreal.log("DONE: EnemyFabMesh components")


if __name__ == "__main__":
    main()
