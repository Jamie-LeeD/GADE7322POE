import unreal

def dump_mat(path):
    mat = unreal.EditorAssetLibrary.load_asset(path)
    unreal.log(f"=== {path} type={type(mat).__name__}")
    try:
        exprs = unreal.MaterialEditingLibrary.get_material_expressions(mat)
        unreal.log(f"expr_count={len(exprs)}")
        for e in exprs:
            unreal.log(f"  {e.get_class().get_name()} '{getattr(e, 'parameter_name', '')}'")
    except Exception as ex:
        unreal.log(f"exprs fail: {ex}")
    # Parent material attrs for MIs
    if isinstance(mat, unreal.MaterialInstanceConstant):
        unreal.log(f"parent={mat.get_editor_property('parent')}")
        try:
            for n in ("WaveAmplitude", "Tiling", "WaveSpeed"):
                v = unreal.MaterialEditingLibrary.get_material_instance_scalar_parameter_value(mat, n)
                unreal.log(f"  scalar {n}={v}")
        except Exception as ex:
            unreal.log(f"scalar fail: {ex}")
        try:
            t = unreal.MaterialEditingLibrary.get_material_instance_texture_parameter_value(mat, "BaseColor")
            unreal.log(f"  tex BaseColor={t}")
        except Exception as ex:
            unreal.log(f"tex fail: {ex}")

dump_mat("/Game/Shaders/M_WavingGrass")
dump_mat("/Game/Terrain/Materials/M_TileGroundA")
dump_mat("/Game/Stylized_Grass/Material/M_Stylized_Grass")

# Tile BP mesh
bp = unreal.EditorAssetLibrary.load_asset("/Game/Terrain/BP_TerrainTile")
gen = bp.generated_class()
cdo = unreal.get_default_object(gen)
unreal.log(f"BP CDO={cdo}")
for comp in cdo.get_components_by_class(unreal.ActorComponent.static_class()):
    unreal.log(f"comp {comp.get_name()} class={comp.get_class().get_name()}")
    if isinstance(comp, unreal.StaticMeshComponent):
        mesh = comp.get_editor_property("static_mesh")
        unreal.log(f"  mesh={mesh}")
        try:
            unreal.log(f"  eval_wpo={comp.get_editor_property('evaluate_world_position_offset')}")
        except Exception as ex:
            unreal.log(f"  eval_wpo err {ex}")
        for i in range(comp.get_num_materials()):
            unreal.log(f"  slot{i}={comp.get_material(i)}")
