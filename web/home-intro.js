// Mark the homepage before its stylesheet can paint, preventing an unanimated flash.
if (location.pathname === '/' && !matchMedia('(prefers-reduced-motion: reduce)').matches) {
  document.documentElement.classList.add('home-hero-intro');
}

document.addEventListener('DOMContentLoaded', () => {
  const dialog = document.getElementById('codaris-flag-explorer');
  if (!dialog || typeof dialog.showModal !== 'function') return;

  const closeButton = dialog.querySelector('[data-flag-explorer-close]');
  const documentRoot = document.documentElement;
  let invoker = null;
  let previousOverflow = '';
  let previousScrollbarGutter = '';

  document.querySelectorAll('[data-flag-explorer-open]').forEach((trigger) => {
    trigger.addEventListener('click', () => {
      if (dialog.open) return;
      invoker = trigger;
      previousOverflow = documentRoot.style.overflow;
      previousScrollbarGutter = documentRoot.style.scrollbarGutter;
      const hadVerticalScrollbar = window.innerWidth > documentRoot.clientWidth;
      dialog.showModal();
      documentRoot.style.overflow = 'hidden';
      if (hadVerticalScrollbar) documentRoot.style.scrollbarGutter = 'stable';
      closeButton?.focus({ preventScroll: true });
    });
  });

  closeButton?.addEventListener('click', () => dialog.close());
  dialog.addEventListener('click', (event) => {
    if (event.target === dialog) dialog.close();
  });
  dialog.addEventListener('close', () => {
    documentRoot.style.overflow = previousOverflow;
    documentRoot.style.scrollbarGutter = previousScrollbarGutter;
    if (invoker?.isConnected) invoker.focus({ preventScroll: true });
    invoker = null;
  });
});
