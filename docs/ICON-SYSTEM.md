# CODARIS icon system

CODARIS uses a small, locally served SVG sprite extracted from Lucide v1.48.0.
The source SVG subset is in `web/assets/icons/lucide-1.48.0/`; the build script
`scripts/build-lucide-sprite.py` creates `build/client/assets/icons/lucide.svg`.
The sprite is copied with its license into static production output. There is
no runtime CDN request, JavaScript icon library, or framework dependency.

Lucide is licensed under ISC. The pinned upstream license file is included at
`web/assets/icons/LICENSE`. It also retains the Feather-derived icon MIT notice
included upstream. The exact selected icons are listed in the sprite builder.

Use icons only where they help identify a navigation area, action, status, or
technical category. Reference them as decorative SVGs because nearby visible
text supplies their meaning:

```html
<svg class="icon icon--sm" aria-hidden="true" focusable="false">
  <use href="/assets/icons/lucide.svg#layout-dashboard"></use>
</svg>
```

Icon-only controls need an accessible name and title. Keep labels beside
important status icons. The common `.icon` classes use `currentColor`; parent
controls determine normal, hover, and active colors. Legal pages remain
primarily text.
