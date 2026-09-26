/* Minimal same-origin session probe for the shared header; no account data is read. */
(() => {
  const nav = document.querySelector('[data-auth-nav]');
  if (!nav) return;

  const status = nav.querySelector('[data-auth-status]');
  const loggedOut = [...nav.querySelectorAll('[data-auth-logged-out]')];
  const loggedIn = [...nav.querySelectorAll('[data-auth-logged-in]')];
  const dashboard = nav.querySelector('a[href^="/dashboard/"]');
  const onDashboard = /^\/dashboard(?:\/|$)/.test(location.pathname);
  const joinCallsToAction = [...document.querySelectorAll('a[href]')].flatMap(anchor => {
    // The shared header has its own signed-in/out controls and should not be
    // relabelled as a page CTA while those controls are switching states.
    if (anchor.closest('[data-auth-nav]')) return [];
    const target = new URL(anchor.href, location.href);
    if (target.origin !== location.origin || target.pathname !== '/join/') return [];
    const plainLabel = anchor.textContent.replace(/[↗→↓]/g, '').replace(/\s+/g, ' ').trim();
    if (plainLabel.toLowerCase() !== 'join the coalition') return [];
    const labelNode = [...anchor.childNodes].find(node =>
      node.nodeType === Node.TEXT_NODE && /join the coalition/i.test(node.nodeValue));
    return labelNode ? [{anchor, labelNode, originalHref: anchor.getAttribute('href'), originalLabel: labelNode.nodeValue}] : [];
  });
  let lastCheckedAt = 0;
  let stateVersion = 0;
  let sessionRequest = null;

  function renderAuthState(state) {
    stateVersion += 1;
    nav.dataset.authState = state;
    const authenticated = state === 'authenticated';
    // Keep the auth area quiet until the session probe resolves. This avoids
    // briefly showing signed-out actions to members on every page navigation.
    // If the API is unavailable, render the signed-out actions as a fallback.
    const pending = state === 'loading';
    loggedOut.forEach(link => { link.hidden = authenticated || pending; });
    loggedIn.forEach(link => {
      link.hidden = !authenticated || pending;
      if (link === dashboard && onDashboard) link.setAttribute('aria-current', 'page');
      else link.removeAttribute('aria-current');
    });
    joinCallsToAction.forEach(({anchor, labelNode, originalHref, originalLabel}) => {
      anchor.setAttribute('href', authenticated ? '/dashboard/#overview' : originalHref);
      labelNode.nodeValue = authenticated
        ? originalLabel.replace(/join the coalition/i, 'Dashboard')
        : originalLabel;
    });
    if (status) status.textContent = state === 'unknown' ? 'Account status is unavailable.' : '';
  }

  async function refreshAuthState(force) {
    if (document.hidden) return;
    if (!force && Date.now() - lastCheckedAt < 30000) return;
    if (sessionRequest) return sessionRequest;
    lastCheckedAt = Date.now();
    const requestVersion = stateVersion;
    const controller = new AbortController();
    const timeout = window.setTimeout(() => controller.abort(), 5000);
    sessionRequest = (async () => {
      try {
        const response = await fetch('/api/session', {
          credentials: 'same-origin',
          cache: 'no-store',
          headers: { Accept: 'application/json' },
          signal: controller.signal,
        });
        if (requestVersion !== stateVersion) return;
        if (response.status === 204) renderAuthState('authenticated');
        else if (response.status === 401) renderAuthState('unauthenticated');
        else renderAuthState('unknown');
      } catch {
        if (requestVersion === stateVersion) renderAuthState('unknown');
      } finally {
        window.clearTimeout(timeout);
        sessionRequest = null;
      }
    })();
    return sessionRequest;
  }

  document.addEventListener('codaris-member-profile', () => renderAuthState('authenticated'));
  document.addEventListener('codaris-auth-required', () => renderAuthState('unauthenticated'));
  // A page restored from the back/forward cache can carry an old signed-out
  // header even after login in another page. Recheck on every return to it.
  window.addEventListener('pageshow', () => refreshAuthState(true));
  window.addEventListener('focus', () => refreshAuthState(true));
  document.addEventListener('visibilitychange', () => {
    if (!document.hidden) refreshAuthState(true);
  });
  refreshAuthState(true);
})();
