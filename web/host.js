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
document.querySelectorAll('[data-member-tab]').forEach(function (button) {
  button.addEventListener('click', function () {
    if (membershipReady()) Module.ccall('codaris_member_tab', null, ['number', 'number'], [Number(button.dataset.memberTab), 1]);
  });
  if (button.getAttribute('role') === 'tab') button.addEventListener('keydown', function (event) {
    if (!membershipReady() || !['ArrowLeft', 'ArrowRight', 'Home', 'End'].includes(event.key)) return;
    event.preventDefault();
    Module.ccall('codaris_member_key', null, ['number', 'string'], [Number(button.dataset.memberTab), event.key]);
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
}
avatarFile?.addEventListener('change', async function () {
  if (!membershipReady()) return;
  const request = ++avatarRequest;
  clearAvatar();
  const file = avatarFile.files[0];
  if (!file) { Module.ccall('codaris_member_photo_removed', null, [], []); return; }
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
  } catch (_) {
    if (request === avatarRequest) {
      clearAvatar();
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
