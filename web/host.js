/* Browser transport only. Account control and filtering live in C/Wasm.
 * ccall owns temporary UTF-8 input storage for the duration of each call. */
var Module = {
  // Explicit loading catches fetch failures in older packaged Emscripten SDKs.
  instantiateWasm: function (imports, receiveInstance) {
    fetch("/codaris.wasm", { credentials: "same-origin" })
      .then(function (response) {
        if (!response.ok) throw new Error("WebAssembly download failed");
        return response.arrayBuffer();
      })
      .then(function (bytes) {
        return WebAssembly.instantiate(bytes, imports);
      })
      .then(function (result) {
        receiveInstance(result.instance, result.module);
      })
      .catch(function () {
        Module.onAbort();
      });
    return {};
  },
  onAbort: function () {
    document.getElementById("runtime-status").textContent =
      "Interactive features unavailable. Please reload the page.";
    const search = document.getElementById("country-search");
    if (search) search.disabled = true;
    const languageSearch = document.getElementById("catalogue-search");
    if (languageSearch) languageSearch.disabled = true;
  const fields = document.getElementById("join-fields");
  if (fields) fields.disabled = true;
    const countryTrigger = document.getElementById('join-country-trigger');
    if (countryTrigger) countryTrigger.disabled = true;
    document.body.dataset.memberReady = "false";
    document.querySelectorAll("[data-account-fields], [data-account-button], [data-preview-fields], [data-preview-button]").forEach(e => e.disabled = true);
  },
};
document.getElementById("codaris-runtime")?.addEventListener("error", Module.onAbort);
document
  .getElementById("country-search")
  ?.addEventListener("input", function (event) {
    Module.ccall("codaris_filter", null, ["string"], [event.target.value]);
  });
const countryPickers = [...document.querySelectorAll('[data-country-picker]')].map(root => {
  const select = root.querySelector('[data-country-picker-select]');
  const trigger = root.querySelector('[data-country-picker-trigger]');
  const flag = root.querySelector('[data-country-picker-flag]');
  const value = root.querySelector('[data-country-picker-value]');
  const menu = root.querySelector('[data-country-picker-menu]');
  function updateCountrySelection() {
    const code = select?.selectedOptions[0]?.dataset.flag || '';
    if (flag) {
      flag.toggleAttribute('hidden', !code);
      if (code) flag.src = `/assets/country-flags/${code}.svg`;
      else flag.removeAttribute('src');
    }
    if (value) value.textContent = select?.selectedOptions[0]?.textContent || 'Select your country';
    menu?.querySelectorAll('[role="option"]').forEach(option => {
      option.setAttribute('aria-selected', String(option.dataset.value === (select?.value || '')));
    });
  }
  function closeCountryMenu(focusTrigger) {
    if (!menu || !trigger) return;
    menu.hidden = true;
    trigger.setAttribute('aria-expanded', 'false');
    if (focusTrigger) trigger.focus();
  }
  function openCountryMenu() {
    if (!menu || !trigger || trigger.disabled) return;
    menu.hidden = false;
    trigger.setAttribute('aria-expanded', 'true');
    const selected = menu.querySelector('[aria-selected="true"]') || menu.querySelector('[role="option"]');
    selected?.focus();
  }
  select?.addEventListener('change', updateCountrySelection);
  trigger?.addEventListener('click', () => {
    if (menu?.hidden) openCountryMenu(); else closeCountryMenu(false);
  });
  menu?.addEventListener('click', event => {
    const option = event.target.closest('[role="option"]');
    if (!option || !select) return;
    select.value = option.dataset.value;
    select.dispatchEvent(new Event('change', {bubbles: true}));
    closeCountryMenu(true);
  });
  menu?.addEventListener('keydown', event => {
    const options = [...menu.querySelectorAll('[role="option"]')];
    const index = options.indexOf(document.activeElement);
    if (event.key === 'Escape') { event.preventDefault(); closeCountryMenu(true); return; }
    if (event.key === 'Tab') { closeCountryMenu(false); return; }
    if (event.key === 'ArrowDown' || event.key === 'ArrowUp' || event.key === 'Home' || event.key === 'End') {
      event.preventDefault();
      const next = event.key === 'Home' ? 0 : event.key === 'End' ? options.length - 1 : (index + (event.key === 'ArrowDown' ? 1 : -1) + options.length) % options.length;
      options[next]?.focus();
    } else if (event.key === 'Enter' || event.key === ' ') {
      event.preventDefault(); document.activeElement?.click();
    }
  });
  updateCountrySelection();
  return {root, closeCountryMenu};
});
document.addEventListener('click', event => {
  countryPickers.forEach(picker => {
    if (!picker.root.contains(event.target)) picker.closeCountryMenu(false);
  });
});
// Development-only form preview; production remains inert and disabled.
// Native validity and UTF-16 lengths match HTML minlength/maxlength semantics.
document
  .querySelectorAll("#join-form input, #join-form textarea, #join-form select")
  .forEach(function (field) {
    function report(reveal) {
      if (document.getElementById("join-fields").disabled || !field.name || !field.id.startsWith("join-")) return;
      const validity = field.validity;
      const flags =
        (validity.valueMissing ? 1 : 0) |
        (validity.typeMismatch ? 2 : 0) |
        (validity.patternMismatch ? 4 : 0);
      Module.ccall(
        "codaris_validate_field",
        null,
        ["string", "number", "number", "number", "number", "number"],
        [
          field.name,
          field.value.length,
          field.minLength || 0,
          field.maxLength || 0,
          flags,
          reveal ? 1 : 0,
        ],
      );
    }
    field.addEventListener("input", function () {
      report(field.getAttribute("aria-invalid") === "true");
    });
    field.addEventListener("blur", function () {
      report(true);
    });
    field.addEventListener("invalid", function () {
      report(true);
    });
  });
document.querySelectorAll('[data-character-count]').forEach(field => {
  const counter = document.getElementById(field.dataset.characterCount);
  if (!counter) return;
  const update = () => {
    counter.textContent = `${field.value.length} / ${field.maxLength}`;
    counter.classList.toggle('near-limit', field.maxLength > 0 && field.value.length >= field.maxLength * .9);
  };
  field.addEventListener('input', update);
  update();
});
const applicationPassword = document.getElementById('application-password');
const applicationConfirm = document.getElementById('application-confirm');
function updateApplicationConfirmation() {
  if (!applicationPassword || !applicationConfirm || !applicationConfirm.value) return;
  const mismatch = applicationPassword.value !== applicationConfirm.value;
  if (!mismatch && applicationConfirm.getAttribute('aria-invalid') !== 'true') return;
  applicationConfirm.setAttribute('aria-invalid', String(mismatch));
  const error = document.getElementById('application-confirm-error');
  if (error) error.textContent = mismatch ? 'Passwords do not match.' : '';
}
applicationPassword?.addEventListener('input', updateApplicationConfirmation);
applicationConfirm?.addEventListener('input', updateApplicationConfirmation);

// Pointer transport only: projection, clipping and orientation live in C/Wasm.
const globe = document.querySelector('.network-globe');
let globePointer = null;
let globeLastX = 0, globeLastY = 0;
globe?.addEventListener('pointerdown', function (event) {
  if (globe.dataset.ready !== 'true' || !event.isPrimary || event.button !== 0) return;
  globePointer = event.pointerId;
  globeLastX = event.clientX; globeLastY = event.clientY;
  globe.setPointerCapture(event.pointerId);
  globe.focus({preventScroll: true});
  const country = event.target.closest('.globe-country');
  Module.ccall('codaris_globe_highlight', null, ['number'], [country ? Number(country.dataset.country) : -1]);
});
globe?.addEventListener('pointermove', function (event) {
  if (event.pointerId !== globePointer) return;
  const scale = globe.viewBox.baseVal.width / globe.getBoundingClientRect().width;
  Module.ccall('codaris_globe_rotate', null, ['number', 'number', 'number'],
    [(event.clientX-globeLastX)*scale, (event.clientY-globeLastY)*scale, 0]);
  globeLastX = event.clientX; globeLastY = event.clientY;
});
function releaseGlobe(event) { if (event.pointerId === globePointer) globePointer = null; }
globe?.addEventListener('pointerup', releaseGlobe);
globe?.addEventListener('pointercancel', releaseGlobe);
globe?.addEventListener('lostpointercapture', releaseGlobe);
globe?.addEventListener('keydown', function (event) {
  if (globe.dataset.ready !== 'true' || event.ctrlKey || event.metaKey) return;
  if (['+', '=', '-'].includes(event.key)) {
    event.preventDefault();
    Module.ccall('codaris_globe_zoom', null, ['number', 'number'], [event.key === '-' ? 120 : -120, 0]);
    return;
  }
  const moves = {ArrowLeft: [-25,0,0], ArrowRight: [25,0,0], ArrowUp: [0,-25,0], ArrowDown: [0,25,0], Home: [0,0,1]};
  if (!moves[event.key]) return;
  event.preventDefault();
  Module.ccall('codaris_globe_rotate', null, ['number','number','number'], moves[event.key]);
});
document.getElementById('globe-reset')?.addEventListener('click', function () {
  Module.ccall('codaris_globe_rotate', null, ['number','number','number'], [0,0,1]);
});

// Consume wheel events only over the ready globe. Preserve browser Ctrl+wheel zoom.
globe?.addEventListener('wheel', function (event) {
  if (globe.dataset.ready !== 'true' || event.ctrlKey || event.metaKey) return;
  event.preventDefault();
  Module.ccall('codaris_globe_zoom', null, ['number', 'number'], [event.deltaY, event.deltaMode]);
}, {passive: false});

// Country events transport IDs only; selection and validation stay in C.
globe?.addEventListener('pointerover', function (event) {
  if (globe.dataset.ready !== 'true' || globePointer !== null || event.pointerType === 'touch') return;
  const country = event.target.closest('.globe-country');
  Module.ccall('codaris_globe_highlight', null, ['number'], [country ? Number(country.dataset.country) : -1]);
});
globe?.addEventListener('pointerleave', function () {
  if (globe.dataset.ready === 'true' && !globe.contains(document.activeElement))
    Module.ccall('codaris_globe_highlight', null, ['number'], [-1]);
});
globe?.addEventListener('focusin', function (event) {
  if (globe.dataset.ready !== 'true') return;
  const country = event.target.closest('.globe-country');
  Module.ccall('codaris_globe_highlight', null, ['number'], [country ? Number(country.dataset.country) : -1]);
});
globe?.addEventListener('focusout', function (event) {
  if (globe.dataset.ready === 'true' && !globe.contains(event.relatedTarget))
    Module.ccall('codaris_globe_highlight', null, ['number'], [-1]);
});
globe?.addEventListener('keydown', function (event) {
  if (globe.dataset.ready === 'true' && event.key === 'Escape') {
    Module.ccall('codaris_globe_highlight', null, ['number'], [-1]);
    globe.focus({preventScroll: true});
  }
});

// Browser visibility transport for the CSS-only decorative hero background.
const rainHero = document.querySelector('.hero:has(.hero-rain), .login-scene, .join-scene');
if (rainHero && 'IntersectionObserver' in window) {
  let rainVisible = false;
  function syncRainVisibility() {
    rainHero.dataset.rainActive = String(rainVisible && !document.hidden);
  }
  const rainObserver = new IntersectionObserver(function (entries) {
    rainVisible = entries[0].isIntersecting;
    syncRainVisibility();
  });
  rainObserver.observe(rainHero);
  document.addEventListener('visibilitychange', syncRainVisibility);
}

// Membership preview: browser events and image decoding only; outcomes live in C.
function membershipReady() { return document.body.dataset.memberReady === 'true'; }
const dashboardTabs = [...document.querySelectorAll('.account-tabs [role="tab"][data-member-tab]')];
const participationDisclosure = document.getElementById('dashboard-participation-menu');
if (participationDisclosure) {
  const trigger = participationDisclosure.querySelector('.dashboard-disclosure-trigger');
  const panel = document.getElementById('dashboard-participation-links');
  let pointerActivation = false;
  let closeTimer = 0;
  function setParticipationMenu(open, returnFocus = false) {
    trigger.setAttribute('aria-expanded', String(open));
    panel.hidden = !open;
    if (returnFocus) trigger.focus();
  }
  participationDisclosure.addEventListener('pointerenter', () => {
    window.clearTimeout(closeTimer);
    if (matchMedia('(hover: hover)').matches) setParticipationMenu(true);
  });
  participationDisclosure.addEventListener('pointerleave', () => {
    window.clearTimeout(closeTimer);
    closeTimer = window.setTimeout(() => {
      if (!participationDisclosure.contains(document.activeElement) && !participationDisclosure.matches(':hover'))
        setParticipationMenu(false);
    }, 180);
  });
  trigger.addEventListener('pointerdown', () => { pointerActivation = true; });
  participationDisclosure.addEventListener('focusin', () => {
    if (!pointerActivation) setParticipationMenu(true);
  });
  participationDisclosure.addEventListener('focusout', () => {
    window.setTimeout(() => {
      if (!participationDisclosure.contains(document.activeElement) && !participationDisclosure.matches(':hover'))
        setParticipationMenu(false);
    }, 0);
  });
  trigger.addEventListener('click', () => {
    const mobile = matchMedia('(max-width: 760px)').matches;
    setParticipationMenu(mobile ? trigger.getAttribute('aria-expanded') !== 'true' : true);
    window.setTimeout(() => { pointerActivation = false; }, 0);
  });
  participationDisclosure.addEventListener('keydown', event => {
    if (event.key === 'Escape') {
      event.preventDefault();
      setParticipationMenu(false, true);
    }
  });
  document.addEventListener('pointerdown', event => {
    if (!participationDisclosure.contains(event.target)) setParticipationMenu(false);
  });
  panel.querySelectorAll('a').forEach(link => link.addEventListener('click', () => setParticipationMenu(false)));
}
if (dashboardTabs.length) {
  const dashboardTitles = {
    overview: 'CODARIS Member Dashboard', profile: 'Profile | CODARIS',
    security: 'Security | CODARIS', topics: 'Explore Topics | CODARIS',
    community: 'Community | CODARIS',
  };
  const dashboardLabels = {
    overview: 'Member Dashboard', profile: 'Profile', security: 'Security',
    topics: 'Explore Topics', community: 'Community',
  };
  const dashboardBreadcrumb = document.querySelector('.breadcrumb [aria-current="page"]');
  let lastDashboardAddress = '';

  function dashboardTabIndexFromHash() {
    const fragment = location.hash.slice(1);
    if (!fragment) return 0;
    const index = dashboardTabs.findIndex(tab => tab.dataset.dashboardSection === fragment);
    if (index >= 0) return index;
    history.replaceState(history.state, '', location.pathname + location.search + '#overview');
    return 0;
  }

  function setDashboardContext(index) {
    const tab = dashboardTabs[index];
    if (!tab) return;
    const section = tab.dataset.dashboardSection || 'overview';
    document.title = dashboardTitles[section] || dashboardTitles.overview;
    if (dashboardBreadcrumb) dashboardBreadcrumb.textContent = dashboardLabels[section] || dashboardLabels.overview;
  }

  function setDashboardTab(index, focus, scroll) {
    if (membershipReady()) {
      Module.ccall('codaris_member_tab', null, ['number', 'number'], [index, focus ? 1 : 0]);
    } else {
      dashboardTabs.forEach((tab, tabIndex) => {
        const selected = tabIndex === index;
        tab.setAttribute('aria-selected', String(selected));
        tab.tabIndex = selected ? 0 : -1;
        const panel = document.getElementById(tab.getAttribute('aria-controls'));
        if (panel) panel.hidden = !selected;
        if (selected && focus) tab.focus({preventScroll: true});
      });
    }
    setDashboardContext(index);
    if (scroll) {
      const reducedMotion = matchMedia('(prefers-reduced-motion: reduce)').matches;
      dashboardTabs[index]?.scrollIntoView({block: 'nearest', inline: 'center', behavior: reducedMotion ? 'auto' : 'smooth'});
    }
  }

  function dashboardAddressChanged() {
    const address = location.pathname + location.search + location.hash;
    if (address === lastDashboardAddress) return;
    const index = dashboardTabIndexFromHash();
    lastDashboardAddress = location.pathname + location.search + location.hash;
    setDashboardTab(index, false, false);
  }

  function writeDashboardHistory(index) {
    const tab = dashboardTabs[index];
    if (!tab) return;
    const section = tab.dataset.dashboardSection || 'overview';
    const currentIndex = dashboardTabIndexFromHash();
    const currentSection = dashboardTabs[currentIndex]?.dataset.dashboardSection || 'overview';
    if (section !== currentSection) {
      const nextAddress = location.pathname + location.search + '#' + section;
      history.pushState(history.state, '', nextAddress);
      lastDashboardAddress = nextAddress;
    }
  }

  dashboardTabs.forEach((tab, index) => {
    tab.addEventListener('click', () => {
      writeDashboardHistory(index);
      setDashboardTab(index, true, true);
    });
    tab.addEventListener('keydown', event => {
      if (!membershipReady() || !['ArrowLeft', 'ArrowRight', 'Home', 'End'].includes(event.key)) return;
      event.preventDefault();
      Module.ccall('codaris_member_key', null, ['number', 'string'], [Number(tab.dataset.memberTab), event.key]);
      const selectedIndex = dashboardTabs.findIndex(candidate => candidate.getAttribute('aria-selected') === 'true');
      if (selectedIndex >= 0) {
        writeDashboardHistory(selectedIndex);
        setDashboardContext(selectedIndex);
        const reducedMotion = matchMedia('(prefers-reduced-motion: reduce)').matches;
        dashboardTabs[selectedIndex].scrollIntoView({block: 'nearest', inline: 'center', behavior: reducedMotion ? 'auto' : 'smooth'});
      }
    });
  });

  window.addEventListener('hashchange', dashboardAddressChanged);
  window.addEventListener('popstate', dashboardAddressChanged);
  dashboardAddressChanged();
}
document.querySelectorAll('[data-member-tab]').forEach(function (button) {
  if (button.closest('.account-tabs')) return;
  button.addEventListener('click', function () {
    if (membershipReady()) Module.ccall('codaris_member_tab', null, ['number', 'number'], [Number(button.dataset.memberTab), 1]);
  });
});
document.querySelectorAll('[data-topic]').forEach(function (button) {
  button.addEventListener('click', function () {
    if (!button.disabled) Module.ccall('codaris_member_topic', null, ['number'], [Number(button.dataset.topic)]);
  });
});
// Transport form fields to the C account controller.
window.codarisActionToken = location.hash.slice(1);
if (['/verify-email/', '/reset-password/'].includes(location.pathname)) history.replaceState(null, '', location.pathname);
// Render browser constraint-validation feedback beside the login fields.
// Credential checks and account outcomes remain on the C backend.
document.querySelectorAll('[data-login-field]').forEach(input => {
  const error = document.getElementById(input.id + '-error');
  function showValidity() {
    input.setAttribute('aria-invalid', String(!input.validity.valid));
    error.textContent = input.validationMessage;
  }
  input.addEventListener('invalid', showValidity);
  input.addEventListener('blur', () => { if (input.value) showValidity(); });
  input.addEventListener('input', () => {
    if (input.hasAttribute('aria-invalid')) showValidity();
  });
});
document.querySelectorAll('[data-account-form]').forEach(form => {
  form.addEventListener('submit', event => {
    event.preventDefault();
    if (membershipReady()) Module.ccall('codaris_account_submit', null, ['string'], [form.dataset.accountForm]);
  });
});
document.querySelectorAll('[data-account-action]').forEach(button => {
  button.addEventListener('click', () => {
    if (membershipReady()) Module.ccall('codaris_account_submit', null, ['string'], [button.dataset.accountAction]);
  });
});
const avatarFile = document.getElementById('avatar-file');
const avatarCanvas = document.getElementById('avatar-preview');
const avatarCanvases = [avatarCanvas, document.getElementById('member-card-avatar')].filter(Boolean);
function showAvatarFallback(show) {
  document.querySelectorAll('[data-avatar-fallback]').forEach(element => { element.hidden = !show; });
}
let avatarRequest = 0;
function clearAvatar() {
  avatarCanvases.forEach(canvas => {
    const context = canvas.getContext('2d');
    if (context) context.clearRect(0, 0, canvas.width, canvas.height);
    canvas.hidden = true;
  });
  showAvatarFallback(true);
  const remove = document.getElementById('avatar-remove');
  if (remove) remove.hidden = true;
}
avatarFile?.addEventListener('change', async function () {
  if (!membershipReady()) return;
  const request = ++avatarRequest;
  const file = avatarFile.files[0];
  if (!file) return;
  if (!Module.ccall('codaris_member_photo', 'number', ['string', 'number', 'number', 'number'], [file.type, file.size, 0, 0])) {
    avatarFile.value = ''; return;
  }
  let bitmap;
  try {
    bitmap = await createImageBitmap(file);
    if (request !== avatarRequest) return;
    if (!Module.ccall('codaris_member_photo', 'number', ['string', 'number', 'number', 'number'], [file.type, file.size, bitmap.width, bitmap.height])) {
      avatarFile.value = ''; return;
    }
    const side = Math.min(bitmap.width, bitmap.height);
    for (const canvas of avatarCanvases) {
      const context = canvas.getContext('2d');
      if (!context) throw new Error('Canvas unavailable');
      context.drawImage(bitmap, (bitmap.width-side)/2, (bitmap.height-side)/2, side, side, 0, 0, canvas.width, canvas.height);
      canvas.hidden = false;
    }
    showAvatarFallback(false);
    const remove = document.getElementById('avatar-remove');
    if (remove) remove.hidden = false;
  } catch (_) {
    if (request === avatarRequest) {
      avatarFile.value = '';
      Module.ccall('codaris_member_photo', 'number', ['string', 'number', 'number', 'number'], [file.type, file.size, -1, -1]);
    }
  } finally {
    if (bitmap) bitmap.close();
  }
});
document.getElementById('avatar-remove')?.addEventListener('click', function () {
  if (!membershipReady()) return;
  ++avatarRequest;
  avatarFile.value = '';
  clearAvatar();
  Module.ccall('codaris_member_photo_removed', null, [], []);
});

// Transport documentation search to C; native details remain usable without Wasm.
document.getElementById('catalogue-search')?.addEventListener('input', event => {
  Module.ccall('codaris_catalogue_filter', null, ['string'], [event.target.value.trim()]);
});
// The dashboard library is a short list of topic cards, so filter those cards
// directly while the larger research catalogues continue to use the C filter.
const topicSearch = document.getElementById('topic-search-input');
if (topicSearch) {
  const topicCards = [...document.querySelectorAll('#panel-learn .learning-card')];
  const topicClear = document.getElementById('topic-search-clear');
  const topicCount = document.getElementById('topic-search-count');
  const topicGroups = document.querySelectorAll('#panel-learn [data-topic-group-title]');
  const filterTopics = () => {
    const query = topicSearch.value.trim().toLocaleLowerCase();
    let visible = 0;
    topicCards.forEach(card => {
      const matches = !query || card.textContent.toLocaleLowerCase().includes(query);
      card.hidden = !matches;
      if (matches) visible++;
    });
    topicGroups.forEach(title => {
      const grid = title.nextElementSibling;
      title.hidden = !grid || ![...grid.querySelectorAll('.learning-card')].some(card => !card.hidden);
    });
    topicClear.hidden = !topicSearch.value;
    topicCount.textContent = query
      ? `${visible} ${visible === 1 ? 'topic' : 'topics'} found`
      : `${visible} topics`;
  };
  topicSearch.addEventListener('input', filterTopics);
  topicClear.addEventListener('click', () => {
    topicSearch.value = '';
    filterTopics();
    topicSearch.focus();
  });
}
function revealDocumentationAnchor() {
  if (!document.querySelector('.language-library')) return;
  let id;
  try { id = decodeURIComponent(location.hash.slice(1)); } catch (_) { return; }
  const target = document.getElementById(id);
  if (!target) return;
  const search = document.getElementById('catalogue-search');
  if (target.closest('.catalogue-group') && search && !search.disabled && search.value) {
    search.value = '';
    Module.ccall('codaris_catalogue_filter', null, ['string'], ['']);
  }
  for (let parent = target; parent; parent = parent.parentElement) {
    if (parent.tagName === 'DETAILS') parent.open = true;
  }
  requestAnimationFrame(() => target.scrollIntoView({block: 'start'}));
}
window.addEventListener('hashchange', revealDocumentationAnchor);
revealDocumentationAnchor();

// Credential transport and native Canvas export. All values enter the page as text.
window.codarisCredentialSide = 'front';
let credentialLoadGeneration = 0;
const credentialImageCache = new Map();
function credentialImageForSide(side) {
  if (!credentialImageCache.has(side)) {
    const source = new Image();
    const url = '/api/credential/card?format=svg&side=' + side + '&v=' + Date.now();
    const loaded = new Promise((resolve, reject) => {
      source.onload = () => resolve(url);
      source.onerror = () => reject(new Error('Your credential preview is temporarily unavailable.'));
      source.src = url;
    });
    credentialImageCache.set(side, loaded);
  }
  return credentialImageCache.get(side);
}
function credentialStageIsCurrent(generation) {
  return generation === credentialLoadGeneration;
}
function updateCredentialPreview(image, stage, status, nextSide, url) {
  image.src = url;
  image.hidden = false;
  stage.dataset.state = 'ready';
  stage.dataset.side = nextSide;
  window.codarisCredentialSide = nextSide;
  if (status) status.textContent = '';
}
async function animateCredentialTurn(stage, shouldAnimate, generation) {
  if (!shouldAnimate) return credentialStageIsCurrent(generation);
  stage.classList.add('is-turning');
  await new Promise(resolve => setTimeout(resolve, 130));
  return credentialStageIsCurrent(generation);
}
function credentialPreviewFailed(image, stage, status, alreadyVisible, generation) {
  if (!credentialStageIsCurrent(generation)) return false;
  if (!alreadyVisible) {
    stage.dataset.state = 'error';
    image.hidden = true;
    if (status) status.textContent = 'Your credential preview is temporarily unavailable.';
  }
  return false;
}
function updateCredentialSideLabels(image) {
  image.alt = 'CODARIS membership credential for ' + (window.codarisCredentialName || 'member') + ', ' + window.codarisCredentialSide + ' side';
  const label = document.getElementById('credential-side-label');
  if (label) label.textContent = window.codarisCredentialSide.toUpperCase();
  const flipLabel = document.querySelector('#credential-flip span');
  if (flipLabel) flipLabel.textContent = window.codarisCredentialSide === 'back' ? 'View front side' : 'View reverse side';
}
window.codarisLoadCredential = async function (side, animate = false) {
  const image = document.getElementById('credential-image');
  const stage = document.getElementById('credential-stage');
  if (!image || !stage) return false;
  const status = document.getElementById('credential-stage-status');
  const generation = ++credentialLoadGeneration;
  const nextSide = side === 'back' ? 'back' : 'front';
  const alreadyVisible = stage.dataset.state === 'ready' && !image.hidden;
  if (!alreadyVisible) {
    stage.dataset.state = 'loading';
    if (status) status.textContent = 'Loading your credential preview…';
  }
  try {
    const url = await credentialImageForSide(nextSide);
    if (!credentialStageIsCurrent(generation)) return false;
    const shouldAnimate = animate && alreadyVisible && !matchMedia('(prefers-reduced-motion: reduce)').matches;
    if (!await animateCredentialTurn(stage, shouldAnimate, generation)) return false;
    updateCredentialPreview(image, stage, status, nextSide, url);
    if (animate) requestAnimationFrame(() => stage.classList.remove('is-turning'));
  } catch {
    return credentialPreviewFailed(image, stage, status, alreadyVisible, generation);
  } finally {
    if (credentialStageIsCurrent(generation)) stage.classList.remove('is-turning');
  }
  updateCredentialSideLabels(image);
  return true;
};
const flipCredential = document.getElementById('credential-flip');
flipCredential?.addEventListener('click', () => {
  const stage = document.getElementById('credential-stage');
  if (!stage || flipCredential.disabled) return;
  flipCredential.disabled = true;
  window.codarisLoadCredential(window.codarisCredentialSide === 'front' ? 'back' : 'front', true)
    .finally(() => { flipCredential.disabled = stage.dataset.state !== 'ready'; });
});
let credentialDrag = null;
document.getElementById('credential-stage')?.addEventListener('pointerdown', event => {
  credentialDrag = {x:event.clientX,y:event.clientY,rx:0,ry:0}; event.currentTarget.setPointerCapture(event.pointerId);
});
document.getElementById('credential-stage')?.addEventListener('pointermove', event => {
  if (!credentialDrag || matchMedia('(prefers-reduced-motion: reduce)').matches) return;
  const dx=Math.max(-10,Math.min(10,(event.clientX-credentialDrag.x)/16));
  const dy=Math.max(-8,Math.min(8,(credentialDrag.y-event.clientY)/18));
  event.currentTarget.dataset.tiltX=String(Math.round(dx/10));
  event.currentTarget.dataset.tiltY=String(Math.round(dy/8));
});
document.getElementById('credential-stage')?.addEventListener('pointerup', event => {
  credentialDrag=null;event.currentTarget.dataset.tiltX='0';event.currentTarget.dataset.tiltY='0';
});
async function saveCredential(format) {
  const side=window.codarisCredentialSide==='back'?'back':'front';
  if(format==='png'){
    const response=await fetch('/api/credential/card?format=svg&side='+side,{credentials:'same-origin',cache:'no-store'});
    if(response.status===401)document.dispatchEvent(new Event('codaris-auth-required'));
    if(!response.ok)throw new Error('Credential image is not available yet.');
    const image=new Image(), blob=await response.blob(), objectUrl=URL.createObjectURL(blob);
    try{await new Promise((resolve,reject)=>{image.onload=resolve;image.onerror=reject;image.src=objectUrl;});
      const canvas=document.createElement('canvas');canvas.width=2020;canvas.height=1276;
      const context=canvas.getContext('2d');context.drawImage(image,0,0,canvas.width,canvas.height);
      const png=await new Promise(resolve=>canvas.toBlob(resolve,'image/png'));
      if(!png)throw new Error('PNG export could not be created.');
      const a=document.createElement('a');a.href=URL.createObjectURL(png);a.download='codaris-membership-'+side+'.png';a.click();setTimeout(()=>URL.revokeObjectURL(a.href),1000);
    }finally{URL.revokeObjectURL(objectUrl);}return;
  }
  const url='/api/credential/card?format='+format+(format==='svg'?'&side='+side:'');
  const response=await fetch(url,{credentials:'same-origin',cache:'no-store'});
  if(response.status===401)document.dispatchEvent(new Event('codaris-auth-required'));
  if(!response.ok)throw new Error('Credential download is unavailable.');
  const blob=await response.blob(),objectUrl=URL.createObjectURL(blob),a=document.createElement('a');
  a.href=objectUrl;a.download=format==='pdf'?'codaris-membership.pdf':'codaris-membership-'+side+'.svg';a.click();setTimeout(()=>URL.revokeObjectURL(objectUrl),1000);
}
document.querySelectorAll('[data-format]').forEach(button=>button.addEventListener('click',async()=>{
  button.disabled=true;try{await saveCredential(button.dataset.format);}catch(error){const out=document.getElementById('credential-consent-status');if(out)out.textContent=error.message||'Credential download failed.';}finally{button.disabled=false;}
}));
const publicToggle=document.getElementById('credential-public-enabled');
publicToggle?.addEventListener('change',()=>{
  if(!membershipReady())return;
  publicToggle.dataset.synced=String(!publicToggle.checked);
  publicToggle.disabled=true;
  Module.ccall('codaris_account_submit',null,['string'],['credential']);
});
document.addEventListener('codaris-member-profile',event=>{
  const data=event.detail||{},credential=data.credential||{};
  credentialImageCache.clear();
  const accessible={'credential-accessible-name':data.name,'credential-accessible-number':credential.membership_number,'credential-accessible-role':data.role,'credential-accessible-status':credential.status,'credential-accessible-date':credential.issued_at};
  Object.entries(accessible).forEach(([id,value])=>{const element=document.getElementById(id);if(element)element.textContent=value||'';});
  window.codarisCredentialName=data.name||'member';
  const publicControl=document.getElementById('credential-public-control');if(publicControl)publicControl.hidden=credential.status!=='active';
  const publicToggle=document.getElementById('credential-public-enabled');if(publicToggle){publicToggle.disabled=credential.status!=='active'||!data.email_verified;publicToggle.checked=!!credential.public_enabled;publicToggle.dataset.synced=String(publicToggle.checked);}
  const live=document.getElementById('credential-live-status');if(live)live.textContent=!data.email_verified?'Email verification required':credential.status==='active'?'Active · issued '+(credential.issued_at||''):'Credential pending verification';
  const summary=document.getElementById('credential-summary');if(summary)summary.textContent=!data.email_verified?'Verify your email to activate and download your membership credential.':credential.status==='active'?'Your credential reflects your current CODARIS membership. The QR code checks its live status.':'Your credential is being prepared.';
  document.querySelectorAll('#credential-flip,#credential-download,#credential-download-pdf,#credential-download-png').forEach(button=>button.disabled=!data.email_verified||credential.status!=='active');
  const verifyLink=document.getElementById('credential-verify-link');if(verifyLink&&credential.verification_id){verifyLink.href='/verify/?credential='+encodeURIComponent(credential.verification_id);verifyLink.hidden=!credential.public_enabled;}
  const stage=document.getElementById('credential-stage'),image=document.getElementById('credential-image'),stageStatus=document.getElementById('credential-stage-status');
  if(credential.status==='active'&&data.email_verified){window.codarisCredentialSide='front';if(window.codarisLoadCredential)window.codarisLoadCredential('front');}
  else if(stage){
    credentialLoadGeneration++;
    stage.dataset.state='pending';
    if(image){image.hidden=true;image.removeAttribute('src');}
    if(stageStatus)stageStatus.textContent=!data.email_verified?'Your credential preview will appear here after email verification.':'Your credential is being prepared.';
  }
});
function loadPublicCredential(){
  const status=document.getElementById('credential-verify-status');if(!status)return;
  const params=new URLSearchParams(location.search),credential=params.get('credential')||'';
  fetch('/api/credential/verify?credential='+encodeURIComponent(credential),{credentials:'omit',cache:'no-store'})
    .then(response=>response.json()).then(data=>{
      if(!data.valid){status.textContent=data.message||'Credential verification is not publicly available.';document.getElementById('credential-verify-description').textContent='This credential is unavailable, disabled or no longer active.';return;}
      for(const [id,value] of Object.entries({'credential-verify-name':data.display_name,'credential-verify-role':data.role,'credential-verify-number':data.membership_number,'credential-verify-member-status':data.status,'credential-verify-date':data.issued_at}))document.getElementById(id).textContent=value||'';
      document.getElementById('credential-verify-fields').hidden=false;status.textContent='Membership verified';document.getElementById('credential-verify-description').textContent='This member is currently active in CODARIS.';
    }).catch(()=>{status.textContent='Unable to verify this credential right now.';});
}
loadPublicCredential();
