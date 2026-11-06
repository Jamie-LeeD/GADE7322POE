# Part 3 — Custom Visual Effects

## Overview

Two custom VFX support Princess TD feedback:

| Requirement | Asset / system | Gameplay role |
|-------------|----------------|---------------|
| Particle system | `/Game/VFX/PS_UpgradeBurst` (Cascade) + `/Game/VFX/NS_UpgradeBurst` (Niagara) + `AUpgradeSparkBurstActor` | Gold spark burst when a defender type is upgraded |
| Screen-space post-process | `/Game/VFX/M_PP_TowerThreat` | Red edge vignette as the princess tower loses health |

Post-process and particle materials are authored in Unreal’s Material / Cascade / Niagara graph editors. Runtime driving is in `UGameplayVfxSubsystem`.

## 1. Particle system — Upgrade burst

**When:** `PurchaseClassUpgrade` → `PlayUpgradeBurstForActor` on each upgraded defender.

**Why it helps:** Makes the global upgrade purchase readable — a clear spark celebration on the units that just powered up (pairs with blue/pink auras).

### Assets

- `M_UpgradeSparkSprite` — custom additive unlit spark material (Material Editor)
- `PS_UpgradeBurst` — Cascade particle system (created by `Tools/CreatePart3VFX.py`)
- `NS_UpgradeBurst` / `NE_UpgradeSpark` — Niagara equivalents
- `AUpgradeSparkBurstActor` — custom mesh-spark burst so the effect always plays in PIE

### Finish Cascade in the editor (recommended for the rubric)

1. Open `/Game/VFX/PS_UpgradeBurst`
2. Add a **Sprite** emitter if none exists
3. Required: Material = `M_UpgradeSparkSprite`, Duration ≈ 0.15, Loops = 1
4. Spawn rate ≈ 200 for that short duration (or use a burst)
5. Lifetime 0.25–0.55, Velocity cone upward, Color gold/pink, Size shrinks over life

### Finish Niagara (optional)

1. Open `NS_UpgradeBurst`
2. Add emitter modules: **Spawn Burst Instantaneous** (~40), **Initialize Particle**, **Add Velocity**, **Sprite Renderer**
3. Sprite material / color: gold–pink; Auto Destroy when inactive

## 2. Screen-space post-process — Tower threat

- **Material domain:** Post Process (`M_PP_TowerThreat`)
- **Parameters:** `ThreatIntensity` (0–1), `ThreatColor` (deep red)
- **Look:** Scene darkens toward the edges and tints red as intensity rises
- **When:** `UGameplayVfxSubsystem` samples princess tower HP each tick; intensity ramps when HP &lt; 70%
- **Why it helps:** Communicates danger without UI spam — the screen itself warns that the princess is in trouble

## Integration

- `UGameplayVfxSubsystem` (tickable world subsystem): loads assets, maintains an unbound `APostProcessVolume` with a MID of `M_PP_TowerThreat`, updates `ThreatIntensity`, spawns Cascade/Niagara + `AUpgradeSparkBurstActor`
- Upgrade hook in `ADefenderPlacementManagerBase::PurchaseClassUpgrade`
- Projectile trail / impact: `UProjectileTrailComponent` attached in `UCombatDirectorSubsystem::FireProjectile`; impact burst in `ApplyShotHit` (gold default, frost cyan, shaman green)

## How to verify

1. Run `Tools/CreatePart3VFX.py` once (creates `/Game/VFX/*`)
2. Optionally finish Cascade/Niagara emitter graphs (above)
3. PIE → upgrade a defender type → sparks at units
4. Let enemies damage the tower → red vignette grows as HP falls
