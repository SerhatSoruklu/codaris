// Mark the homepage before its stylesheet can paint, preventing an unanimated flash.
if (location.pathname === '/' && !matchMedia('(prefers-reduced-motion: reduce)').matches) {
  document.documentElement.classList.add('home-hero-intro');
}
