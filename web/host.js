/* Browser transport only. Data, filtering and preview outcome live in C/Wasm.
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
      "Interactive preview unavailable. Please rebuild or reload the page.";
    const search = document.getElementById("country-search");
    if (search) search.disabled = true;
    const fields = document.getElementById("join-fields");
    if (fields) fields.disabled = true;
  },
};
document.getElementById("codaris-runtime")?.addEventListener("error", Module.onAbort);
document
  .getElementById("country-search")
  ?.addEventListener("input", function (event) {
    Module.ccall("codaris_filter", null, ["string"], [event.target.value]);
  });
document
  .getElementById("join-form")
  ?.addEventListener("submit", function (event) {
    event.preventDefault();
    Module.ccall("codaris_preview_join", null, [], []);
  });

// Prepared for future activation; the current form is inert and disabled.
// Native validity and UTF-16 lengths match HTML minlength/maxlength semantics.
document
  .querySelectorAll("#join-form input, #join-form textarea, #join-form select")
  .forEach(function (field) {
    function report(reveal) {
      if (document.getElementById("join-fields").disabled) return;
      const validity = field.validity;
      const flags =
        (validity.valueMissing ? 1 : 0) |
        (validity.typeMismatch ? 2 : 0) |
        (validity.patternMismatch ? 4 : 0);
      Module.ccall(
        "codaris_validate_field",
        null,
        ["string", "number", "number", "number", "number", "number", "number"],
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
