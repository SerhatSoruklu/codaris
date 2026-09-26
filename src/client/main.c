#include <emscripten.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "demo_data.h"
#include "globe.h"
#include "../shared/membership_options.h"

/* The bridge copies strings synchronously into the DOM; C retains ownership.
 * No input values are inserted as HTML. Account requests use separate JSON transport. */
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
EM_JS(void, view_country_option, (const char *name, const char *code), {
    const countryName = UTF8ToString(name);
    const flagCode = UTF8ToString(code);
    document.querySelectorAll('[data-country-picker-select]').forEach(select => {
        const option = document.createElement('option');
        option.value = option.textContent = countryName;
        option.dataset.flag = flagCode;
        select.append(option);
    });
    document.querySelectorAll('[data-country-picker-menu]').forEach(menu => {
        const item = document.createElement('button');
        item.type = 'button'; item.className = 'country-picker-option'; item.setAttribute('role', 'option');
        item.dataset.value = countryName; item.dataset.flag = flagCode; item.setAttribute('aria-selected', 'false');
        if (flagCode) {
            const flag = document.createElement('img'); flag.className = 'country-picker-option-flag';
            flag.src = "/assets/country-flags/" + flagCode + ".svg"; flag.alt = ""; flag.width = 20; flag.height = 15;
            flag.setAttribute('aria-hidden', 'true'); item.append(flag);
        } else {
            const mark = document.createElement('span'); mark.className = 'country-picker-option-mark'; mark.setAttribute('aria-hidden', 'true'); mark.textContent = '◎'; item.append(mark);
        }
        const label = document.createElement('span'); label.textContent = countryName; item.append(label);
        menu.append(item);
    });
})
EM_JS(void, view_empty, (int empty), {
    document.getElementById('empty-state').hidden = !empty;
})
EM_JS(void, view_ready, (void), {
    const search = document.getElementById('country-search');
    if (search) search.disabled = false;
    document.querySelectorAll('[data-topic]').forEach(button => { button.disabled = !document.getElementById('account-content'); });
    const languageSearch = document.getElementById('catalogue-search');
    if (languageSearch) languageSearch.disabled = false;
    const reset = document.getElementById('globe-reset');
    if (reset) reset.disabled = false;
    const globe = document.querySelector('.network-globe');
    if (globe) globe.dataset.ready = 'true';
    /* Account controls are enabled by the C membership controller. */
    document.getElementById('runtime-status').textContent = "";
})
/* Native browser validity flags are transported to C for inline feedback. */
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

#include "membership.h"
#include "contact.h"
#include "languages.h"

EM_JS(int, view_has_countries, (void), {
    return !!document.getElementById('country-rows');
})

EM_JS(void, view_community_metrics, (void), {
    if(!document.getElementById('supporters-metric'))return;
    fetch('/api/community',{credentials:'same-origin',cache:'no-store'})
      .then(response=>{if(!response.ok)throw new Error('Unavailable');return response.json();})
      .then(data=>{document.getElementById('supporters-metric').textContent=String(data.members);
          document.getElementById('countries-metric').textContent=String(data.countries);})
      .catch(()=>{document.getElementById('supporters-metric').textContent='Unavailable';
          document.getElementById('countries-metric').textContent='Unavailable';});
})
int main(void) {
    for(size_t i=0;i<membership_country_count;++i)
        view_country_option(membership_countries[i].name, membership_countries[i].code);
    view_country_option("Other / not listed", "");
    membership_init();
    contact_init();
    if(catalogue_view_count())codaris_catalogue_filter("");
    view_community_metrics();
    view_ready();
    return 0;
}
