/* Static documentation remains readable without Wasm; C owns search matching. */
EM_JS(int, catalogue_view_count, (void), {
    Module.catalogueRows = document.querySelectorAll('[data-catalogue-name]');
    return Module.catalogueRows.length;
})
EM_JS(void, catalogue_view_name, (int index, char *buffer, int capacity), {
    const row = Module.catalogueRows[index];
    stringToUTF8(row ? row.dataset.catalogueName : "", buffer, capacity);
})
EM_JS(void, catalogue_view_match, (int index, int visible), {
    const row = Module.catalogueRows[index];
    if (row) row.hidden = !visible;
})
EM_JS(void, catalogue_view_finish, (int count, int searching), {
    const empty = document.getElementById('catalogue-empty');
    if (empty) empty.hidden = count !== 0;
    document.querySelectorAll('.catalogue-group').forEach(group => {
        group.hidden = !group.querySelector('.catalogue-entry:not([hidden])');
        if (searching) group.open = true;
    });
})
EMSCRIPTEN_KEEPALIVE void codaris_catalogue_filter(const char *query) {
    if (!query) return;
    int total = catalogue_view_count(), count = 0;
    for (int i = 0; i < total; ++i) {
        char name[4096];
        catalogue_view_name(i, name, sizeof(name));
        int visible = contains_case_insensitive(name, query);
        catalogue_view_match(i, visible);
        count += visible;
    }
    char message[96];
    int length = snprintf(message, sizeof(message), "%d of %d catalogue entries", count, total);
    if (length >= 0 && (size_t)length < sizeof(message)) view_text("catalogue-count", message);
    catalogue_view_finish(count, *query != '\0');
}
