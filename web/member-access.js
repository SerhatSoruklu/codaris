/* Keep the original protected page as the sign-in destination. */
(() => {
  const login = document.querySelector('[data-member-gate-login]');
  if (!login) return;
  const fallback = '/topics/';
  let destination = fallback;
  const current = location.pathname + location.search + location.hash;
  if (location.pathname !== '/member-sign-in-required/' && current.startsWith('/') && !current.startsWith('//') && !current.includes('\\')) {
    try {
      const target = new URL(current, location.origin);
      if (target.origin === location.origin && target.pathname !== '/login/' && !target.pathname.startsWith('/api/')) {
        destination = target.pathname + target.search + target.hash;
      }
    } catch {}
  }
  const signIn = new URL('/login/', location.origin);
  signIn.searchParams.set('return_to', destination);
  login.href = signIn.pathname + signIn.search;
})();
