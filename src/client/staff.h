#ifndef CODARIS_STAFF_H
#define CODARIS_STAFF_H
/* Fictional, read-only fixtures. Client-side preview roles are not authorization. */
static int staff_role;
static const char *staff_names[] = {"Member — no staff role", "Administrator", "Moderator", "Support"};
static const char *staff_slugs[] = {"", "administrator", "moderator", "support"};
typedef struct { const char *id, *name, *email, *location, *discipline, *status, *motivation; } StaffApplication;
static const StaffApplication staff_applications[] = {
    {"DEMO-001", "Alex Example", "alex@example.test", "United Kingdom", "Software development", "Pending", "I would like to contribute to testing and responsible software design."},
    {"DEMO-002", "Sam Sample", "sam@example.test", "Canada", "System design", "Under review", "I enjoy documenting architectural decisions and exploring resilient systems."},
    {"DEMO-003", "Jordan Demo", "jordan@example.test", "Germany", "Developer education", "Approved", "I want to help new developers learn through practical examples."}
};
EM_JS(void, staff_role_view, (int role, const char *name, const char *slug), {
    const label = UTF8ToString(name), fragment = UTF8ToString(slug);
    const badge = document.getElementById('member-staff-badge');
    if (badge) badge.textContent = role ? 'Staff · ' + label + ' · preview' : 'Member · no staff role';
    const select = document.getElementById('member-role-preview');
    if (select) select.value = String(role);
    const link = document.getElementById('member-staff-link');
    if (link) {
        link.hidden = !role;
        link.href = '/staff-dashboard/#' + fragment;
    }
    const workspace = document.getElementById('staff-workspace');
    if (workspace) {
        workspace.hidden = !role;
        document.getElementById('staff-access-message').hidden = !!role;
        document.getElementById('staff-role-name').textContent = label;
        const origin = document.body.dataset.adminSite === 'true' ? 'https://codaris.org' : "";
        document.getElementById('staff-member-return').href = origin + '/dashboard/#' + fragment;
        document.getElementById('staff-application-detail').hidden = true;
    }
})
EM_JS(void, staff_login_view, (int missing_id, int missing_password), {
    const id = document.getElementById('staff-identifier');
    const password = document.getElementById('staff-password');
    id.setAttribute('aria-invalid', String(!!missing_id));
    password.setAttribute('aria-invalid', String(!!missing_password));
    document.getElementById('staff-identifier-error').textContent = missing_id ? 'Enter your membership ID or email.' : "";
    document.getElementById('staff-password-error').textContent = missing_password ? 'Enter your password.' : "";
    if (missing_id) id.focus();
    else if (missing_password) password.focus();
    else { password.value = ""; document.getElementById('staff-login-feedback').focus(); }
})
EM_JS(void, staff_list_clear, (void), {
    const list = document.getElementById('staff-application-list');
    if (list) list.replaceChildren();
})
EM_JS(void, staff_list_row, (int index, const char *id, const char *name, const char *status), {
    const list = document.getElementById('staff-application-list');
    if (!list) return;
    const row = document.createElement('div'); row.className = 'staff-application-row';
    const text = document.createElement('span');
    text.textContent = UTF8ToString(name) + ' · ' + UTF8ToString(id);
    const state = document.createElement('span'); state.className = 'status'; state.textContent = UTF8ToString(status);
    const button = document.createElement('button'); button.type = 'button'; button.className = 'topic-toggle';
    button.dataset.application = String(index); button.textContent = 'View application';
    button.setAttribute('aria-label', 'View application from ' + UTF8ToString(name));
    row.append(text, state, button); list.append(row);
})
EM_JS(void, staff_list_count, (int count), {
    const empty = document.getElementById('staff-empty');
    if (empty) empty.hidden = count !== 0;
})
EM_JS(void, staff_detail_view, (int show), {
    const detail = document.getElementById('staff-application-detail');
    if (detail) {
        detail.hidden = !show;
        if (show) document.getElementById('staff-detail-name').focus();
        else document.getElementById('staff-search').focus();
    }
})
EMSCRIPTEN_KEEPALIVE void codaris_staff_filter(const char *query, const char *status) {
    if (!member_development() || !staff_role) return;
    staff_list_clear();
    int count = 0;
    for (int i = 0; i < 3; ++i) {
        const StaffApplication *app = &staff_applications[i];
        if (strcmp(status, "all") && strcmp(status, app->status)) continue;
        if (!contains_case_insensitive(app->name, query) && !contains_case_insensitive(app->id, query)) continue;
        staff_list_row(i, app->id, app->name, app->status); ++count;
    }
    char result[64];
    int written = snprintf(result, sizeof(result), "%d of 3 sample applications", count);
    if (written > 0 && (size_t)written < sizeof(result)) view_text("staff-results-count", result);
    staff_list_count(count);
}
EMSCRIPTEN_KEEPALIVE void codaris_staff_role(int role) {
    if (!member_development()) return;
    staff_role = role >= 1 && role <= 3 ? role : 0;
    staff_role_view(staff_role, staff_names[staff_role], staff_slugs[staff_role]);
    staff_list_clear();
    if (staff_role) codaris_staff_filter("", "all");
}
EMSCRIPTEN_KEEPALIVE void codaris_staff_fragment(const char *fragment) {
    int role = 0;
    for (int i = 1; i <= 3; ++i) if (!strcmp(fragment, staff_slugs[i])) role = i;
    codaris_staff_role(role);
}
EMSCRIPTEN_KEEPALIVE void codaris_staff_login(const char *identifier, const char *password) {
    if (!member_development()) return;
    while (*identifier && isspace((unsigned char)*identifier)) ++identifier;
    view_text("staff-login-feedback", !*identifier || !*password ? "Please complete the highlighted fields." :
        "Staff sign-in is not connected. No credentials were checked and no access was granted. Use a labelled staff preview to explore.");
    staff_login_view(!*identifier, !*password);
}
EMSCRIPTEN_KEEPALIVE void codaris_staff_application(int index) {
    if (!member_development() || !staff_role) {
        view_text("staff-action-feedback", "Staff access is required."); return;
    }
    if (index == -1) { staff_detail_view(0); return; }
    if (index < 0 || index >= 3) { view_text("staff-action-feedback", "That sample application is unavailable."); return; }
    const StaffApplication *app = &staff_applications[index];
    view_text("staff-detail-name", app->name); view_text("staff-detail-id", app->id);
    view_text("staff-detail-email", app->email); view_text("staff-detail-location", app->location);
    view_text("staff-detail-discipline", app->discipline); view_text("staff-detail-status", app->status);
    view_text("staff-detail-motivation", app->motivation); view_text("staff-action-feedback", "");
    staff_detail_view(1);
}
#endif
