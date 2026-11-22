# Drift Horizon asset pipeline (flat colour, low poly, one shared palette)

Every model is painted from ONE texture, `Resources/Textures/palette.tga`. That is what keeps cars, buildings and props
looking like they belong together, and it means **nobody ever UV-unwraps anything**.

```
Tools/palette.json ──make_palette.py──▶ Resources/Textures/palette.tga      (the one shared texture)
        │
        └──dh_setup_palette.py (in Blender)──▶ materials  dh_red, dh_glass, dh_wall_cream, ...
                                                      │  you give each FACE one of these
Blender collection  dh_car_sedan ──dh_export.py──▶ Resources/Models/car_sedan.txt  + car_sedan.meta
                                                      │
                          AssetManager::GetModel("car_sedan")   (loads once, shares the palette texture)
```

## Rules for models
* 1 Blender unit = 1 metre. Model at the world origin, sitting on the ground (Z = 0), centred.
* Vehicles face **+Y** in Blender (= +Z "forward" in the game).
* One collection per asset, named `dh_<asset>` (e.g. `dh_car_sedan`, `dh_bld_shop_a`).
* Colour = material. Only `dh_<colour>` materials from the palette are allowed; the exporter rejects anything else.
* Flat shading by default (set "Shade Smooth" on faces that should be round). Keep it low poly (warning above 6000 triangles).
* Empties named `anchor_<name>` (e.g. `anchor_wheel_fl`) are written to the `.meta` file for the game to read.

## Adding / changing colours
Edit `Tools/palette.json` and **only append** (a colour's position is its swatch number; reordering recolours every model),
then run `python Tools/make_palette.py` and re-run `dh_setup_palette.py` in Blender.

## Using a model in the game
`const ModelAsset* a = renderer->GetAssets()->GetModel("car_sedan");` -> `a->model` for a `RenderItem`,
`a->boundsMin/boundsMax` for collision boxes, `a->anchors["wheel_fl"]` for points on the model. Returns nullptr when the
file does not exist, so always have a fallback.

## Test model without Blender
`python Tools/make_test_car.py [colour]` writes `car_test` (box car) in exactly the same format as the exporter.
