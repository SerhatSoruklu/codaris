#ifndef CODARIS_CONTACT_H
#define CODARIS_CONTACT_H

#include "../shared/contact_options.h"
#include "../shared/contact_policy.h"

/* DOM/form serialization and fetch are browser transport glue. The topic list,
 * interaction state and response handling remain in C. */
EM_JS(int, contact_page_present, (void), {
    return !!document.querySelector('[data-contact-form]');
})
EM_JS(void, contact_add_topic, (const char *label), {
    const select = document.getElementById('contact-topic');
    if (!select) return;
    const option = document.createElement("option");
    option.value = UTF8ToString(label);
    option.textContent = UTF8ToString(label);
    select.append(option);
})
EM_JS(void, contact_enable_form, (void), {
    const fields = document.getElementById('contact-fields');
    if (fields) fields.disabled = false;
    const form = document.querySelector('[data-contact-form]');
    if (form) form.dataset.ready = 'true';
    const loading = document.querySelector('.contact-loading');
    if (loading) loading.hidden = true;
})
EM_JS(void, contact_update_counter, (int count), {
    const output = document.getElementById('contact-message-count');
    if (!output) return;
    const value = Math.max(0, count);
    output.textContent = value.toLocaleString() + ' / 4,000';
    output.setAttribute('aria-label', value + ' of 4,000 characters');
})
EM_JS(void, contact_bind_form, (void), {
    const form = document.querySelector('[data-contact-form]');
    if (!form) return;
    const message = document.getElementById('contact-message');
    if (message) {
        const updateCount = () => Module.ccall("codaris_contact_count", null, ["number"], [message.value.length]);
        message.addEventListener('input', updateCount);
        updateCount();
    }
    form.addEventListener('submit', event => {
        event.preventDefault();
        if (!form.reportValidity() || form.dataset.submitting === "true") return;
        form.dataset.submitting = "true";
        const button = form.querySelector("[type=\"submit\"]");
        const output = document.getElementById("contact-feedback");
        if (button) button.disabled = true;
        if (output) { output.textContent = "Sending your message…"; output.setAttribute("role", "status"); }
        const payload = {};
        new FormData(form).forEach((value, key) => { payload[key] = value; });
        fetch("/api/contact", {
            method: "POST", credentials: "same-origin", cache: "no-store",
            headers: {"Content-Type": "application/json"}, body: JSON.stringify(payload)
        }).then(async response => {
            const type = response.headers.get("content-type") || "";
            if (!type.toLowerCase().includes("application/json")) throw new Error("unexpected");
            const data = await response.json();
            Module.ccall("codaris_contact_response", null, ["number", "string"], [response.status, data.message || ""]);
        }).catch(error => {
            Module.ccall("codaris_contact_response", null, ["number", "string"],
                [0, error instanceof TypeError ? "Unable to reach CODARIS. Please try again." : "CODARIS returned an unexpected response. Please try again later."]);
        });
    });
})

EM_JS(void, contact_response_success_ui, (void), {
    const output = document.getElementById("contact-feedback");
    if (output) {
        output.setAttribute("role", "status");
        output.dataset.tone = "success";
    }
    const form = document.querySelector("[data-contact-form]");
    if (form) {
        form.reset();
        form.dataset.submitting = "false";
        const button = form.querySelector("[type=\"submit\"]");
        if (button) button.disabled = false;
    }
})
EM_JS(void, contact_response_error_ui, (void), {
    const output = document.getElementById("contact-feedback");
    if (output) {
        output.setAttribute("role", "alert");
        output.dataset.tone = "error";
    }
    const form = document.querySelector("[data-contact-form]");
    if (form) {
        form.dataset.submitting = "false";
        const button = form.querySelector("[type=\"submit\"]");
        if (button) button.disabled = false;
    }
})

EMSCRIPTEN_KEEPALIVE void codaris_contact_response(int status, const char *message) {
    const int success = status == 202;
    view_text("contact-feedback", message);
    if (success)
        contact_response_success_ui();
    else
        contact_response_error_ui();
}

EMSCRIPTEN_KEEPALIVE void codaris_contact_count(int count) {
    if (count < 0)
        count = 0;
    contact_update_counter(count > 4000 ? 4000 : count);
}

static void contact_init(void) {
    if (!contact_page_present()) return;
    for (size_t i = 0; i < sizeof(codaris_contact_topics) / sizeof(codaris_contact_topics[0]); ++i)
        contact_add_topic(codaris_contact_topics[i]);
    contact_bind_form();
    contact_enable_form();
}

#endif
