import unreal

mat = unreal.EditorAssetLibrary.load_asset("/Game/Enemies/Materials/M_GoblinBody")
unreal.log(f"goblin mat={mat}")
sds = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
for path in (
    "/Game/Enemies/BP_GoblinEnemy",
    "/Game/Enemies/BP_GoblinBerserker",
    "/Game/Enemies/BP_GoblinShaman",
    "/Game/Terrain/BP_TerrainTile",
):
    bp = unreal.EditorAssetLibrary.load_asset(path)
    for h in sds.k2_gather_subobject_data_for_blueprint(bp):
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h)
        obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)
        if not obj or not isinstance(obj, unreal.StaticMeshComponent):
            continue
        m = obj.get_material(0)
        unreal.log(f"{path} | {obj.get_name()} -> {m.get_path_name() if m else None}")
