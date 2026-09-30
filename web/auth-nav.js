/* Minimal same-origin session probe for the shared header; no account data is read. */
(() => {
// Play the home entrance on each page load; reveal below-fold content when seen.
if (location.pathname === '/' && !matchMedia('(prefers-reduced-motion: reduce)').matches) {
  const meaning = document.querySelector('.meaning-overview');
  if (meaning && 'IntersectionObserver' in window) {
    const meaningObserver = new IntersectionObserver(entries => {
      if (!entries.some(entry => entry.isIntersecting)) return;
      document.documentElement.classList.add('home-meaning-intro');
      meaning.classList.add('home-meaning-intro');
      meaningObserver.disconnect();
    }, {threshold: 0.05, rootMargin: '0px 0px 12% 0px'});
    meaningObserver.observe(meaning);
  } else if (meaning) {
    document.documentElement.classList.add('home-meaning-intro');
    meaning.classList.add('home-meaning-intro');
  }
}
// Shared transition screen. Network phases are reported by the request owner;
// dashboard completion waits for the authenticated profile response.
const codarisLoader = (() => {
  const screen = document.querySelector('[data-codaris-loader]');
  if (!screen) return null;
  const titles = {login: 'Welcome back', register: 'Creating your account', logout: 'Signing you out', dashboard: 'Opening your workspace', delete: 'Permanently deleting your account'};
  const title = screen.querySelector('[data-codaris-loader-title]');
  const phase = screen.querySelector('[data-codaris-loader-phase]');
  const progress = screen.querySelector('[data-codaris-loader-progress]');
  const bar = screen.querySelector('[data-codaris-loader-bar]');
  const percent = screen.querySelector('[data-codaris-loader-percent]');
  const fill = screen.querySelector('[data-codaris-loader-fill]');
  const action = screen.querySelector('[data-codaris-loader-action]');
  let active = false;
  let current = 0;
  let navigationLoaded = false;
  let requestComplete = false;
  let activeOperation = '';
  let introDone = false;
  let startedAt = 0;
  let introDuration = 3000;
  let pendingNetworkProgress = 0;
  let pendingNetworkMessage = '';
  let networkIndeterminate = false;
  let animationFrame = 0;
  let autoHideTimer = 0;
  const track = screen.querySelector('.codaris-loader__track');
  function hide() {
    active = false;
    if (animationFrame) window.cancelAnimationFrame(animationFrame);
    if (autoHideTimer) window.clearTimeout(autoHideTimer);
    document.body.removeAttribute('data-codaris-busy');
    screen.hidden = true;
    screen.setAttribute('aria-hidden', 'true');
  }
  function paint(value, message) {
    current = Math.max(0, Math.min(100, Math.floor(value)));
    const ratio = current / 100;
    screen.style.setProperty('--codaris-loader-progress', ratio);
    bar.style.transform = `scaleX(${ratio})`;
    fill.setAttribute('y', String(40 * (1 - ratio)));
    fill.setAttribute('height', String(40 * ratio));
    percent.textContent = String(current);
    progress.setAttribute('aria-valuenow', String(current));
    if (message) phase.textContent = message;
  }
  function working(message) {
    if (!active) return;
    networkIndeterminate = true;
    pendingNetworkMessage = message || pendingNetworkMessage;
    if (!introDone) return;
    track.classList.add('is-indeterminate');
    percent.hidden = true;
    progress.removeAttribute('aria-valuenow');
    progress.setAttribute('aria-label', 'Network activity');
    if (message) phase.textContent = message;
  }
  function initialMessage(kind) {
    return ({login: 'Preparing your sign-in request…', register: 'Preparing your account request…', logout: 'Preparing your sign-out request…', dashboard: 'Preparing your workspace…', delete: 'Removing your profile and signing you out…'})[kind] || 'Preparing your request…';
  }
  function showNetworkProgress() {
    track.classList.remove('is-indeterminate');
    percent.hidden = false;
    progress.setAttribute('aria-label', 'Transfer progress');
    paint(50 + pendingNetworkProgress * 0.5, pendingNetworkMessage);
  }
  function finishIntro() {
    if (introDone || !active) return;
    introDone = true;
    if (networkIndeterminate) working(pendingNetworkMessage);
    else showNetworkProgress();
    if (requestComplete && (navigationLoaded || !['login', 'logout', 'register', 'dashboard'].includes(activeOperation))) finishRoute();
  }
  function finishRoute() {
    if (autoHideTimer) window.clearTimeout(autoHideTimer);
    autoHideTimer = window.setTimeout(hide, 1800);
  }
  function start(kind, heading = '', timing = {}) {
    if (typeof kind !== 'string' || (!titles[kind] && !heading)) return;
    active = true;
    current = 0;
    activeOperation = kind;
    navigationLoaded = false;
    requestComplete = false;
    introDone = false;
    startedAt = Number.isFinite(timing.startedAt) ? timing.startedAt : Date.now();
    const randomDuration = window.crypto.getRandomValues(new Uint32Array(1))[0] % 2001;
    introDuration = Number.isFinite(timing.introDuration) ? Math.max(2000, Math.min(4000, timing.introDuration)) : 2000 + randomDuration;
    pendingNetworkProgress = 0;
    pendingNetworkMessage = initialMessage(kind);
    networkIndeterminate = false;
    title.textContent = heading || titles[kind];
    if (action) action.hidden = true;
    screen.hidden = false;
    screen.setAttribute('aria-hidden', 'false');
    document.body.dataset.codarisBusy = 'true';
    track.classList.remove('is-indeterminate');
    percent.hidden = false;
    progress.setAttribute('aria-label', 'Request progress');
    const tick = () => {
      if (!active || introDone) return;
      const elapsed = Math.max(0, Date.now() - startedAt);
      paint(Math.min(50, elapsed / introDuration * 50), initialMessage(kind));
      if (elapsed >= introDuration) finishIntro();
      else animationFrame = window.requestAnimationFrame(tick);
    };
    tick();
  }
  function update(value, message) {
    if (!active) return;
    pendingNetworkProgress = Math.max(0, Math.min(100, value));
    pendingNetworkMessage = message || pendingNetworkMessage;
    networkIndeterminate = false;
    if (introDone) showNetworkProgress();
  }
  function complete(kind, success) {
    if (!active) return;
    // The profile request is the real readiness check after a dashboard route
    // loads. Other account operations complete against their own response.
    if (kind === 'me' && activeOperation === 'dashboard') kind = 'dashboard';
    if (!success) {
      hide();
      return;
    }
    requestComplete = true;
    pendingNetworkProgress = 100;
    pendingNetworkMessage = ({register: 'Account created. Opening confirmation…', login: 'Sign-in confirmed. Opening your workspace…', logout: 'Sign-out confirmed. Finishing…', dashboard: 'Your workspace is ready…'})[kind] || 'Request complete.';
    networkIndeterminate = false;
    if (!introDone) {
      if (kind === 'login' || kind === 'logout' || kind === 'register') {
        try { sessionStorage.setItem('codaris-loader-navigation', JSON.stringify({kind, startedAt, introDuration})); } catch {}
      }
      return;
    }
    track.classList.remove('is-indeterminate');
    percent.hidden = false;
    progress.setAttribute('aria-label', 'Request progress');
    paint(100, ({register: 'Account created. Opening confirmation…', login: 'Sign-in confirmed. Opening your workspace…', logout: 'Sign-out confirmed. Finishing…', dashboard: 'Your workspace is ready…'})[kind] || 'Request complete.');
    if (kind === 'login' || kind === 'logout' || kind === 'register') {
      try { sessionStorage.setItem('codaris-loader-navigation', JSON.stringify({kind, startedAt, introDuration})); } catch {}
      return;
    }
    if (kind !== 'dashboard' || navigationLoaded) finishRoute();
  }
  function setAction(label, href) {
    if (!action || typeof label !== 'string' || typeof href !== 'string') return;
    const destination = new URL(href, location.href);
    if (destination.origin !== location.origin || !destination.pathname.startsWith('/')) return;
    action.textContent = label;
    action.href = destination.pathname + destination.search + destination.hash;
    action.hidden = false;
  }
  function setContinueAction(label) {
    if (!action) return;
    action.textContent = label;
    action.href = location.pathname + location.search + location.hash;
    action.hidden = false;
    action.onclick = event => {
      event.preventDefault();
      // The destination is already loaded. Let people dismiss the transition
      // immediately while any remaining page data continues to load normally.
      hide();
    };
  }
  function routeAction(kind) {
    if (kind === 'logout') return 'Finish signing out now';
    if (kind === 'register') return 'Continue to email verification now';
    if (kind === 'dashboard' && /^\/dashboard(?:\/|$)/.test(location.pathname)) return 'Go to dashboard now';
    return 'Go to this page now';
  }
  try {
    const stored = sessionStorage.getItem('codaris-loader-navigation');
    if (stored) {
      sessionStorage.removeItem('codaris-loader-navigation');
      const navigation = JSON.parse(stored);
      // A successful sign-in commonly lands directly on the dashboard. Keep
      // its loader active until the profile request confirms workspace access.
      const isDashboard = /^\/dashboard(?:\/|$)/.test(location.pathname);
      const kind = ((navigation.kind === 'login' || navigation.kind === 'dashboard') && isDashboard)
        ? 'dashboard'
        : navigation.kind === 'dashboard' ? 'login' : navigation.kind;
      start(kind, '', navigation);
      const actionLabel = routeAction(kind);
      if (actionLabel) setContinueAction(actionLabel);
      window.addEventListener('load', () => {
        navigationLoaded = true;
        if (kind !== 'dashboard') {
          requestComplete = true;
          pendingNetworkProgress = 100;
          pendingNetworkMessage = kind === 'logout' ? 'Signed out.' : kind === 'register' ? 'Confirmation page ready.' : 'Workspace page ready.';
          networkIndeterminate = false;
          if (introDone) showNetworkProgress();
        }
        const loadedAction = routeAction(kind);
        if (loadedAction) setContinueAction(loadedAction);
        if (introDone && requestComplete) finishRoute();
      }, {once: true});
    }
  } catch {}
  function navigateToDashboard(event) {
    const anchor = event.target.closest('a[href]');
    if (!anchor || anchor.target || anchor.hasAttribute('download')) return;
    const destination = new URL(anchor.href, location.href);
    if (destination.origin !== location.origin || destination.pathname !== '/dashboard/' || location.pathname === '/dashboard/') return;
    start('dashboard');
    try { sessionStorage.setItem('codaris-loader-navigation', JSON.stringify({kind: 'dashboard', startedAt, introDuration})); } catch {}
  }
  document.addEventListener('click', navigateToDashboard, true);
  return {start, update, working, complete, setAction, isLoggingOut: () => activeOperation === 'logout', isActive: kind => active && activeOperation === kind};
})();
window.codarisLoader = codarisLoader;
  const nav = document.querySelector('[data-auth-nav]');
  if (!nav) return;

  const status = nav.querySelector('[data-auth-status]');
  const loggedOut = [...nav.querySelectorAll('[data-auth-logged-out]')];
  const loggedIn = [...nav.querySelectorAll('[data-auth-logged-in]')];
  const dashboard = nav.querySelector('a[href^="/dashboard/"]');
  const onDashboard = /^\/dashboard(?:\/|$)/.test(location.pathname);
  const memberRoute = document.body.dataset.memberRoute === 'true';
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
    if (!labelNode) return [];
    anchor.classList.add('auth-state-cta');
    return [{anchor, labelNode, originalHref: anchor.getAttribute('href'), originalLabel: labelNode.nodeValue}];
  });
  let lastCheckedAt = 0;
  let stateVersion = 0;
  let sessionRequest = null;
  const memberMenu = nav.querySelector('[data-member-menu]');
  const memberMenuTrigger = nav.querySelector('[data-member-menu-trigger]');
  const memberMenuPanel = nav.querySelector('[data-member-menu-panel]');
  const memberMenuName = nav.querySelector('[data-member-menu-name]');
  const memberSessionAge = nav.querySelector('[data-member-session-age]');
  let sessionStartedAt = 0;
  let memberMenuCloseTimer = 0;

  function paintMemberSummary(profile) {
    if (memberMenuName && profile?.name) memberMenuName.textContent = profile.name;
    const startedAt = Number(profile?.session_started_at);
    if (Number.isFinite(startedAt) && startedAt > 0) sessionStartedAt = startedAt * 1000;
    if (!memberSessionAge || !sessionStartedAt) return;
    const totalMinutes = Math.max(0, Math.floor((Date.now() - sessionStartedAt) / 60000));
    const days = Math.floor(totalMinutes / 1440);
    const hours = Math.floor((totalMinutes % 1440) / 60);
    const minutes = totalMinutes % 60;
    const duration = days ? `${days}d ${hours}h` : hours ? `${hours}h ${minutes}m` : `${minutes}m`;
    memberSessionAge.textContent = `Signed in for ${duration}`;
  }
  window.setInterval(() => paintMemberSummary({}), 60000);

  function closeMemberMenu(restoreFocus = false) {
    if (!memberMenuTrigger || !memberMenuPanel) return;
    memberMenuTrigger.setAttribute('aria-expanded', 'false');
    memberMenuPanel.removeAttribute('data-open');
    window.clearTimeout(memberMenuCloseTimer);
    memberMenuCloseTimer = window.setTimeout(() => {
      if (memberMenuTrigger.getAttribute('aria-expanded') === 'false') memberMenuPanel.hidden = true;
    }, 180);
    if (restoreFocus) memberMenuTrigger.focus();
  }
  memberMenuTrigger?.addEventListener('click', () => {
    const opening = memberMenuTrigger.getAttribute('aria-expanded') !== 'true';
    window.clearTimeout(memberMenuCloseTimer);
    memberMenuTrigger.setAttribute('aria-expanded', String(opening));
    if (opening) {
      memberMenuPanel.hidden = false;
      window.requestAnimationFrame(() => {
        if (memberMenuTrigger.getAttribute('aria-expanded') === 'true') memberMenuPanel.setAttribute('data-open', 'true');
      });
    } else {
      memberMenuPanel.removeAttribute('data-open');
      memberMenuCloseTimer = window.setTimeout(() => { memberMenuPanel.hidden = true; }, 180);
    }
  });
  memberMenu?.addEventListener('click', event => {
    if (event.target.closest('a')) closeMemberMenu();
  });
  document.addEventListener('pointerdown', event => {
    if (memberMenu && !memberMenu.contains(event.target)) closeMemberMenu();
  });
  document.addEventListener('keydown', event => {
    if (event.key === 'Escape' && memberMenuTrigger?.getAttribute('aria-expanded') === 'true') {
      closeMemberMenu(true);
    }
  });

  function renderAuthState(state) {
    stateVersion += 1;
    nav.dataset.authState = state;
    const authenticated = state === 'authenticated';
    if (!authenticated) {
      closeMemberMenu();
      sessionStartedAt = 0;
      if (memberMenuName) memberMenuName.textContent = 'Member';
      if (memberSessionAge) memberSessionAge.textContent = 'Session duration unavailable';
    }
    if (authenticated && location.pathname === '/login/') {
      const requested = new URLSearchParams(location.search).get('return_to');
      if (requested && requested.startsWith('/') && !requested.startsWith('//') && !requested.includes('\\')) {
        const target = new URL(requested, location.origin);
        if (target.origin === location.origin && target.pathname !== '/login/' && !target.pathname.startsWith('/api/')) {
          location.replace(target.pathname + target.search + target.hash);
          return;
        }
      }
    }
    if (memberRoute) {
      if (authenticated) document.body.dataset.memberRouteAuthenticated = 'true';
      else delete document.body.dataset.memberRouteAuthenticated;
      if (state === 'unauthenticated') {
        const returnTo = location.pathname + location.search + location.hash;
        if (onDashboard) {
          try { sessionStorage.setItem('codaris-loader-navigation', JSON.stringify({kind: 'dashboard', startedAt: Date.now()})); } catch {}
        }
        location.replace('/login/?return_to=' + encodeURIComponent(returnTo));
        return;
      }
    }
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
    if (state === 'authenticated' || state === 'unauthenticated') document.body.dataset.authStateReady = 'true';
    if (status) status.textContent = state === 'unknown'
      ? (memberRoute ? 'Access could not be verified. Check your connection and refresh.' : 'Account status is unavailable.')
      : '';
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
        const response = await fetch(window.codarisApiUrl('/api/session'), {
          credentials: 'include',
          cache: 'no-store',
          headers: { Accept: 'application/json' },
          signal: controller.signal,
        });
        if (requestVersion !== stateVersion) return;
        if (response.status === 204) {
          renderAuthState('unauthenticated');
          return;
        }
        if (!response.ok) {
          renderAuthState('unknown');
          return;
        }
        const session = await response.json();
        if (session.authenticated === true) paintMemberSummary(session);
        renderAuthState(session.authenticated === true ? 'authenticated' : 'unauthenticated');
      } catch {
        if (requestVersion === stateVersion) renderAuthState('unknown');
      } finally {
        window.clearTimeout(timeout);
        sessionRequest = null;
      }
    })();
    return sessionRequest;
  }

  document.addEventListener('codaris-member-profile', event => {
    paintMemberSummary(event.detail);
    const paused = event.detail?.credential?.status === 'suspended';
    if (paused) document.body.dataset.membershipStatus = 'suspended';
    else delete document.body.dataset.membershipStatus;
    document.querySelectorAll('[data-membership-paused-warning]').forEach(warning => { warning.hidden = !paused; });
    renderAuthState('authenticated');
  });
  document.addEventListener('codaris-auth-required', () => renderAuthState('unauthenticated'));
  // A page restored from the back/forward cache can carry an old signed-out
  // header even after login in another page. Recheck on every return to it.
  window.addEventListener('pageshow', event => {
    if (memberRoute && event.persisted) delete document.body.dataset.memberRouteAuthenticated;
    refreshAuthState(true);
  });
  window.addEventListener('focus', () => refreshAuthState(true));
  document.addEventListener('visibilitychange', () => {
    if (!document.hidden) refreshAuthState(true);
  });
  refreshAuthState(true);
})();
