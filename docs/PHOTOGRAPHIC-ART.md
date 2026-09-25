# Photographic hero artwork

Created with the built-in image generator, using a raster rendering of `web/assets/emblem.svg` as the logo reference. These are fictional photographic-style scenes, not photographs of actual CODARIS facilities or staff. Generated emblems approximate the supplied logo; the page mastheads retain the exact SVG.

Actual source dimensions: 1672 × 941 for all three. The generator did not return requested native 4K. WebP exports use quality 95. The previous cinematic files remain available but are no longer referenced by the pages.

## Assets and final prompts

### mission

Saved asset: `web/assets/mission-photo.webp`

Use case: photorealistic-natural.
Create a new 3840x2160 landscape editorial photograph for a CODARIS website hero. Image 1 is the exact CODARIS emblem reference, not a scene to reproduce. Preserve its outlined hexagonal silhouette, angular inner C, and narrow right-hand accent, with cyan #59e4eb on charcoal. Use it faithfully on a few modest embroidered shoulder patches or a small physical wall sign. Do not invent an alternative logo or add text to the emblem.
Composition: eye-level wide architectural photograph, people and key activity central/right, quieter dark architectural space in the left third for a separate website headline. Show a believable present-day workplace. Real human proportions, candid expressions, natural skin texture and hands. Navy and charcoal surfaces, subtle cyan brand accents, realistic neutral overhead and window light. Crisp fine detail without oversharpening; deep enough focus to read the environment. Real cloth weave, slight everyday imperfections, practical equipment and restrained reflections.
Avoid game-engine rendering, CGI aesthetics, sci-fi architecture, holograms, artificial glow, theatrical haze, plastic skin, weapons, fake dashboards, unrelated brand logos, watermarks, and text overlays. This is fictional brand concept photography, not documentary evidence of real facilities or personnel.

Scene: A small group of four engineers at a practical conference table in a secure technology operations room, reviewing work on normal laptops and one wall-mounted world-map display. Charcoal work shirts with small exact-reference CODARIS shoulder patches. One near-right engineer seen in three-quarter profile, other colleagues naturally discussing and typing. Acoustic wall panels, cable trays, notebooks, coffee cups and normal office chairs. A focused, quiet collaborative briefing, photographed with natural mixed daylight and overhead lighting.

### global-reach

Saved asset: `web/assets/global-reach-photo.webp`

Use case: photorealistic-natural.
Create a new 3840x2160 landscape editorial photograph for a CODARIS website hero. Image 1 is the exact CODARIS emblem reference, not a scene to reproduce. Preserve its outlined hexagonal silhouette, angular inner C, and narrow right-hand accent, with cyan #59e4eb on charcoal. Use it faithfully on a few modest embroidered shoulder patches or a small physical wall sign. Do not invent an alternative logo or add text to the emblem.
Composition: eye-level wide architectural photograph, people and key activity central/right, quieter dark architectural space in the left third for a separate website headline. Show a believable present-day workplace. Real human proportions, candid expressions, natural skin texture and hands. Navy and charcoal surfaces, subtle cyan brand accents, realistic neutral overhead and window light. Crisp fine detail without oversharpening; deep enough focus to read the environment. Real cloth weave, slight everyday imperfections, practical equipment and restrained reflections.
Avoid game-engine rendering, CGI aesthetics, sci-fi architecture, holograms, artificial glow, theatrical haze, plastic skin, weapons, fake dashboards, unrelated brand logos, watermarks, and text overlays. This is fictional brand concept photography, not documentary evidence of real facilities or personnel.

Scene: An international communications centre in a real office building at blue hour. Two engineers coordinate at practical desks with ordinary monitors and a large flat world-map display on the rear wall. Through the windows is a believable city skyline. Modest exact-reference CODARIS patches on technical overshirts. The scene suggests people collaborating across countries without depicting a space station, planetary window, glowing globe or impossible technology.

### ecosystem

Saved asset: `web/assets/ecosystem-photo.webp`

Use case: photorealistic-natural.
Create a new 3840x2160 landscape editorial photograph for a CODARIS website hero. Image 1 is the exact CODARIS emblem reference, not a scene to reproduce. Preserve its outlined hexagonal silhouette, angular inner C, and narrow right-hand accent, with cyan #59e4eb on charcoal. Use it faithfully on a few modest embroidered shoulder patches or a small physical wall sign. Do not invent an alternative logo or add text to the emblem.
Composition: eye-level wide architectural photograph, people and key activity central/right, quieter dark architectural space in the left third for a separate website headline. Show a believable present-day workplace. Real human proportions, candid expressions, natural skin texture and hands. Navy and charcoal surfaces, subtle cyan brand accents, realistic neutral overhead and window light. Crisp fine detail without oversharpening; deep enough focus to read the environment. Real cloth weave, slight everyday imperfections, practical equipment and restrained reflections.
Avoid game-engine rendering, CGI aesthetics, sci-fi architecture, holograms, artificial glow, theatrical haze, plastic skin, weapons, fake dashboards, unrelated brand logos, watermarks, and text overlays. This is fictional brand concept photography, not documentary evidence of real facilities or personnel.

Scene: Three engineers collaborating at a real electronics workbench in a shared development laboratory. One inspects a small circuit board, another uses a laptop, a colleague reviews a printed schematic. Practical test instruments, orderly tools, labelled storage without legible branding, and a glass partition to a small server room. Modest exact-reference CODARIS patches on everyday technical overshirts. Comfortable, human, quietly productive; daylight and ordinary task lights.

## Stable framing

Hero photographs use an absolutely positioned, fixed image box and responsive `object-position`. The camera zoom/pan animation was removed because restarting it on page entry visibly changed the crop. Localized SVG overlays animate the Mission map and front laptop, the Global Reach wall display, and Ecosystem server LEDs. The photograph and overlays share a 1672 × 941 coordinate system inside a CSS container sized to reproduce the existing cover crop at each breakpoint. Screen masks exclude foreground people. CSS opacity pulses and a faint scan line simulate activity; they do not represent live data. Pause screen activity freezes all overlay animations at their current frames. Reduced-motion mode hides and stops the effects. No animation library or JavaScript is needed.
