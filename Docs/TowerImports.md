# Tower Imports (Blender)

Source files live in `Content/Imports/Towers` (folder label: **Imports**).

| Source `.blend` | FBX / StaticMesh | Used by |
|-----------------|------------------|---------|
| `PrincessTower.blend` | `/Game/Imports/Towers/PrincessTower` | `BP_PrincessTower` |
| `ArcherTower.blend` | `/Game/Imports/Towers/ArcherTower` | `BP_RoyalArcher` |
| `IceTower.blend` | `/Game/Imports/Towers/IceTower` | `BP_FrostArcher` |
| `PaladinTower.blend` | `/Game/Imports/Towers/PaladinTower` | `BP_RoyalGuardian` |

Runtime: `UTowerFabMeshComponent` hides the old cylinder and attaches the imported static mesh.

## Re-export / re-import

```bat
"C:\Program Files\Blender Foundation\Blender 4.5\blender.exe" --background --python Tools\ExportTowerBlendsToFbx.py
UnrealEditor-Cmd ... -ExecutePythonScript=Tools\ImportTowerMeshes.py
```
