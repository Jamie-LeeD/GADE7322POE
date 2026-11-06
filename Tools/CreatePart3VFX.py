"""
Part 3 VFX:
1) Custom post-process material M_PP_TowerThreat (screen-space vignette when princess is hurt)
2) Custom Cascade particle system PS_UpgradeBurst (spark burst on type-wide upgrades)
   + Niagara system NS_UpgradeBurst (optional; finish emitter modules in Niagara editor)
"""
import unreal

VFX_DIR = "/Game/VFX"
PP_PATH = f"{VFX_DIR}/M_PP_TowerThreat"
NS_PATH = f"{VFX_DIR}/NS_UpgradeBurst"
NE_PATH = f"{VFX_DIR}/NE_UpgradeSpark"
PS_PATH = f"{VFX_DIR}/PS_UpgradeBurst"
SPRITE_MAT_PATH = f"{VFX_DIR}/M_UpgradeSparkSprite"


def E(mat, cls, x, y):
    return unreal.MaterialEditingLibrary.create_material_expression(mat, cls, x, y)


def C(a, ap, b, bp):
    unreal.MaterialEditingLibrary.connect_material_expressions(a, ap, b, bp)


def ensure_dir():
    if not unreal.EditorAssetLibrary.does_directory_exist(VFX_DIR):
        unreal.EditorAssetLibrary.make_directory(VFX_DIR)


def create_post_process_material():
    """Screen-space threat vignette — Material Domain Post Process."""
    if unreal.EditorAssetLibrary.does_asset_exist(PP_PATH):
        mat = unreal.EditorAssetLibrary.load_asset(PP_PATH)
        unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
    else:
        mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "M_PP_TowerThreat", VFX_DIR, unreal.Material, unreal.MaterialFactoryNew()
        )

    mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
    try:
        mat.set_editor_property("blendable_location", unreal.BlendableLocation.BL_AFTER_TONEMAPPING)
    except Exception:
        pass

    uv = E(mat, unreal.MaterialExpressionTextureCoordinate, -1200, 0)
    scene = E(mat, unreal.MaterialExpressionSceneTexture, -900, -40)
    try:
        scene.set_editor_property(
            "scene_texture_id", unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0
        )
    except Exception:
        pass
    try:
        C(uv, "", scene, "Coordinates")
    except Exception:
        pass

    center = E(mat, unreal.MaterialExpressionConstant2Vector, -1200, 200)
    try:
        center.set_editor_property("constant", unreal.Vector2D(0.5, 0.5))
    except Exception:
        pass
    sub = E(mat, unreal.MaterialExpressionSubtract, -900, 160)
    C(uv, "", sub, "A")
    C(center, "", sub, "B")

    abs_n = E(mat, unreal.MaterialExpressionAbs, -700, 160)
    C(sub, "", abs_n, "")

    mx = E(mat, unreal.MaterialExpressionComponentMask, -500, 120)
    mx.set_editor_property("r", True)
    mx.set_editor_property("g", False)
    mx.set_editor_property("b", False)
    mx.set_editor_property("a", False)
    C(abs_n, "", mx, "")
    my = E(mat, unreal.MaterialExpressionComponentMask, -500, 200)
    my.set_editor_property("r", False)
    my.set_editor_property("g", True)
    my.set_editor_property("b", False)
    my.set_editor_property("a", False)
    C(abs_n, "", my, "")
    add_xy = E(mat, unreal.MaterialExpressionAdd, -300, 160)
    C(mx, "", add_xy, "A")
    C(my, "", add_xy, "B")

    intensity = E(mat, unreal.MaterialExpressionScalarParameter, -300, 300)
    intensity.set_editor_property("parameter_name", "ThreatIntensity")
    intensity.set_editor_property("default_value", 0.0)

    # Soft power curve on radial distance
    pow_n = E(mat, unreal.MaterialExpressionPower, -100, 120)
    C(add_xy, "", pow_n, "Base")
    exp = E(mat, unreal.MaterialExpressionConstant, -100, 200)
    exp.set_editor_property("r", 1.6)
    C(exp, "", pow_n, "Exp")

    edge = E(mat, unreal.MaterialExpressionMultiply, 100, 160)
    C(pow_n, "", edge, "A")
    C(intensity, "", edge, "B")

    clamp_n = E(mat, unreal.MaterialExpressionClamp, 300, 160)
    C(edge, "", clamp_n, "")
    try:
        mn = E(mat, unreal.MaterialExpressionConstant, 300, 240)
        mn.set_editor_property("r", 0.0)
        mxv = E(mat, unreal.MaterialExpressionConstant, 300, 300)
        mxv.set_editor_property("r", 1.0)
        C(mn, "", clamp_n, "Min")
        C(mxv, "", clamp_n, "Max")
    except Exception:
        pass

    threat_col = E(mat, unreal.MaterialExpressionVectorParameter, -100, -80)
    threat_col.set_editor_property("parameter_name", "ThreatColor")
    threat_col.set_editor_property("default_value", unreal.LinearColor(0.85, 0.05, 0.12, 1.0))

    one_minus = E(mat, unreal.MaterialExpressionOneMinus, 500, 160)
    C(clamp_n, "", one_minus, "")
    scene_mul = E(mat, unreal.MaterialExpressionMultiply, 700, -40)
    try:
        C(scene, "Color", scene_mul, "A")
    except Exception:
        C(scene, "", scene_mul, "A")
    C(one_minus, "", scene_mul, "B")

    tint_mul = E(mat, unreal.MaterialExpressionMultiply, 700, 80)
    C(threat_col, "", tint_mul, "A")
    C(clamp_n, "", tint_mul, "B")

    add_out = E(mat, unreal.MaterialExpressionAdd, 950, 20)
    C(scene_mul, "", add_out, "A")
    C(tint_mul, "", add_out, "B")

    unreal.MaterialEditingLibrary.connect_material_property(
        add_out, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR
    )

    unreal.MaterialEditingLibrary.layout_material_expressions(mat)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(PP_PATH)
    unreal.log(f"Created post-process {PP_PATH}")
    return mat


def create_spark_sprite_material():
    """Unlit translucent gold spark for Cascade/Niagara sprites."""
    if unreal.EditorAssetLibrary.does_asset_exist(SPRITE_MAT_PATH):
        mat = unreal.EditorAssetLibrary.load_asset(SPRITE_MAT_PATH)
        unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
    else:
        mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "M_UpgradeSparkSprite", VFX_DIR, unreal.Material, unreal.MaterialFactoryNew()
        )

    mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_SURFACE)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    try:
        mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    except Exception:
        pass
    try:
        mat.set_editor_property("two_sided", True)
    except Exception:
        pass

    color = E(mat, unreal.MaterialExpressionVectorParameter, -400, 0)
    color.set_editor_property("parameter_name", "SparkColor")
    color.set_editor_property("default_value", unreal.LinearColor(1.0, 0.75, 0.2, 1.0))

    # Soft radial particle from UV distance to center
    uv = E(mat, unreal.MaterialExpressionTextureCoordinate, -600, 200)
    center = E(mat, unreal.MaterialExpressionConstant2Vector, -600, 280)
    try:
        center.set_editor_property("constant", unreal.Vector2D(0.5, 0.5))
    except Exception:
        pass
    sub = E(mat, unreal.MaterialExpressionSubtract, -400, 220)
    C(uv, "", sub, "A")
    C(center, "", sub, "B")
    # length approx
    abs_n = E(mat, unreal.MaterialExpressionAbs, -200, 220)
    C(sub, "", abs_n, "")
    mx = E(mat, unreal.MaterialExpressionComponentMask, 0, 180)
    mx.set_editor_property("r", True)
    mx.set_editor_property("g", False)
    mx.set_editor_property("b", False)
    mx.set_editor_property("a", False)
    C(abs_n, "", mx, "")
    my = E(mat, unreal.MaterialExpressionComponentMask, 0, 260)
    my.set_editor_property("r", False)
    my.set_editor_property("g", True)
    my.set_editor_property("b", False)
    my.set_editor_property("a", False)
    C(abs_n, "", my, "")
    add_xy = E(mat, unreal.MaterialExpressionAdd, 200, 220)
    C(mx, "", add_xy, "A")
    C(my, "", add_xy, "B")
    one = E(mat, unreal.MaterialExpressionConstant, 200, 300)
    one.set_editor_property("r", 1.0)
    om = E(mat, unreal.MaterialExpressionOneMinus, 400, 220)
    C(add_xy, "", om, "")
    saturate = E(mat, unreal.MaterialExpressionClamp, 600, 220)
    C(om, "", saturate, "")
    try:
        mn = E(mat, unreal.MaterialExpressionConstant, 600, 300)
        mn.set_editor_property("r", 0.0)
        mxv = E(mat, unreal.MaterialExpressionConstant, 600, 360)
        mxv.set_editor_property("r", 1.0)
        C(mn, "", saturate, "Min")
        C(mxv, "", saturate, "Max")
    except Exception:
        pass
    soft = E(mat, unreal.MaterialExpressionPower, 800, 220)
    C(saturate, "", soft, "Base")
    exp = E(mat, unreal.MaterialExpressionConstant, 800, 300)
    exp.set_editor_property("r", 2.0)
    C(exp, "", soft, "Exp")

    col_mul = E(mat, unreal.MaterialExpressionMultiply, 1000, 40)
    C(color, "", col_mul, "A")
    C(soft, "", col_mul, "B")

    unreal.MaterialEditingLibrary.connect_material_property(
        col_mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        soft, "", unreal.MaterialProperty.MP_OPACITY
    )

    unreal.MaterialEditingLibrary.layout_material_expressions(mat)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(SPRITE_MAT_PATH)
    unreal.log(f"Created spark sprite {SPRITE_MAT_PATH}")
    return mat


def _configure_cascade_emitter(ps, spark_mat):
    """Best-effort Cascade emitter configuration for a spark burst."""
    try:
        emitters = ps.get_editor_property("emitters")
    except Exception as ex:
        unreal.log_warning(f"No emitters property: {ex}")
        return False

    if not emitters or len(emitters) == 0:
        unreal.log_warning("Cascade system has no emitters yet — open PS_UpgradeBurst and add a sprite emitter")
        return False

    emitter = emitters[0]
    try:
        lod = emitter.get_editor_property("lod_levels")[0]
    except Exception:
        try:
            lod = emitter.lod_levels[0]
        except Exception as ex:
            unreal.log_warning(f"Could not read LOD: {ex}")
            return False

    # Required modules on LOD
    try:
        required = lod.get_editor_property("required_module")
        if required and spark_mat:
            try:
                required.set_editor_property("material", spark_mat)
            except Exception:
                pass
            try:
                required.set_editor_property("emitter_duration", 0.1)
                required.set_editor_property("emitter_loops", 1)
                required.set_editor_property("emitter_duration_low", 0.1)
            except Exception:
                pass
    except Exception as ex:
        unreal.log_warning(f"required_module: {ex}")

    try:
        spawn = lod.get_editor_property("spawn_module")
        if spawn:
            # Burst-style: high rate for short duration
            try:
                rate = spawn.get_editor_property("rate")
                # DistributionFloatConstant
                if hasattr(rate, "set_editor_property"):
                    rate.set_editor_property("constant", 200.0)
            except Exception:
                pass
    except Exception as ex:
        unreal.log_warning(f"spawn_module: {ex}")

    return True


def create_cascade_burst(spark_mat):
    """Create Cascade ParticleSystem — primary particle VFX for Part 3."""
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

    if unreal.EditorAssetLibrary.does_asset_exist(PS_PATH):
        unreal.EditorAssetLibrary.delete_asset(PS_PATH)

    factory = None
    for name in ("ParticleSystemFactoryNew", "CascadeParticleSystemFactoryNew"):
        cls = getattr(unreal, name, None)
        if cls is not None:
            try:
                factory = cls()
                unreal.log(f"Using {name}")
                break
            except Exception as ex:
                unreal.log_warning(f"{name} failed: {ex}")

    if factory is None:
        unreal.log_warning("No ParticleSystem factory available")
        return None

    ps = asset_tools.create_asset("PS_UpgradeBurst", VFX_DIR, unreal.ParticleSystem, factory)
    if not ps:
        unreal.log_warning("Could not create ParticleSystem")
        return None

    configured = _configure_cascade_emitter(ps, spark_mat)
    unreal.EditorAssetLibrary.save_asset(PS_PATH)
    unreal.log(f"Created Cascade {PS_PATH} (configured={configured})")
    unreal.log(
        "If sparks are missing: open PS_UpgradeBurst → add Sprite emitter, "
        "Lifetime=200 for 0.15s, Lifetime 0.2-0.5, Velocity cone up, Material=M_UpgradeSparkSprite"
    )
    return ps


def create_niagara_burst():
    """Create empty Niagara system (finish modules in Niagara editor if Cascade is not used)."""
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

    if unreal.EditorAssetLibrary.does_asset_exist(NE_PATH):
        unreal.EditorAssetLibrary.delete_asset(NE_PATH)
    if unreal.EditorAssetLibrary.does_asset_exist(NS_PATH):
        unreal.EditorAssetLibrary.delete_asset(NS_PATH)

    try:
        emitter_factory = unreal.NiagaraEmitterFactoryNew()
        emitter = asset_tools.create_asset(
            "NE_UpgradeSpark", VFX_DIR, unreal.NiagaraEmitter, emitter_factory
        )
        if emitter:
            unreal.EditorAssetLibrary.save_asset(NE_PATH)
    except Exception as ex:
        unreal.log_warning(f"Niagara emitter: {ex}")
        emitter = None

    try:
        system_factory = unreal.NiagaraSystemFactoryNew()
        system = asset_tools.create_asset(
            "NS_UpgradeBurst", VFX_DIR, unreal.NiagaraSystem, system_factory
        )
    except Exception as ex:
        unreal.log_warning(f"Niagara system: {ex}")
        return None

    if not system:
        return None

    added = False
    if emitter:
        for method_name in ("add_emitter_handle", "add_emitter", "add_emitter_as_copy"):
            fn = getattr(system, method_name, None)
            if callable(fn):
                try:
                    fn(emitter)
                    added = True
                    break
                except Exception:
                    pass

    unreal.EditorAssetLibrary.save_asset(NS_PATH)
    unreal.log(f"Created Niagara {NS_PATH} (emitter_added={added})")
    return system


def main():
    ensure_dir()
    create_post_process_material()
    spark = create_spark_sprite_material()
    create_cascade_burst(spark)
    create_niagara_burst()
    unreal.log("DONE: Part 3 VFX assets under /Game/VFX")


if __name__ == "__main__":
    main()
