import unreal

GOBLIN_MAT = "/Game/Enemies/Materials/M_GoblinBody"
BP_PATH = "/Game/Enemies/BP_GoblinEnemy"


def main():
    mat = unreal.EditorAssetLibrary.load_asset(GOBLIN_MAT)
    if not mat:
        unreal.log_error("M_GoblinBody missing — run FixGoblinAndGrassMats.py first")
        return

    bp = unreal.EditorAssetLibrary.load_asset(BP_PATH)
    sds = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    handles = sds.k2_gather_subobject_data_for_blueprint(bp)

    for h in handles:
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h)
        obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)
        if not obj or obj.get_name() != "Mesh_GEN_VARIABLE":
            continue

        # Clear then set override materials array (persists on SCS templates)
        try:
            obj.set_editor_property("override_materials", [mat])
        except Exception as ex:
            unreal.log_warning(f"override_materials failed: {ex}")
        obj.set_material(0, mat)

        # Force property change notification through subsystem when available
        try:
            sds.mark_subobject_as_modified(h)
        except Exception:
            pass

        unreal.log(f"Mesh material now {obj.get_material(0)}")
        try:
            unreal.log(f"override_materials={obj.get_editor_property('override_materials')}")
        except Exception as ex:
            unreal.log_warning(f"read overrides: {ex}")

    try:
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
        unreal.log("Compiled BP_GoblinEnemy")
    except Exception as ex:
        unreal.log_warning(f"compile: {ex}")

    unreal.EditorAssetLibrary.save_asset(BP_PATH)
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)

    # Re-load verify
    bp2 = unreal.EditorAssetLibrary.load_asset(BP_PATH)
    for h in sds.k2_gather_subobject_data_for_blueprint(bp2):
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h)
        obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)
        if obj and obj.get_name() == "Mesh_GEN_VARIABLE":
            m = obj.get_material(0)
            unreal.log(f"VERIFY Mesh -> {m.get_path_name() if m else None}")

    unreal.log("DONE: goblin body material assignment")


if __name__ == "__main__":
    main()
