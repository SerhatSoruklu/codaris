# Globe geography

Source: Natural Earth, 1:50m Admin 0 countries (public domain).
https://www.naturalearthdata.com/downloads/50m-cultural-vectors/
https://github.com/nvkelso/natural-earth-vector/blob/master/geojson/ne_50m_admin_0_countries.geojson

The SVG uses an orthographic projection centred on 35° W, 30° N, with real country outlines, hemisphere clipping, ocean shading and an atmosphere. It starts at the Atlantic view and supports drag/touch and arrow-key rotation through a C/Wasm orthographic projection. Orientation stays where released; Reset view or Home restores the starting position. This is interactive 3D presented in SVG, not 4D or WebGL. CSS animates illustrative connection routes, not measured traffic. Callout coordinates identify the illustrative New York (40.70° N, 74.00° W) and London (51.50° N, 0.12° W) endpoints, not country centroids. No runtime map service or new production dependency is used.

“The United British Isles” is the requested custom regional label, not a country name. The blue highlight includes Great Britain (including Scotland), Ireland, and nearby islands present at the dataset scale, including Isle of Man and Channel Islands. It excludes UK overseas territories outside this region. Existing country data and form options retain their country names. USA polygons are red. Small islands are limited by the 1:50m source resolution.

To regenerate, download the source GeoJSON and run from the repository root:

```bash
python3 scripts/assets/generate-globe.py /path/to/ne_50m_admin_0_countries.geojson
./scripts/build-client.sh
```

The standard-library-only generator updates `web/assets/network.svg`, the marked inline SVG in `web/pages/home.html`, and compiled geographic fixtures in `src/client/globe_data.h`. `src/client/globe.h` owns rotation, projection, hemisphere clipping, route positioning, and hidden-side marker visibility. The browser host only forwards pointer/keyboard events and copies projected geometry. Inline presentation enables the native checkbox/CSS pause control without JavaScript application logic. Reduced-motion preferences disable animation automatically.

Mouse-wheel zoom is C-owned, bounded to 0.8×–2.5×, and retains the current orientation. +/− provide keyboard zoom. Reset view/Home resets both zoom and orientation. Wheel scrolling outside the SVG and browser Ctrl+wheel zoom remain native. Drag deltas account for the current SVG camera scale.

Initial SVG paths use the same sampled outlines as the C redraw fixtures, retaining detailed British Isles outlines while reducing homepage transfer size. The responsive globe frame has an explicit border and rounded viewport at every camera zoom. Browser Ctrl/Cmd keyboard zoom shortcuts remain native.

Country outlines are grouped by Natural Earth feature (242 countries/territories). Hover, keyboard focus, or touch selects a group through a C-owned ID. A separate SVG `use` layer scales and lifts its outlines with a shadow, while the underlying hit areas remain stationary. USA uses a stronger red glow; British Isles features use a stronger blue glow. This is a CSS depth effect, not extruded 3D geometry. Decorative overlays ignore pointer events. Reduced-motion preferences remove scaling and transitions; the colour highlight remains. Rotation clears selection and updates keyboard availability for visible countries. Escape clears the highlight.
