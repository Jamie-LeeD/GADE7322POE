import unreal

def list_cdo_object_props(path, keys):
    unreal.log(f"==== {path}")
    bp = unreal.EditorAssetLibrary.load_asset(path)
    gen = bp.generated_class()
    cdo = unreal.get_default_object(gen)
    # Use UE reflection
    for prop in unreal.Fields.get_fields(gen, unreal.Property, True, True, True):
        name = prop.get_name()
        lname = name.lower()
        if not any(k in lname for k in keys):
            continue
        try:
            val = cdo.get_editor_property(name)
            unreal.log(f"CDO.{name} = {val}")
        except Exception as ex:
            try:
                # map/array
                unreal.log(f"CDO.{name} (raw access failed: {ex})")
            except Exception:
                pass


def list_all_interesting(path):
    unreal.log(f"==== ALL OBJ {path}")
    bp = unreal.EditorAssetLibrary.load_asset(path)
    gen = bp.generated_class()
    cdo = unreal.get_default_object(gen)
    try:
        fields = list(unreal.Fields.get_fields(gen, unreal.ObjectProperty, True, True, True))
        unreal.log(f"object props count={len(fields)}")
        for prop in fields:
            name = prop.get_name()
            try:
                val = cdo.get_editor_property(name)
                if val is not None:
                    unreal.log(f"  {name}={val}")
            except Exception:
                pass
    except Exception as ex:
        unreal.log(f"Fields API failed: {ex}")

    # Fallback: export properties via get_editor_property known names
    candidates = [
        "GroundMaterialA", "GroundMaterialB", "GroundMaterialC",
        "MaterialGroundA", "MaterialGroundB", "MaterialGroundC",
        "TileMaterials", "Materials", "GroundMaterials", "MaterialMap",
        "MatGroundA", "MatGroundB", "MatGroundC",
        "M_TileGroundA", "GroundA", "GroundB", "GroundC",
        "GrassMaterial", "TileMaterial", "DefaultMaterial",
        "BodyMaterial", "MeshMaterial", "EnemyMaterial",
        "BlockedMaterial", "PathMaterial", "SpawnMaterial",
        "TowerPadMaterial", "DefenderMaterial",
    ]
    for name in candidates:
        try:
            val = cdo.get_editor_property(name)
            unreal.log(f"try {name}={val}")
        except Exception:
            pass


list_all_interesting("/Game/Terrain/BP_TerrainTile")
list_all_interesting("/Game/Enemies/BP_GoblinEnemy")

# Also dump goblin mesh materials
bp = unreal.EditorAssetLibrary.load_asset("/Game/Enemies/BP_GoblinEnemy")
sds = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
for h in sds.k2_gather_subobject_data_for_blueprint(bp):
    data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h)
    obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)
    if not obj:
        continue
    unreal.log(f"goblincomp {obj.get_name()} {obj.get_class().get_name()}")
    if isinstance(obj, unreal.MeshComponent):
        for i in range(obj.get_num_materials()):
            unreal.log(f"  slot{i}={obj.get_material(i)}")
