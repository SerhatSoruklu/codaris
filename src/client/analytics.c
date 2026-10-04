#include <emscripten.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Consent decisions and expiry live in C. Bridges copy strings immediately;
 * JavaScript owns DOM nodes, Google transport and browser event listeners. */
#define CONSENT_LIFETIME 15552000LL /* 180 days */
static int analytics_started;
static int rejected_for_page;

EM_JS(int, analytics_eligible, (void), {
    return location.protocol === 'https:' && location.hostname === 'codaris.org' &&
        document.body.dataset.development === 'false' &&
        !!document.getElementById('analytics-settings');
})

EM_JS(void, analytics_read, (char *buffer, int capacity), {
    try {
        const value = localStorage.getItem('codaris_analytics_consent_v1') || "";
        if (lengthBytesUTF8(value) < capacity) stringToUTF8(value, buffer, capacity);
    } catch (_) { /* No stored permission is the default. */ }
})

EM_JS(int, analytics_save, (const char *value), {
    try {
        localStorage.setItem('codaris_analytics_consent_v1', UTF8ToString(value));
        return 1;
    } catch (_) { return 0; }
})

EM_JS(void, analytics_view, (int choices, const char *message), {
    document.getElementById('analytics-choices').hidden = !choices;
    document.getElementById('analytics-change').hidden = !!choices;
    document.getElementById('analytics-status').textContent = UTF8ToString(message);
})

EM_JS(void, analytics_bind, (void), {
    document.getElementById('analytics-allow').addEventListener('click', () => _codaris_analytics_choice(1));
    document.getElementById('analytics-reject').addEventListener('click', () => _codaris_analytics_choice(0));
    document.getElementById('analytics-change').addEventListener('click', () => {
        _codaris_analytics_preferences();
        document.getElementById('analytics-reject').focus();
    });
    document.querySelectorAll('#analytics-settings button').forEach(button => { button.disabled = false; });
    window.addEventListener('storage', event => {
        if (event.key === 'codaris_analytics_consent_v1' || event.key === null)
            _codaris_analytics_refresh();
    });
    window.addEventListener('pageshow', () => _codaris_analytics_refresh());
    window.addEventListener('focus', () => _codaris_analytics_refresh());
    document.addEventListener('visibilitychange', () => {
        if (!document.hidden) _codaris_analytics_refresh();
    });
})

EM_JS(int, analytics_start, (void), {
    try {
        const url = 'https://www.googletagmanager.com/gtag/js?id=G-8P0VYMWP0J';
        const script = document.createElement('script');
        script.async = true;
        // This policy accepts only the fixed Analytics script URL.
        const policy = window.trustedTypes ? trustedTypes.createPolicy('codaris-analytics', {
            createScriptURL: value => {
                if (value !== url) throw new TypeError('Unexpected Analytics script URL');
                return value;
            }
        }) : null;
        script.src = policy ? policy.createScriptURL(url) : url;
        script.addEventListener('error', () => {
            document.getElementById('analytics-status').textContent =
                'Analytics was allowed, but the Google tag could not load. You can still change your choice.';
        });
        window.dataLayer = window.dataLayer || [];
        window.gtag = function () { window.dataLayer.push(arguments); };
        window['ga-disable-G-8P0VYMWP0J'] = false;
        gtag('consent', 'default', {
            analytics_storage: 'denied', ad_storage: 'denied',
            ad_user_data: 'denied', ad_personalization: 'denied'
        });
        gtag('consent', 'update', { analytics_storage: 'granted' });
        gtag('js', new Date());
        gtag('config', 'G-8P0VYMWP0J', {
            allow_google_signals: false,
            allow_ad_personalization_signals: false,
            cookie_domain: 'none', cookie_path: '/',
            cookie_expires: 15552000, cookie_update: false,
            cookie_flags: 'SameSite=Lax;Secure',
            page_location: location.origin + location.pathname,
            page_referrer: "",
            page_title: document.title
        });
        document.head.append(script);
        return 1;
    } catch (_) { return 0; }
})

EM_JS(void, analytics_stop, (int reload), {
    window['ga-disable-G-8P0VYMWP0J'] = true;
    // Clear only this integration's cookies; authentication is independent.
    for (const name of ['_ga', '_ga_8P0VYMWP0J']) {
        const expired = name + '=; Max-Age=0; Path=/; SameSite=Lax; Secure';
        document.cookie = expired;
        document.cookie = expired + '; Domain=codaris.org';
    }
    // Unload Google completely instead of continuing with cookieless pings.
    if (reload) location.reload();
})

static int stored_choice(void) {
    char receipt[64] = {0};
    analytics_read(receipt, (int)sizeof(receipt));
    if ((receipt[0] != '0' && receipt[0] != '1') || receipt[1] != ':' || !receipt[2]) return -1;
    for (const char *p = receipt + 2; *p; ++p) if (*p < '0' || *p > '9') return -1;
    errno = 0;
    char *end = NULL;
    long long recorded = strtoll(receipt + 2, &end, 10);
    time_t now = time(NULL);
    if (errno || !end || *end || now == (time_t)-1 || recorded <= 0 ||
        recorded > (long long)now || (long long)now - recorded >= CONSENT_LIFETIME) return -1;
    return receipt[0] - '0';
}

static void apply_choice(int choice) {
    if (choice == 1) {
        if (!analytics_started) analytics_started = analytics_start();
        analytics_view(0, analytics_started ? "Analytics allowed. You can change your choice at any time." :
                       "Analytics could not start. You can change your choice or retry on your next visit.");
    } else {
        analytics_stop(analytics_started);
        analytics_view(choice == -1, choice == -1 ? "Analytics is off until you allow it." :
                       "Analytics rejected. No Google Analytics tag is loaded.");
    }
}

EMSCRIPTEN_KEEPALIVE void codaris_analytics_choice(int allow) {
    if (!analytics_eligible() || (allow != 0 && allow != 1)) return;
    char receipt[64];
    time_t now = time(NULL);
    if (now == (time_t)-1 || now <= 0) return;
    int size = snprintf(receipt, sizeof(receipt), "%d:%lld", allow, (long long)now);
    if (size < 0 || (size_t)size >= sizeof(receipt)) return;
    if (!analytics_save(receipt)) {
        /* Rejection must still stop an already loaded tag if storage fails. */
        if (!allow) {
            rejected_for_page = 1;
            analytics_stop(0);
        }
        analytics_view(1, analytics_started ?
            "Your browser could not save this choice. If you rejected analytics, it is disabled on this page. Clear this site's browser data to remove a previously saved permission." :
            "Your browser could not save your choice. Analytics remains off; please allow browser storage and try again.");
        return;
    }
    if (allow && rejected_for_page && analytics_started) analytics_stop(1);
    rejected_for_page = 0;
    apply_choice(allow);
}

EMSCRIPTEN_KEEPALIVE void codaris_analytics_preferences(void) {
    analytics_view(1, analytics_started ? "Analytics is allowed. Choose Reject analytics to withdraw your permission." :
                   "Analytics is off. You can allow or reject it here.");
}

EMSCRIPTEN_KEEPALIVE void codaris_analytics_refresh(void) {
    if (analytics_eligible() && !rejected_for_page) apply_choice(stored_choice());
}

int main(void) {
    if (!analytics_eligible()) return 0;
    analytics_bind();
    codaris_analytics_refresh();
    return 0;
}
