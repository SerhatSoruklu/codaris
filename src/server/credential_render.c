#include "credential_render.h"

#include <hpdf.h>
#include <qrcodegen.h>
#include <sodium.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <zlib.h>

#define CREDENTIAL_VIEW_WIDTH 1010.0
#define CREDENTIAL_VIEW_HEIGHT (CREDENTIAL_VIEW_WIDTH * 53.98 / 85.6)
#define CREDENTIAL_PRINT_WIDTH 242.65
#define CREDENTIAL_PRINT_HEIGHT (CREDENTIAL_PRINT_WIDTH * 53.98 / 85.6)
#define CREDENTIAL_LABEL_LINE_X 480.0
#define CREDENTIAL_LABEL_LINE_START_X 630.0
#define CREDENTIAL_SECTION_LABEL_Y 153.0
#define CREDENTIAL_ACCENT_LINE_WIDTH 3.0
#define CREDENTIAL_QR_SIZE 155.0

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} TextBuffer;

static int reserve(TextBuffer *buffer, size_t addition) {
    if (addition > SIZE_MAX - buffer->length - 1)
        return 0;
    size_t needed = buffer->length + addition + 1;
    if (needed <= buffer->capacity)
        return 1;
    size_t next = buffer->capacity ? buffer->capacity : 4096;
    while (next < needed) {
        if (next > SIZE_MAX / 2) {
            next = needed;
            break;
        }
        next *= 2;
    }
    char *memory = realloc(buffer->data, next);
    if (!memory)
        return 0;
    buffer->data = memory;
    buffer->capacity = next;
    return 1;
}

static int append_n(TextBuffer *buffer, const char *text, size_t length) {
    if (!reserve(buffer, length))
        return 0;
    memcpy(buffer->data + buffer->length, text, length);
    buffer->length += length;
    buffer->data[buffer->length] = 0;
    return 1;
}

static int append(TextBuffer *buffer, const char *text) {
    return append_n(buffer, text, strlen(text));
}

#if defined(__GNUC__) || defined(__clang__)
static int appendf(TextBuffer *buffer, const char *format, ...)
    __attribute__((format(printf, 2, 3)));
#endif
static int appendf(TextBuffer *buffer, const char *format, ...) {
    va_list args, copy;
    va_start(args, format);
    va_copy(copy, args);
    int needed = vsnprintf(NULL, 0, format, copy);
    va_end(copy);
    if (needed < 0 || !reserve(buffer, (size_t)needed)) {
        va_end(args);
        return 0;
    }
    vsnprintf(buffer->data + buffer->length, buffer->capacity - buffer->length, format, args);
    va_end(args);
    buffer->length += (size_t)needed;
    return 1;
}

static int xml(TextBuffer *buffer, const char *text) {
    for (const unsigned char *p = (const unsigned char *)text; *p; ++p) {
        const char *escaped = NULL;
        switch (*p) {
        case '&': escaped = "&amp;"; break;
        case '<': escaped = "&lt;"; break;
        case '>': escaped = "&gt;"; break;
        case '"': escaped = "&quot;"; break;
        case '\'': escaped = "&apos;"; break;
        default: break;
        }
        if (escaped) {
            if (!append(buffer, escaped)) return 0;
        } else if (!append_n(buffer, (const char *)p, 1)) {
            return 0;
        }
    }
    return 1;
}

static int xml_n(TextBuffer *buffer, const char *text, size_t length) {
    size_t start=0;
    for(size_t i=0;i<length;i++) {
        const char *escaped=NULL;
        switch((unsigned char)text[i]) {
        case '&': escaped="&amp;"; break;
        case '<': escaped="&lt;"; break;
        case '>': escaped="&gt;"; break;
        case '"': escaped="&quot;"; break;
        case '\'': escaped="&apos;"; break;
        default: break;
        }
        if(escaped) {
            if(i>start&&!append_n(buffer,text+start,i-start))return 0;
            if(!append(buffer,escaped))return 0;
            start=i+1;
        }
    }
    return start<length?append_n(buffer,text+start,length-start):1;
}

static unsigned utf8_count(const char *text) {
    unsigned count = 0;
    for (const unsigned char *p = (const unsigned char *)text; *p; ++p)
        if ((*p & 0xc0u) != 0x80u) ++count;
    return count;
}

static const char *code128_patterns[] = {
    "212222","222122","222221","121223","121322","131222","122213","122312","132212","221213",
    "221312","231212","112232","122132","122231","113222","123122","123221","223211","221132",
    "221231","213212","223112","312131","311222","321122","321221","312212","322112","322211",
    "212123","212321","232121","111323","131123","131321","112313","132113","132311","211313",
    "231113","231311","112133","112331","132131","113123","113321","133121","313121","211331",
    "231131","213113","213311","213131","311123","311321","331121","312113","312311","332111",
    "314111","221411","431111","111224","111422","121124","121421","141122","141221","112214",
    "112412","122114","122411","142112","142211","241211","221114","413111","241112","134111",
    "111242","121142","121241","114212","124112","124211","411212","421112","421211","212141",
    "214121","412121","111143","111341","131141","114113","114311","411113","411311","113141",
    "114131","311141","411131","211412","211214","211232","2331112"
};

static int code128_values(const char *value, unsigned values[96], size_t *count) {
    size_t length = strlen(value);
    if (!length || length > 80)
        return 0;
    values[0] = 104; /* Code Set B */
    unsigned checksum = 104;
    for (size_t i = 0; i < length; ++i) {
        unsigned char c = (unsigned char)value[i];
        if (c < 32 || c > 126)
            return 0;
        values[i + 1] = (unsigned)c - 32;
        checksum += values[i + 1] * (unsigned)(i + 1);
    }
    values[length + 1] = checksum % 103;
    values[length + 2] = 106;
    *count = length + 3;
    return 1;
}

static int code128_svg(TextBuffer *buffer, const char *value, double x, double y, double width,
                       double height) {
    unsigned values[96];
    size_t count = 0;
    if (!code128_values(value, values, &count))
        return 0;
    unsigned modules = 20; /* quiet zones */
    for (size_t i = 0; i < count; ++i)
        for (const char *p = code128_patterns[values[i]]; *p; ++p)
            modules += (unsigned)(*p - '0');
    const double quiet_y = 6.0;
    double unit = width / modules, cursor = x + unit * 10.0;
    if (!appendf(buffer, "<rect x=\"%.3f\" y=\"%.2f\" width=\"%.3f\" height=\"%.2f\" fill=\"#f4f7fa\"/><g fill=\"#071018\" aria-hidden=\"true\">",x,y,width,height)) return 0;
    for (size_t i = 0; i < count; ++i) {
        const char *pattern = code128_patterns[values[i]];
        int black = 1;
        for (const char *p = pattern; *p; ++p) {
            double run = (double)(*p - '0') * unit;
            if (black && !appendf(buffer, "<rect x=\"%.3f\" y=\"%.2f\" width=\"%.3f\" height=\"%.2f\"/>", cursor, y + quiet_y, run, height - quiet_y * 2.0)) return 0;
            cursor += run;
            black = !black;
        }
    }
    return append(buffer, "</g>");
}

static int qr_matrix(const CodarisCredential *credential, uint8_t qr[qrcodegen_BUFFER_LEN_MAX], int *size) {
    uint8_t temporary[qrcodegen_BUFFER_LEN_MAX];
    return qrcodegen_encodeText(credential->verification_url, temporary, qr,
                                qrcodegen_Ecc_QUARTILE, 1, 20, qrcodegen_Mask_AUTO, true) &&
           ((*size = qrcodegen_getSize(qr)) > 0);
}

static int qr_svg(TextBuffer *buffer, const CodarisCredential *credential, double x, double y,
                  double box) {
    uint8_t qr[qrcodegen_BUFFER_LEN_MAX];
    int size = 0;
    if (!qr_matrix(credential, qr, &size)) return 0;
    const double quiet = 4.0, span = (double)size + 2.0 * quiet, unit = box / span;
    if (!appendf(buffer, "<rect x=\"%.2f\" y=\"%.2f\" width=\"%.2f\" height=\"%.2f\" rx=\"4\" fill=\"#fff\"/>", x, y, box, box)) return 0;
    if (!appendf(buffer, "<path fill=\"#061018\" transform=\"translate(%.4f %.4f) scale(%.5f)\" d=\"", x + quiet * unit, y + quiet * unit, unit)) return 0;
    for (int row = 0; row < size; ++row) {
        int col = 0;
        while (col < size) {
            while (col < size && !qrcodegen_getModule(qr, col, row)) ++col;
            int start = col;
            while (col < size && qrcodegen_getModule(qr, col, row)) ++col;
            if (start < col && !appendf(buffer, "M%d %dh%dv1h-%dz", start, row, col - start, col - start)) return 0;
        }
    }
    return append(buffer, "\"/>");
}

static int png_chunk(TextBuffer *png, const char type[4], const unsigned char *data, uint32_t size) {
    unsigned char header[4] = {(unsigned char)(size >> 24), (unsigned char)(size >> 16),
                               (unsigned char)(size >> 8), (unsigned char)size};
    if (!append_n(png, (const char *)header, sizeof(header)) || !append_n(png, type, 4) ||
        (size && !append_n(png, (const char *)data, size))) return 0;
    uLong crc = crc32(0L, Z_NULL, 0);
    crc = crc32(crc, (const Bytef *)type, 4);
    if (size) crc = crc32(crc, data, size);
    unsigned char tail[4] = {(unsigned char)(crc >> 24), (unsigned char)(crc >> 16),
                             (unsigned char)(crc >> 8), (unsigned char)crc};
    return append_n(png, (const char *)tail, sizeof(tail));
}

static int avatar_png_data(const CodarisCredential *credential, char **base64) {
    *base64 = NULL;
    if (!credential->avatar_rgba || credential->avatar_size != 40000)
        return 1;
    unsigned char raw[30100];
    for (size_t y = 0; y < 100; ++y) {
        raw[y * 301] = 0; /* PNG filter: None; 1 filter byte + 100 RGB pixels. */
        for (size_t x = 0; x < 100; ++x) {
            const unsigned char *rgba = credential->avatar_rgba + (y * 100 + x) * 4;
            unsigned alpha = rgba[3];
            unsigned char *rgb = raw + y * 301 + 1 + x * 3;
            for (size_t channel = 0; channel < 3; ++channel)
                rgb[channel] = (unsigned char)((rgba[channel] * alpha + 8u * (255u - alpha)) / 255u);
        }
    }
    uLongf compressed_size = compressBound(sizeof(raw));
    unsigned char *compressed = malloc(compressed_size);
    if (!compressed) return 0;
    if (compress2(compressed, &compressed_size, raw, sizeof(raw), Z_BEST_COMPRESSION) != Z_OK) {
        free(compressed); return 0;
    }
    TextBuffer png = {0};
    static const unsigned char signature[8] = {137,80,78,71,13,10,26,10};
    unsigned char ihdr[13] = {0,0,0,100,0,0,0,100,8,2,0,0,0};
    int ok = append_n(&png, (const char *)signature, sizeof(signature)) &&
             png_chunk(&png, "IHDR", ihdr, sizeof(ihdr)) &&
             png_chunk(&png, "IDAT", compressed, (uint32_t)compressed_size) &&
             png_chunk(&png, "IEND", NULL, 0);
    free(compressed);
    if (!ok) { free(png.data); return 0; }
    size_t encoded_size = sodium_base64_encoded_len(png.length, sodium_base64_VARIANT_ORIGINAL);
    *base64 = malloc(encoded_size);
    if (!*base64) { free(png.data); return 0; }
    sodium_bin2base64(*base64, encoded_size, (const unsigned char *)png.data, png.length,
                      sodium_base64_VARIANT_ORIGINAL);
    sodium_memzero(png.data, png.length);
    free(png.data);
    return 1;
}

static const char *utf8_next_character(const char *p) {
    ++p;
    while (*p && ((unsigned char)*p & 0xc0u) == 0x80u) ++p;
    return p;
}

static const char *svg_line_end(const char *start, unsigned max_chars, unsigned *count) {
    const char *p = start, *last_space = NULL;
    *count = 0;
    while (*p && *count < max_chars) {
        if (*p == ' ') last_space = p;
        ++*count;
        p = utf8_next_character(p);
    }
    if (*p && last_space && last_space > start) p = last_space;
    *count = 0;
    for (const char *q = start; q < p; q = utf8_next_character(q)) ++*count;
    return p;
}

static int svg_line(TextBuffer *buffer, const char *text, unsigned max_chars, unsigned size,
                    double x, double y, unsigned max_lines) {
    const char *p = text;
    unsigned line = 0;
    while (*p && line < max_lines) {
        const char *start = p;
        unsigned count = 0;
        p = svg_line_end(start, max_chars, &count);
        if (!count) break;
        if (!appendf(buffer, "<text class=\"member-name\" x=\"%.1f\" y=\"%.1f\" font-size=\"%u\" fill=\"#f4f7fa\" font-family=\"system-ui,sans-serif\" font-weight=\"650\">", x, y + line * (size + 6), size) ||
            !xml_n(buffer, start, (size_t)(p - start)) || !append(buffer, "</text>")) return 0;
        ++line;
        if (*p == ' ') ++p;
    }
    if (*p && line && !appendf(buffer, "<text class=\"member-name\" x=\"%.1f\" y=\"%.1f\" font-size=\"%u\" fill=\"#f4f7fa\" font-family=\"system-ui,sans-serif\" font-weight=\"650\">…</text>", x, y + line * (size + 6), size)) return 0;
    return 1;
}

static const char *credential_role_mark(const char *role) {
    if (!strcmp(role, "Developer / Engineer")) return "DEV";
    if (!strcmp(role, "AI / ML Engineer")) return "AI";
    if (!strcmp(role, "Security Engineer / Researcher")) return "SEC";
    if (!strcmp(role, "Researcher")) return "RCH";
    if (!strcmp(role, "Systems / Infrastructure Engineer")) return "SYS";
    if (!strcmp(role, "Technical Founder / Entrepreneur")) return "FND";
    if (!strcmp(role, "Open-source Maintainer / Contributor")) return "OSS";
    if (!strcmp(role, "Product / UX Designer")) return "UX";
    if (!strcmp(role, "Educator / Technical Writer")) return "EDU";
    if (!strcmp(role, "Community organiser")) return "COM";
    if (!strcmp(role, "Student")) return "STU";
    return "OTH";
}

static int svg_begin(TextBuffer *b) {
    return appendf(b,
        "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"%.0f\" height=\"%.4f\" viewBox=\"0 0 %.0f %.4f\" role=\"img\">"
        "<defs><linearGradient id=\"glass\" x2=\"1\" y2=\"1\"><stop stop-color=\"#10212c\"/><stop offset=\".48\" stop-color=\"#0b121a\"/><stop offset=\"1\" stop-color=\"#101b27\"/></linearGradient>"
        "<linearGradient id=\"edge\" x2=\"0\" y2=\"1\"><stop stop-color=\"#7df7ff\"/><stop offset=\".48\" stop-color=\"#20d6ff\"/><stop offset=\"1\" stop-color=\"#12748b\"/></linearGradient>"
        "<pattern id=\"grid\" width=\"34\" height=\"34\" patternUnits=\"userSpaceOnUse\"><path d=\"M34 0H0V34\" fill=\"none\" stroke=\"#35c9df\" stroke-opacity=\".075\" stroke-width=\"1\"/></pattern>"
        "<style>.credential-label{font-family:monospace;font-size:11px;letter-spacing:2px}.credential-label-muted{fill:#8ea8b7}.credential-label-accent{fill:#20d6ff}.credential-structural{fill:none;stroke:#20d6ff;stroke-opacity:.17;stroke-width:2}.credential-section-rule{fill:none;stroke:#294650;stroke-width:1}.credential-field-rule{fill:none;stroke:#20d6ff;stroke-width:%.1f}</style>"
        "<clipPath id=\"portrait\"><rect x=\"60\" y=\"191\" width=\"214\" height=\"275\" rx=\"13\"/></clipPath></defs>"
        "<rect x=\"7\" y=\"7\" width=\"996\" height=\"623\" rx=\"26\" fill=\"#071018\" stroke=\"url(#edge)\" stroke-width=\"3\"/>"
        "<rect x=\"19\" y=\"19\" width=\"972\" height=\"599\" rx=\"18\" fill=\"url(#glass)\" stroke=\"#35505f\"/>"
        "<rect x=\"20\" y=\"20\" width=\"970\" height=\"597\" rx=\"18\" fill=\"url(#grid)\"/>"
        "<path class=\"credential-structural\" d=\"M20 100H%.1fL%.1f %.1fH990\"/>"
        "<path d=\"M58 55l20-12 20 12v24l-20 12-20-12z\" fill=\"none\" stroke=\"#20d6ff\" stroke-width=\"3\"/>"
        "<text x=\"116\" y=\"76\" fill=\"#f4f7fa\" font-family=\"system-ui,sans-serif\" font-size=\"33\" font-weight=\"750\" letter-spacing=\"5\">CODARIS</text>"
        "<text x=\"58\" y=\"118\" fill=\"#c3d3de\" font-family=\"system-ui,sans-serif\" font-size=\"12\" letter-spacing=\"2\">COALITION OF DEVELOPERS ADVANCING RESPONSIBLE</text>"
        "<text x=\"58\" y=\"138\" fill=\"#c3d3de\" font-family=\"system-ui,sans-serif\" font-size=\"12\" letter-spacing=\"2\">INTELLIGENT SYSTEMS</text>"
        "<text x=\"959\" y=\"50\" text-anchor=\"end\" fill=\"#20d6ff\" font-family=\"monospace\" font-size=\"11\" letter-spacing=\"2\">BUILD. VERIFY. ADVANCE.</text>",
        CREDENTIAL_VIEW_WIDTH, CREDENTIAL_VIEW_HEIGHT, CREDENTIAL_VIEW_WIDTH, CREDENTIAL_VIEW_HEIGHT,
        CREDENTIAL_ACCENT_LINE_WIDTH, CREDENTIAL_LABEL_LINE_START_X,
        CREDENTIAL_LABEL_LINE_X, CREDENTIAL_SECTION_LABEL_Y);
}

static int svg_label(TextBuffer *b, const char *text, double x, double y, int accent) {
    if (!appendf(b, "<text class=\"credential-label %s\" x=\"%.1f\" y=\"%.1f\">",
                 accent ? "credential-label-accent" : "credential-label-muted", x, y))
        return 0;
    return xml(b, text) && append(b, "</text>");
}

static int svg_field_rule(TextBuffer *b, double x, double y, double height) {
    return appendf(b, "<path class=\"credential-field-rule\" d=\"M%.1f %.1fV%.1f\"/>",
                   x, y, y + height);
}

static int svg_section_label(TextBuffer *b, const char *text) {
    return svg_label(b, text, 60, CREDENTIAL_SECTION_LABEL_Y, 1);
}

static int svg_front(TextBuffer *b, const CodarisCredential *c) {
    char *photo = NULL;
    if (!avatar_png_data(c, &photo)) return 0;
    int ok = svg_section_label(b, "MEMBER CREDENTIAL / VERIFIED IDENTITY") &&
             append(b, "<rect x=\"51\" y=\"182\" width=\"232\" height=\"293\" rx=\"16\" fill=\"#20d6ff\" fill-opacity=\".12\" stroke=\"#37dced\" stroke-opacity=\".62\" stroke-width=\"1\"/>");
    if (ok && photo) ok = append(b, "<image x=\"60\" y=\"191\" width=\"214\" height=\"275\" preserveAspectRatio=\"xMidYMid slice\" clip-path=\"url(#portrait)\" href=\"data:image/png;base64,") && append(b, photo) && append(b, "\"/>");
    if (ok && !photo) ok = append(b, "<g transform=\"translate(167 329)\"><circle r=\"68\" fill=\"#0e2732\" stroke=\"#20d6ff\" stroke-width=\"3\"/><path d=\"M0-48 42-24v48L0 48-42 24v-48z\" fill=\"none\" stroke=\"#20d6ff\" stroke-width=\"3\"/><circle r=\"13\" fill=\"#20d6ff\"/></g>");
    free(photo);
    if (!ok) return 0;
    unsigned name_count = utf8_count(c->display_name);
    unsigned displayed_chars = name_count > 90 ? 90 : name_count;
    unsigned name_lines = (displayed_chars + 29) / 30;
    if (name_lines < 1) name_lines = 1;
    if (name_lines > 3) name_lines = 3;
    unsigned name_size = name_count <= 30 ? 28 : 18;
    int number_label_y = 270 + (int)name_lines * 34;
    int number_value_y = number_label_y + 31;
    int detail_y = 321 + (int)name_lines * 34;
    const double field_height = 46;
    if (!svg_label(b, "MEMBER ACCESS CREDENTIAL", 315, 205, 1) ||
        !svg_line(b, c->display_name, 30, name_size, 315, 244, 4) ||
        !svg_label(b, "PUBLIC MEMBERSHIP NUMBER", 315, number_label_y, 0) ||
        !appendf(b, "<text x=\"315\" y=\"%d\" fill=\"#f4f7fa\" font-family=\"monospace\" font-size=\"22\" letter-spacing=\"2\">", number_value_y) ||
        !xml(b, c->membership_number) || !append(b, "</text>")) return 0;
    if (!svg_field_rule(b, 315, detail_y, field_height) ||
        !svg_label(b, "ROLE", 331, detail_y + 13, 0) ||
        !appendf(b, "<text x=\"331\" y=\"%d\" fill=\"#f4f7fa\" font-family=\"system-ui,sans-serif\" font-size=\"15\">", detail_y + 37) ||
        !xml(b, c->role) || !append(b, "</text>") ||
        !svg_field_rule(b, 674, detail_y, field_height) ||
        !svg_label(b, "STATUS", 690, detail_y + 13, 0) ||
        !appendf(b, "<text x=\"690\" y=\"%d\" fill=\"%s\" font-family=\"system-ui,sans-serif\" font-size=\"16\" font-weight=\"700\">", detail_y + 37, strcmp(c->status, "active") ? "#ffbf69" : "#58e6bc") ||
        !xml(b, c->status) || !append(b, "</text>")) return 0;
    if (!svg_label(b, "SCAN TO VERIFY", 790, 177, 0) ||
        !qr_svg(b, c, 790, 192, CREDENTIAL_QR_SIZE) ||
        !svg_label(b, "ISSUED", 790, 367, 0) ||
        !append(b, "<text x=\"790\" y=\"389\" fill=\"#20d6ff\" font-family=\"system-ui,sans-serif\" font-size=\"14\">") || !xml(b, c->issued_at) || !append(b, "</text>")) return 0;
    if (!append(b, "<rect x=\"60\" y=\"488\" width=\"884\" height=\"109\" rx=\"10\" fill=\"#08131c\" stroke=\"#20d6ff\" stroke-opacity=\".22\" stroke-width=\"1\"/>") ||
        !svg_label(b, "VERIFICATION BARCODE", 80, 508, 1) ||
        !code128_svg(b, c->verification_id, 76, 520, 658, 64) ||
        !append(b, "<text x=\"762\" y=\"538\" fill=\"#f4f7fa\" font-family=\"system-ui,sans-serif\" font-size=\"14\" font-weight=\"700\">Build.</text><text x=\"762\" y=\"557\" fill=\"#20d6ff\" font-family=\"system-ui,sans-serif\" font-size=\"14\" font-weight=\"700\">Verify.</text><text x=\"762\" y=\"576\" fill=\"#f4f7fa\" font-family=\"system-ui,sans-serif\" font-size=\"12\" font-weight=\"700\">Advance.</text>")) return 0;
    return 1;
}

static int svg_back(TextBuffer *b, const CodarisCredential *c) {
    if (!svg_section_label(b, "MEMBERSHIP VERIFICATION") ||
        !append(b, "<text x=\"70\" y=\"246\" fill=\"#f4f7fa\" font-family=\"system-ui,sans-serif\" font-size=\"34\" font-weight=\"700\">Build. Verify. Advance.</text><path class=\"credential-section-rule\" d=\"M70 276H940\"/>") ||
        !svg_label(b, "MEMBER", 70, 319, 0))
        return 0;
    unsigned chars = utf8_count(c->display_name);
    unsigned shown_chars = chars > 96 ? 96 : chars;
    unsigned max_chars = 32;
    unsigned lines = (shown_chars + max_chars - 1) / max_chars;
    if (lines < 1) lines = 1;
    unsigned size = chars <= 32 ? 22 : chars <= 64 ? 18 : chars <= 96 ? 16 : 14;
    unsigned line_height = size + 6;
    double name_start = 374.0 - (double)(lines - 1) * line_height / 2.0;
    if (!svg_line(b, c->display_name, max_chars, size, 70, name_start, 4))
        return 0;
    if (!svg_label(b, "PUBLIC MEMBERSHIP NUMBER", 70, 437, 0) ||
        !append(b, "<text x=\"70\" y=\"468\" fill=\"#20d6ff\" font-family=\"monospace\" font-size=\"17\">"))
        return 0;
    if (!xml(b, c->membership_number) || !append(b, "</text>") ||
        !svg_label(b, "STATUS", 70, 519, 0) ||
        !append(b, "<text x=\"70\" y=\"549\" fill=\"#58e6bc\" font-family=\"system-ui,sans-serif\" font-size=\"16\" font-weight=\"700\">"))
        return 0;
    if (!xml(b, c->status) || !append(b, "</text>") ||
        !svg_label(b, "ISSUED", 370, 519, 0) ||
        !append(b, "<text x=\"370\" y=\"549\" fill=\"#f4f7fa\" font-family=\"system-ui,sans-serif\" font-size=\"16\">"))
        return 0;
    if (!xml(b, c->issued_at) ||
        !append(b, "</text><text x=\"505\" y=\"589\" text-anchor=\"middle\" fill=\"#8ea8b7\" font-family=\"system-ui,sans-serif\" font-size=\"11\">This digital credential verifies current CODARIS membership. It is not a government ID or physical-access pass.</text><text class=\"credential-label credential-label-accent\" x=\"790\" y=\"177\">VERIFY CURRENT STATUS</text>") ||
        !qr_svg(b, c, 790, 192, CREDENTIAL_QR_SIZE) ||
        !append(b, "<text x=\"790\" y=\"367\" fill=\"#8ea8b7\" font-family=\"system-ui,sans-serif\" font-size=\"12\">codaris.org</text><path d=\"M700 445l18-11 18 11v22l-18 11-18-11z M744 445l18-11 18 11v22l-18 11-18-11z M788 445l18-11 18 11v22l-18 11-18-11z M832 445l18-11 18 11v22l-18 11-18-11z M876 445l18-11 18 11v22l-18 11-18-11z\" fill=\"none\" stroke=\"#20d6ff\" stroke-opacity=\".32\" stroke-width=\"2\"/>"))
        return 0;
    return 1;
}

int codaris_credential_svg(const CodarisCredential *credential, int back, char **output,
                           size_t *output_size) {
    if (!credential || !output || !output_size || !credential->display_name ||
        !credential->role || !credential->membership_number || !credential->verification_id ||
        !credential->status || !credential->issued_at || !credential->verification_url)
        return 0;
    *output = NULL; *output_size = 0;
    TextBuffer b = {0};
    int ok = svg_begin(&b) &&
             appendf(&b, "<text x=\"78\" y=\"72\" text-anchor=\"middle\" fill=\"#20d6ff\" font-family=\"monospace\" font-size=\"11\" font-weight=\"700\" letter-spacing=\".4\">%s</text>", credential_role_mark(credential->role)) &&
             (back ? svg_back(&b, credential) : svg_front(&b, credential)) && append(&b, "</svg>");
    if (!ok) { free(b.data); return 0; }
    *output = b.data; *output_size = b.length;
    return 1;
}

static void pdf_error(HPDF_STATUS error, HPDF_STATUS detail, void *user) {
    (void)error; (void)detail;
    int *failed = user;
    if (failed) *failed = 1;
}

static void pdf_box(HPDF_Page page, HPDF_REAL x, HPDF_REAL y, HPDF_REAL width, HPDF_REAL height,
                    HPDF_RGBColor color) {
    HPDF_Page_SetRGBFill(page, color.r, color.g, color.b);
    HPDF_Page_Rectangle(page, x, y, width, height);
    HPDF_Page_Fill(page);
}

static void pdf_qr(HPDF_Page page, const CodarisCredential *c, HPDF_REAL x, HPDF_REAL y,
                   HPDF_REAL box) {
    uint8_t qr[qrcodegen_BUFFER_LEN_MAX];
    int size = 0;
    if (!qr_matrix(c, qr, &size)) return;
    HPDF_REAL unit = box / (HPDF_REAL)(size + 8);
    pdf_box(page, x, y, box, box, (HPDF_RGBColor){1,1,1});
    HPDF_Page_SetRGBFill(page, .02f, .06f, .09f);
    for (int row = 0; row < size; ++row)
        for (int col = 0; col < size; ++col)
            if (qrcodegen_getModule(qr, col, row))
                HPDF_Page_Rectangle(page, x + unit * (4 + col), y + box - unit * (5 + row), unit, unit);
    HPDF_Page_Fill(page);
}

static void pdf_barcode(HPDF_Page page, const char *value, HPDF_REAL x, HPDF_REAL y,
                        HPDF_REAL width, HPDF_REAL height) {
    unsigned values[96]; size_t count = 0;
    if (!code128_values(value, values, &count)) return;
    unsigned modules = 20;
    for (size_t i=0; i<count; ++i)
        for (const char *p=code128_patterns[values[i]]; *p; ++p) modules += (unsigned)(*p-'0');
    HPDF_REAL unit = width/(HPDF_REAL)modules, cursor=x+unit*10;
    pdf_box(page,x,y,width,height,(HPDF_RGBColor){.96f,.98f,.99f});
    HPDF_REAL quiet_y = height * (6.0f / 64.0f);
    HPDF_Page_SetRGBFill(page, .02f,.06f,.09f);
    for (size_t i=0; i<count; ++i) {
        int black=1;
        for (const char *p=code128_patterns[values[i]]; *p; ++p) {
            HPDF_REAL run=unit*(HPDF_REAL)(*p-'0');
            if (black) HPDF_Page_Rectangle(page,cursor,y+quiet_y,run,height-quiet_y*2);
            cursor+=run; black=!black;
        }
    }
    HPDF_Page_Fill(page);
}

static HPDF_REAL credential_pdf_scale(void) {
    return (HPDF_REAL)(CREDENTIAL_PRINT_WIDTH / CREDENTIAL_VIEW_WIDTH);
}

static HPDF_REAL credential_pdf_x(double x) {
    return (HPDF_REAL)(x * credential_pdf_scale());
}

static HPDF_REAL credential_pdf_y(double y) {
    return (HPDF_REAL)(CREDENTIAL_PRINT_HEIGHT - y * credential_pdf_scale());
}

static void pdf_svg_text(HPDF_Page page, HPDF_Font font, HPDF_REAL svg_size, double x,
                         double baseline_y, HPDF_RGBColor color, const char *text,
                         int anchor_end, int anchor_middle) {
    const HPDF_REAL print_font_factor = 1.0f;
    HPDF_REAL size = svg_size * credential_pdf_scale() * print_font_factor;
    HPDF_Page_SetRGBFill(page, color.r, color.g, color.b);
    HPDF_Page_BeginText(page);
    HPDF_Page_SetFontAndSize(page, font, size);
    HPDF_REAL text_width = HPDF_Page_TextWidth(page, text);
    HPDF_REAL px = credential_pdf_x(x);
    if (anchor_middle)
        px -= text_width / 2;
    else if (anchor_end)
        px -= text_width;
    HPDF_Page_TextOut(page, px, credential_pdf_y(baseline_y), text);
    HPDF_Page_EndText(page);
}

static void pdf_svg_text_fit(HPDF_Page page, HPDF_Font font, HPDF_REAL svg_size,
                             double max_svg_width, double x, double baseline_y,
                             HPDF_RGBColor color, const char *text) {
    HPDF_REAL size = svg_size * credential_pdf_scale();
    HPDF_REAL max_width = (HPDF_REAL)(max_svg_width * credential_pdf_scale());
    HPDF_Page_SetFontAndSize(page, font, size);
    HPDF_REAL text_width = HPDF_Page_TextWidth(page, text);
    if (text_width > max_width && text_width > 0) {
        size *= max_width / text_width;
        HPDF_Page_SetFontAndSize(page, font, size);
    }
    HPDF_Page_SetRGBFill(page, color.r, color.g, color.b);
    HPDF_Page_BeginText(page);
    HPDF_Page_SetFontAndSize(page, font, size);
    HPDF_Page_TextOut(page, credential_pdf_x(x), credential_pdf_y(baseline_y), text);
    HPDF_Page_EndText(page);
}

static void pdf_svg_line(HPDF_Page page, double x1, double y1, double x2, double y2,
                         HPDF_RGBColor color, HPDF_REAL svg_width) {
    HPDF_Page_SetRGBStroke(page, color.r, color.g, color.b);
    HPDF_Page_SetLineWidth(page, svg_width * credential_pdf_scale());
    HPDF_Page_MoveTo(page, credential_pdf_x(x1), credential_pdf_y(y1));
    HPDF_Page_LineTo(page, credential_pdf_x(x2), credential_pdf_y(y2));
    HPDF_Page_Stroke(page);
}

static void pdf_svg_rect(HPDF_Page page, double x, double y, double width, double height,
                         double radius, HPDF_RGBColor fill, HPDF_RGBColor stroke,
                         HPDF_REAL stroke_width) {
    const HPDF_REAL sx = credential_pdf_x(x);
    const HPDF_REAL sy = credential_pdf_y(y + height);
    const HPDF_REAL sw = credential_pdf_x(width);
    const HPDF_REAL sh = credential_pdf_x(height);
    const HPDF_REAL r = credential_pdf_x(radius);
    const HPDF_REAL k = 0.55228475f;
    HPDF_Page_SetRGBFill(page, fill.r, fill.g, fill.b);
    HPDF_Page_SetRGBStroke(page, stroke.r, stroke.g, stroke.b);
    HPDF_Page_SetLineWidth(page, stroke_width * credential_pdf_scale());
    HPDF_Page_MoveTo(page, sx + r, sy);
    HPDF_Page_LineTo(page, sx + sw - r, sy);
    HPDF_Page_CurveTo(page, sx + sw - r + k*r, sy, sx + sw, sy + r - k*r, sx + sw, sy + r);
    HPDF_Page_LineTo(page, sx + sw, sy + sh - r);
    HPDF_Page_CurveTo(page, sx + sw, sy + sh - r + k*r, sx + sw - r + k*r, sy + sh, sx + sw - r, sy + sh);
    HPDF_Page_LineTo(page, sx + r, sy + sh);
    HPDF_Page_CurveTo(page, sx + r - k*r, sy + sh, sx, sy + sh - r + k*r, sx, sy + sh - r);
    HPDF_Page_LineTo(page, sx, sy + r);
    HPDF_Page_CurveTo(page, sx, sy + r - k*r, sx + r - k*r, sy, sx + r, sy);
    HPDF_Page_ClosePathFillStroke(page);
}

static void pdf_svg_qr(HPDF_Page page, const CodarisCredential *c, double x, double y, double box) {
    const HPDF_REAL s = credential_pdf_scale();
    pdf_qr(page, c, credential_pdf_x(x),
           (HPDF_REAL)(CREDENTIAL_PRINT_HEIGHT - (y + box) * s), (HPDF_REAL)(box * s));
}

static void pdf_svg_barcode(HPDF_Page page, const char *value, double x, double y,
                            double width, double height) {
    const HPDF_REAL s = credential_pdf_scale();
    pdf_barcode(page, value, credential_pdf_x(x),
                (HPDF_REAL)(CREDENTIAL_PRINT_HEIGHT - (y + height) * s),
                (HPDF_REAL)(width * s), (HPDF_REAL)(height * s));
}

static void pdf_svg_image_crop(HPDF_Page page, HPDF_Image image, double x, double y,
                              double width, double height) {
    if (!image) return;
    HPDF_REAL s = credential_pdf_scale();
    HPDF_REAL px = credential_pdf_x(x), py = (HPDF_REAL)(CREDENTIAL_PRINT_HEIGHT - (y + height) * s);
    HPDF_REAL pw = (HPDF_REAL)(width * s), ph = (HPDF_REAL)(height * s);
    HPDF_REAL side = ph;
    HPDF_Page_GSave(page);
    HPDF_Page_Rectangle(page, px, py, pw, ph);
    HPDF_Page_Clip(page);
    HPDF_Page_EndPath(page);
    HPDF_Page_DrawImage(page, image, px - (side - pw) / 2, py, side, side);
    HPDF_Page_GRestore(page);
}

static const HPDF_RGBColor PDF_INK = {.96f,.98f,.99f};
static const HPDF_RGBColor PDF_MUTED = {.56f,.66f,.72f};
static const HPDF_RGBColor PDF_CYAN = {.13f,.84f,.94f};
static const HPDF_RGBColor PDF_GREEN = {.35f,.90f,.74f};
static const HPDF_RGBColor PDF_AMBER = {.96f,.68f,.30f};
static const HPDF_RGBColor PDF_PANEL = {.03f,.075f,.105f};

static void pdf_svg_label(HPDF_Page page, HPDF_Font font, const char *text,
                          double x, double y, int accent) {
    pdf_svg_text(page, font, 11, x, y, accent ? PDF_CYAN : PDF_MUTED, text, 0, 0);
}

static int pdf_svg_name_lines(HPDF_Page page, HPDF_Font font, const char *text,
                              double x, double y, unsigned max_chars, unsigned size,
                              unsigned max_lines, double max_svg_width) {
    const char *p = text;
    unsigned line = 0;
    while (*p && line < max_lines) {
        const char *start = p, *last_space = NULL;
        unsigned count = 0;
        while (*p && count < max_chars) {
            if (*p == ' ') last_space = p;
            ++count;
            p = utf8_next_character(p);
        }
        if (*p && last_space && last_space > start) p = last_space;
        size_t bytes = (size_t)(p - start);
        char *part = malloc(bytes + 1);
        if (!part) return 0;
        memcpy(part, start, bytes);
        part[bytes] = 0;
        pdf_svg_text_fit(page, font, (HPDF_REAL)size, max_svg_width, x,
                         y + (double)line * (size + 6), PDF_INK, part);
        free(part);
        ++line;
        if (*p == ' ') ++p;
    }
    if (*p && line < max_lines) {
        pdf_svg_text_fit(page, font, (HPDF_REAL)size, max_svg_width, x,
                         y + (double)line * (size + 6), PDF_INK, "…");
    }
    return 1;
}

static void pdf_svg_emblem(HPDF_Page page, HPDF_Font bold, const char *role) {
    static const double x[] = {58,78,98,98,78,58,58};
    static const double y[] = {55,43,55,79,91,79,55};
    HPDF_Page_SetRGBStroke(page, PDF_CYAN.r, PDF_CYAN.g, PDF_CYAN.b);
    HPDF_Page_SetLineWidth(page, credential_pdf_scale() * 3.0f);
    HPDF_Page_MoveTo(page, credential_pdf_x(x[0]), credential_pdf_y(y[0]));
    for (size_t i = 1; i < sizeof(x) / sizeof(x[0]); ++i)
        HPDF_Page_LineTo(page, credential_pdf_x(x[i]), credential_pdf_y(y[i]));
    HPDF_Page_Stroke(page);
    pdf_svg_text(page, bold, 11, 78, 70, PDF_CYAN, credential_role_mark(role), 0, 1);
}

static void pdf_placeholder_avatar(HPDF_Page page) {
    const double cx = 167, cy = 329;
    HPDF_Page_SetRGBStroke(page, PDF_CYAN.r, PDF_CYAN.g, PDF_CYAN.b);
    HPDF_Page_SetLineWidth(page, credential_pdf_scale() * 3);
    HPDF_Page_MoveTo(page, credential_pdf_x(cx), credential_pdf_y(cy - 48));
    HPDF_Page_LineTo(page, credential_pdf_x(cx + 42), credential_pdf_y(cy - 24));
    HPDF_Page_LineTo(page, credential_pdf_x(cx + 42), credential_pdf_y(cy + 24));
    HPDF_Page_LineTo(page, credential_pdf_x(cx), credential_pdf_y(cy + 48));
    HPDF_Page_LineTo(page, credential_pdf_x(cx - 42), credential_pdf_y(cy + 24));
    HPDF_Page_LineTo(page, credential_pdf_x(cx - 42), credential_pdf_y(cy - 24));
    HPDF_Page_ClosePathStroke(page);
    HPDF_Page_SetRGBFill(page, PDF_CYAN.r, PDF_CYAN.g, PDF_CYAN.b);
    HPDF_Page_Circle(page, credential_pdf_x(cx), credential_pdf_y(cy), credential_pdf_x(13));
    HPDF_Page_Fill(page);
}

static void pdf_draw_front_content(HPDF_Page page, HPDF_Font font, HPDF_Font bold,
                                   const CodarisCredential *c, HPDF_Image avatar) {
    pdf_svg_label(page, font, "MEMBER CREDENTIAL / VERIFIED IDENTITY", 60, CREDENTIAL_SECTION_LABEL_Y, 1);
    pdf_svg_rect(page, 51, 182, 232, 293, 16, PDF_PANEL, (HPDF_RGBColor){.22f,.87f,.93f}, 1);
    if (avatar) pdf_svg_image_crop(page, avatar, 60, 191, 214, 275);
    else pdf_placeholder_avatar(page);

    unsigned name_count = utf8_count(c->display_name);
    unsigned chars = name_count > 90 ? 90 : name_count;
    unsigned lines = (chars + 29) / 30;
    if (lines < 1) lines = 1;
    if (lines > 3) lines = 3;
    int number_label_y = 270 + (int)lines * 34;
    int detail_y = 321 + (int)lines * 34;
    pdf_svg_label(page, font, "MEMBER ACCESS CREDENTIAL", 315, 205, 1);
    unsigned name_size = name_count <= 30 ? 28 : 18;
    if (!pdf_svg_name_lines(page, bold, c->display_name, 315, 244, 30, name_size, 4, 440)) return;
    pdf_svg_label(page, font, "PUBLIC MEMBERSHIP NUMBER", 315, number_label_y, 0);
    pdf_svg_text_fit(page, bold, 22, 440, 315, number_label_y + 31, PDF_INK, c->membership_number);
    pdf_svg_line(page, 315, detail_y, 315, detail_y + 46, PDF_CYAN, 3);
    pdf_svg_label(page, font, "ROLE", 331, detail_y + 13, 0);
    pdf_svg_text_fit(page, font, 15, 320, 331, detail_y + 37, PDF_INK, c->role);
    pdf_svg_line(page, 674, detail_y, 674, detail_y + 46, PDF_CYAN, 3);
    pdf_svg_label(page, font, "STATUS", 690, detail_y + 13, 0);
    HPDF_RGBColor status_color = strcmp(c->status, "active") ? PDF_AMBER : PDF_GREEN;
    pdf_svg_text(page, bold, 16, 690, detail_y + 37, status_color, c->status, 0, 0);
    pdf_svg_label(page, font, "SCAN TO VERIFY", 790, 177, 0);
    pdf_svg_qr(page, c, 790, 192, CREDENTIAL_QR_SIZE);
    pdf_svg_label(page, font, "ISSUED", 790, 367, 0);
    pdf_svg_text_fit(page, font, 14, 180, 790, 389, PDF_CYAN, c->issued_at);
    pdf_svg_rect(page, 60, 488, 884, 109, 10, PDF_PANEL, (HPDF_RGBColor){.13f,.84f,.94f}, 1);
    pdf_svg_label(page, font, "VERIFICATION BARCODE", 80, 508, 1);
    pdf_svg_barcode(page, c->verification_id, 76, 520, 658, 64);
    pdf_svg_text(page, bold, 14, 762, 538, PDF_INK, "Build.", 0, 0);
    pdf_svg_text(page, bold, 14, 762, 557, PDF_CYAN, "Verify.", 0, 0);
    pdf_svg_text(page, bold, 12, 762, 576, PDF_INK, "Advance.", 0, 0);
}

static void pdf_draw_back_content(HPDF_Page page, HPDF_Font font, HPDF_Font bold,
                                  const CodarisCredential *c) {
    pdf_svg_label(page, font, "MEMBERSHIP VERIFICATION", 60, CREDENTIAL_SECTION_LABEL_Y, 1);
    pdf_svg_text_fit(page, bold, 34, 650, 70, 246, PDF_INK, "Build. Verify. Advance.");
    pdf_svg_line(page, 70, 276, 940, 276, (HPDF_RGBColor){.16f,.27f,.31f}, 1);
    pdf_svg_label(page, font, "MEMBER", 70, 319, 0);
    unsigned name_count = utf8_count(c->display_name);
    unsigned shown = name_count > 96 ? 96 : name_count;
    unsigned lines = (shown + 31) / 32;
    if (lines < 1) lines = 1;
    unsigned size = 14;
    if (name_count <= 32) size = 22;
    else if (name_count <= 64) size = 18;
    else if (name_count <= 96) size = 16;
    double start = 374.0 - (double)(lines - 1) * (size + 6) / 2.0;
    if (!pdf_svg_name_lines(page, bold, c->display_name, 70, start, 32, size, 4, 650)) return;
    pdf_svg_label(page, font, "PUBLIC MEMBERSHIP NUMBER", 70, 437, 0);
    pdf_svg_text_fit(page, bold, 17, 270, 70, 468, PDF_CYAN, c->membership_number);
    pdf_svg_label(page, font, "STATUS", 70, 519, 0);
    HPDF_RGBColor status_color = strcmp(c->status, "active") ? PDF_AMBER : PDF_GREEN;
    pdf_svg_text(page, bold, 16, 70, 549, status_color, c->status, 0, 0);
    pdf_svg_label(page, font, "ISSUED", 330, 519, 0);
    pdf_svg_text_fit(page, font, 16, 280, 370, 549, PDF_INK, c->issued_at);
    pdf_svg_label(page, font, "VERIFY CURRENT STATUS", 790, 177, 0);
    pdf_svg_qr(page, c, 790, 192, CREDENTIAL_QR_SIZE);
    pdf_svg_text(page, font, 12, 790, 367, PDF_MUTED, "codaris.org", 0, 0);
    pdf_svg_text(page, font, 11, 505, 589, PDF_MUTED,
        "This digital credential verifies current CODARIS membership. It is not a government ID or physical-access pass.", 0, 1);
    for (int n = 0; n < 5; n++) {
        double x = 700.0 + n * 44.0;
        double px[] = {x, x + 18, x + 36, x + 36, x + 18, x};
        double py[] = {445, 434, 445, 467, 478, 467};
        HPDF_Page_SetRGBStroke(page, PDF_CYAN.r, PDF_CYAN.g, PDF_CYAN.b);
        HPDF_Page_SetLineWidth(page, credential_pdf_scale() * 2);
        HPDF_Page_MoveTo(page, credential_pdf_x(px[0]), credential_pdf_y(py[0]));
        for (size_t i = 1; i < 6; i++)
            HPDF_Page_LineTo(page, credential_pdf_x(px[i]), credential_pdf_y(py[i]));
        HPDF_Page_ClosePathStroke(page);
    }
}

static void pdf_draw_page(HPDF_Doc pdf, HPDF_Font font, HPDF_Font bold,
                          const CodarisCredential *c, int back, HPDF_Image avatar) {
    const HPDF_REAL w = (HPDF_REAL)CREDENTIAL_PRINT_WIDTH;
    const HPDF_REAL h = (HPDF_REAL)CREDENTIAL_PRINT_HEIGHT;
    HPDF_Page page = HPDF_AddPage(pdf);
    HPDF_Page_SetWidth(page, w);
    HPDF_Page_SetHeight(page, h);
    pdf_box(page, 0, 0, w, h, (HPDF_RGBColor){.027f,.063f,.09f});
    pdf_svg_rect(page, 7, 7, 996, CREDENTIAL_VIEW_HEIGHT - 14, 26,
                 (HPDF_RGBColor){.027f,.063f,.09f}, PDF_CYAN, CREDENTIAL_ACCENT_LINE_WIDTH);
    pdf_svg_rect(page, 19, 19, 972, CREDENTIAL_VIEW_HEIGHT - 38, 18,
                 (HPDF_RGBColor){.043f,.071f,.095f}, (HPDF_RGBColor){.21f,.31f,.37f}, 1);
    pdf_svg_emblem(page, bold, c->role);
    pdf_svg_text(page, bold, 33, 116, 76, PDF_INK, "CODARIS", 0, 0);
    pdf_svg_text_fit(page, font, 12, 900, 58, 118, (HPDF_RGBColor){.76f,.83f,.87f},
                     "COALITION OF DEVELOPERS ADVANCING RESPONSIBLE");
    pdf_svg_text(page, font, 12, 58, 138, (HPDF_RGBColor){.76f,.83f,.87f},
                 "INTELLIGENT SYSTEMS", 0, 0);
    pdf_svg_text(page, font, 11, 959, 50, PDF_CYAN, "BUILD. VERIFY. ADVANCE.", 1, 0);
    pdf_svg_line(page, 20, 100, CREDENTIAL_LABEL_LINE_START_X, 100, (HPDF_RGBColor){.13f,.84f,.94f}, 2);
    pdf_svg_line(page, CREDENTIAL_LABEL_LINE_START_X, 100, CREDENTIAL_LABEL_LINE_X,
                 CREDENTIAL_SECTION_LABEL_Y, (HPDF_RGBColor){.13f,.84f,.94f}, 2);
    pdf_svg_line(page, CREDENTIAL_LABEL_LINE_X, CREDENTIAL_SECTION_LABEL_Y, 990,
                 CREDENTIAL_SECTION_LABEL_Y, (HPDF_RGBColor){.13f,.84f,.94f}, 2);
    if (back) pdf_draw_back_content(page, font, bold, c);
    else pdf_draw_front_content(page, font, bold, c, avatar);
}

int codaris_credential_pdf(const CodarisCredential *credential, const char *font_path,
                           unsigned char **output, size_t *output_size) {
    if (!credential || !font_path || !*font_path || !output || !output_size) return 0;
    *output=NULL; *output_size=0;
    int failed=0;
    HPDF_Doc pdf=HPDF_New(pdf_error,&failed);
    if (!pdf) return 0;
    HPDF_SetCompressionMode(pdf,HPDF_COMP_ALL);
    HPDF_UseUTFEncodings(pdf);
    const char *font_name=HPDF_LoadTTFontFromFile(pdf,font_path,HPDF_TRUE);
    HPDF_Font font=font_name?HPDF_GetFont(pdf,font_name,"UTF-8"):NULL;
    HPDF_Font bold=font;
    HPDF_Image avatar=NULL;
    unsigned char *rgb=NULL;
    if (credential->avatar_rgba && credential->avatar_size==40000) {
        rgb=malloc(30000);
        if (rgb) {
            for (size_t i=0;i<10000;++i) {
                const unsigned char *p=credential->avatar_rgba+i*4;
                unsigned alpha=p[3];
                rgb[i*3]=(unsigned char)((p[0]*alpha+8u*(255u-alpha))/255u);
                rgb[i*3+1]=(unsigned char)((p[1]*alpha+8u*(255u-alpha))/255u);
                rgb[i*3+2]=(unsigned char)((p[2]*alpha+8u*(255u-alpha))/255u);
            }
            avatar=HPDF_LoadRawImageFromMem(pdf,rgb,100,100,HPDF_CS_DEVICE_RGB,8);
        }
    }
    if (!font || (credential->avatar_rgba && credential->avatar_size==40000 && !avatar)) failed=1;
    if (!failed) {
        HPDF_SetInfoAttr(pdf,HPDF_INFO_TITLE,"CODARIS Membership Credential");
        HPDF_SetInfoAttr(pdf,HPDF_INFO_CREATOR,"CODARIS");
        pdf_draw_page(pdf,font,bold,credential,0,avatar);
        pdf_draw_page(pdf,font,bold,credential,1,avatar);
        if (!failed && HPDF_SaveToStream(pdf)==HPDF_OK) {
            HPDF_UINT size=HPDF_GetStreamSize(pdf);
            unsigned char *data=malloc(size);
            if (data && HPDF_ReadFromStream(pdf,data,&size)==HPDF_OK) {
                *output=data; *output_size=size;
            } else free(data);
        }
    }
    free(rgb);
    HPDF_Free(pdf);
    return *output!=NULL && *output_size>0 && !failed;
}
