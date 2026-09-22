#include <emscripten.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "demo_data.h"
#include "globe.h"

/* The bridge copies strings synchronously into the DOM; C retains ownership.
 * No input values are inserted as HTML, transmitted, or persisted. */
EM_JS(void, view_text, (const char *id, const char *value), {
    const element = document.getElementById(UTF8ToString(id));
    if (element) element.textContent = UTF8ToString(value);
})
EM_JS(void, view_clear_rows, (void), {
    document.getElementById('country-rows').replaceChildren();
})
EM_JS(void, view_row, (const char *name, const char *code, const char *estimate,
                       const char *supporters, const char *status), {
    const row = document.createElement('tr');
    const country = document.createElement('td');
    const badge = document.createElement('span');
    badge.className = 'country-code';
    badge.textContent = UTF8ToString(code);
    badge.setAttribute('aria-hidden', 'true');
    country.append(badge, document.createTextNode(UTF8ToString(name)));
    row.append(country);
    for (const value of [estimate, supporters]) {
        const cell = document.createElement('td');
        cell.textContent = UTF8ToString(value);
        row.append(cell);
    }
    const cell = document.createElement('td');
    const tag = document.createElement('span');
    tag.textContent = UTF8ToString(status);
    tag.className = 'status ' + UTF8ToString(status).toLowerCase();
    cell.append(tag);
    row.append(cell);
    document.getElementById('country-rows').append(row);
})
EM_JS(void, view_country_option, (const char *name), {
    const option = document.createElement('option');
    option.value = option.textContent = UTF8ToString(name);
    const select = document.getElementById('join-country');
    if (select) select.append(option);
})
EM_JS(void, view_empty, (int empty), {
    document.getElementById('empty-state').hidden = !empty;
})
EM_JS(void, view_ready, (void), {
    const search = document.getElementById('country-search');
    if (search) search.disabled = false;
    const reset = document.getElementById('globe-reset');
    if (reset) reset.disabled = false;
    const globe = document.querySelector('.network-globe');
    if (globe) globe.dataset.ready = 'true';
    /* Applications remain disabled until the email/API workflow is implemented. */
    document.getElementById('runtime-status').textContent = "";
})
/* Native browser validity flags are transported to C; personal values never leave DOM. */
EM_JS(void, view_field, (const char *name, const char *message, const char *count, int invalid, int warning), {
    const key = UTF8ToString(name);
    const field = document.getElementById('join-' + key);
    field.setAttribute('aria-invalid', invalid ? 'true' : 'false');
    document.getElementById(key + '-error').textContent = UTF8ToString(message);
    const counter = document.getElementById(key + '-count');
    if (counter) {
        counter.textContent = UTF8ToString(count);
        counter.classList.toggle('near-limit', !!warning);
    }
})

EMSCRIPTEN_KEEPALIVE void codaris_validate_field(const char *name, int length, int minimum,
                                                int maximum, int flags, int reveal) {
    const char *message = "";
    char count[64] = "";
    if (maximum > 0) {
        int written = snprintf(count, sizeof(count), "%d / %d%s", length, maximum,
                               length >= maximum ? " — limit reached" : "");
        if (written < 0 || (size_t)written >= sizeof(count)) count[0] = '\0';
    }
    if (reveal) {
        if (flags & 1) message = "Please complete this required field.";
        else if (flags & 2) message = "Enter a valid email address or complete URL.";
        else if (flags & 4) message = "Use a complete HTTPS URL (https://…).";
        else if (length > 0 && length < minimum) message = "Please add more detail to meet the minimum length.";
        else if (maximum > 0 && length > maximum) message = "Please shorten this value to the character limit.";
    }
    view_field(name, message, count, *message != '\0', maximum > 0 && length >= maximum * 9 / 10);
}

/* All buffers are fixed and sized for the full unsigned integer range. */
static void format_number(unsigned value, char output[32]) {
    char digits[16];
    int length = snprintf(digits, sizeof(digits), "%u", value);
    if (length < 0 || (size_t)length >= sizeof(digits)) {
        output[0] = '\0';
        return;
    }
    size_t pos = 0;
    for (int i = 0; i < length; ++i) {
        if (i > 0 && (length - i) % 3 == 0) output[pos++] = ',';
        output[pos++] = digits[i];
    }
    output[pos] = '\0';
}

static int contains_case_insensitive(const char *text, const char *query) {
    if (*query == '\0') return 1;
    for (; *text; ++text) {
        size_t i = 0;
        while (query[i] && text[i] &&
               tolower((unsigned char)text[i]) == tolower((unsigned char)query[i])) ++i;
        if (!query[i]) return 1;
    }
    return 0;
}

EMSCRIPTEN_KEEPALIVE void codaris_filter(const char *query) {
    size_t visible = 0;
    view_clear_rows();
    for (size_t i = 0; i < country_count; ++i) {
        const Country *country = &countries[i];
        if (!contains_case_insensitive(country->name, query) &&
            !contains_case_insensitive(country->code, query)) continue;
        char estimate[32], supporters[32];
        format_number(country->developers, estimate);
        format_number(country->supporters, supporters);
        view_row(country->name, country->code, estimate, supporters, country->status);
        ++visible;
    }
    char count[80];
    int result = snprintf(count, sizeof(count), "%zu of %zu countries · illustrative preview", visible, country_count);
    if (result >= 0 && (size_t)result < sizeof(count)) view_text("result-count", count);
    view_empty(visible == 0);
}

EMSCRIPTEN_KEEPALIVE void codaris_preview_join(void) {
    view_text("join-feedback", "Applications are not open yet. Nothing has been sent or saved.");
}

EM_JS(int, view_has_countries, (void), {
    return !!document.getElementById('country-rows');
})

int main(void) {
    unsigned supporters = 0, developers = 0, represented = 0;
    for (size_t i = 0; i < country_count; ++i) {
        supporters += countries[i].supporters;
        developers += countries[i].developers;
        if (countries[i].supporters > 0) ++represented;
        view_country_option(countries[i].name);
    }
    view_country_option("Other / not listed");
    char number[32];
    format_number(supporters, number);
    view_text("supporters-metric", number);
    format_number(represented, number);
    view_text("countries-metric", number);
    format_number(demo_communities, number);
    view_text("communities-metric", number);
    int result = snprintf(number, sizeof(number), "%.1fM", developers / 1000000.0);
    if (result >= 0 && (size_t)result < sizeof(number)) view_text("reach-metric", number);
    if (view_has_countries()) codaris_filter("");
    view_ready();
    return 0;
}
