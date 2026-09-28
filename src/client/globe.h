#ifndef CODARIS_GLOBE_H
#define CODARIS_GLOBE_H
#include <math.h>
#include "globe_data.h"

/* Projection and rotation state are C-owned. DOM bridge only copies geometry. */
typedef struct { double x, y, z; } GlobePoint;
typedef struct { double lon, sin_lat, cos_lat; } GlobeTrig;
static double globe_lon = -35, globe_lat = 30;
static double globe_zoom = 1;
static const size_t globe_point_count = sizeof(globe_points) / sizeof(globe_points[0]);
static GlobeTrig globe_point_trig[sizeof(globe_points) / sizeof(globe_points[0])];
static int globe_point_trig_ready;
static double globe_sin_lat, globe_cos_lat;
static char globe_path[1048576];
static size_t globe_used;
static int globe_failed;
static GlobePoint clipped[30000];

/* C owns country selection; the DOM bridge only presents the selected region. */
static int globe_highlighted = -1;
EM_JS(void, globe_country_view, (int country), {
    const svg = document.querySelector('.network-globe');
    if (!svg) return;
    const previous = svg.querySelector('.is-highlighted');
    if (previous) previous.classList.remove('is-highlighted');
    const next = svg.querySelector('[data-country="' + country + '"]');
    const label = document.getElementById('globe-country-name');
    if (next) {
        next.classList.add('is-highlighted');
        // Paint the raised country above neighbours; its base hit area stays fixed.
        const shade = svg.querySelector('.globe-shade');
        if (shade.parentNode.moveBefore) shade.parentNode.moveBefore(next, shade);
        else if (!next.contains(document.activeElement)) shade.parentNode.insertBefore(next, shade);
    }
    if (label) label.textContent = next ? next.getAttribute('aria-label') : 'Explore a country';
})
EMSCRIPTEN_KEEPALIVE void codaris_globe_highlight(int country) {
    if (country < -1 || (country >= 0 && (unsigned)country >= globe_country_count)) return;
    if (country == globe_highlighted) return;
    globe_highlighted = country;
    globe_country_view(country);
}
EM_JS(void, globe_country_visibility, (void), {
    const svg = document.querySelector('.network-globe');
    if (!svg) return;
    let countries = svg.__codarisCountryPaths;
    if (!countries) {
        countries = Array.from(svg.querySelectorAll('.globe-country'), country =>
            [country, country.querySelectorAll('.country-surface path')]);
        svg.__codarisCountryPaths = countries;
    }
    for (let i = 0; i < countries.length; ++i) {
        const country = countries[i][0];
        const paths = countries[i][1];
        let visible = false;
        for (const path of paths) {
            if (path.getAttribute('d')) { visible = true; break; }
        }
        const tabindex = visible ? '0' : '-1';
        if (country.getAttribute('tabindex') !== tabindex) country.setAttribute('tabindex', tabindex);
    }
})

/* Zoom the SVG camera without changing geographic coordinates or orientation. */
EM_JS(void, globe_camera, (double zoom), {
    const svg = document.querySelector('.network-globe');
    const size = 660 / zoom;
    svg.setAttribute('viewBox', [330 - size / 2, 330 - size / 2, size, size].join(' '));
})
EMSCRIPTEN_KEEPALIVE void codaris_globe_zoom(double delta, int mode) {
    if (!isfinite(delta)) return;
    /* Normalize browser wheel units, then bound each event and the total scale. */
    if (mode == 1) delta *= 16;
    else if (mode == 2) delta *= 660;
    delta = fmax(-240, fmin(240, delta));
    globe_zoom = fmax(0.8, fmin(2.5, globe_zoom * exp(-delta * 0.0015)));
    globe_camera(globe_zoom);
}

EM_JS(void, globe_set_path, (const char *id, const char *path), {
    const svg = document.querySelector('.network-globe');
    if (!svg) return;
    let paths = svg.__codarisPathNodes;
    if (!paths) {
        paths = new Map();
        svg.querySelectorAll('[id]').forEach(node => {
            paths.set(node.id, node);
            if (node.hasAttribute('d')) node.__codarisPathData = node.getAttribute('d');
        });
        svg.__codarisPathNodes = paths;
    }
    const node = paths.get(UTF8ToString(id));
    if (node) {
        const data = UTF8ToString(path);
        if (node.__codarisPathData !== data) {
            node.setAttribute('d', data);
            node.__codarisPathData = data;
        }
    }
})
EM_JS(void, globe_node, (int i, double x, double y, int visible), {
    const node = document.getElementById('node-' + i);
    node.style.display = visible ? "" : 'none';
    for (const circle of node.children) {
        circle.setAttribute('cx', x); circle.setAttribute('cy', y);
    }
    if (i < 2) document.getElementById('label-' + i).style.display = visible ? "" : 'none';
})
static void globe_prepare_projection(void) {
    const double rad = 0.017453292519943295;
    double lat = globe_lat * rad;
    globe_sin_lat = sin(lat); globe_cos_lat = cos(lat);
    if (globe_point_trig_ready) return;
    for (size_t i = 0; i < globe_point_count; ++i) {
        double point_lat = globe_points[i][1] * rad;
        globe_point_trig[i] = (GlobeTrig){globe_points[i][0], sin(point_lat), cos(point_lat)};
    }
    globe_point_trig_ready = 1;
}
static GlobePoint globe_project_trig(GlobeTrig point) {
    const double rad = 0.017453292519943295;
    double delta_lon = (point.lon - globe_lon) * rad;
    double sin_delta = sin(delta_lon), cos_delta = cos(delta_lon);
    return (GlobePoint){point.cos_lat * sin_delta,
                        globe_cos_lat * point.sin_lat - globe_sin_lat * point.cos_lat * cos_delta,
                        globe_sin_lat * point.sin_lat + globe_cos_lat * point.cos_lat * cos_delta};
}
static GlobePoint globe_project(double lon, double lat) {
    const double rad = 0.017453292519943295;
    double longitude = (lon - globe_lon) * rad, point_lat = lat * rad;
    double sin_delta = sin(longitude), cos_delta = cos(longitude);
    double sin_lat = sin(point_lat), cos_lat = cos(point_lat);
    return (GlobePoint){cos_lat * sin_delta,
                        globe_cos_lat * sin_lat - globe_sin_lat * cos_lat * cos_delta,
                        globe_sin_lat * sin_lat + globe_cos_lat * cos_lat * cos_delta};
}
static void globe_append_char(char value) {
    if (globe_failed) return;
    if (globe_used + 1 >= sizeof(globe_path)) { globe_failed = 1; return; }
    globe_path[globe_used++] = value;
}
static void globe_append_text(const char *text) {
    while (*text && !globe_failed) globe_append_char(*text++);
}
static void globe_append_uint(unsigned long long value) {
    char digits[24]; size_t count = 0;
    do { digits[count++] = (char)('0' + value % 10); value /= 10; } while (value && count < sizeof(digits));
    while (count) globe_append_char(digits[--count]);
}
static void globe_append_fixed(double value, unsigned decimals) {
    const unsigned long long scale = decimals == 2 ? 100 : 10;
    unsigned long long scaled = (unsigned long long)nearbyint(fabs(value) * (double)scale);
    if (signbit(value)) globe_append_char('-');
    globe_append_uint(scaled / scale);
    globe_append_char('.');
    unsigned long long fraction = scaled % scale;
    if (decimals == 2 && fraction < 10) globe_append_char('0');
    globe_append_uint(fraction);
}
static void globe_append_xy(char command, double x, double y, unsigned decimals) {
    globe_append_char(command);
    globe_append_fixed(x, decimals);
    globe_append_char(',');
    globe_append_fixed(y, decimals);
}
static void globe_append_arc(int sweep, double x, double y) {
    globe_append_text("A246,246 0 0,");
    globe_append_char(sweep ? '1' : '0');
    globe_append_char(' ');
    globe_append_fixed(x, 2);
    globe_append_char(',');
    globe_append_fixed(y, 2);
}
static void globe_begin(void) { globe_used=0; globe_failed=0; globe_path[0]='\0'; }
static void globe_publish(const char *id) {
    if (!globe_failed) { globe_path[globe_used]='\0'; globe_set_path(id,globe_path); }
}
static void globe_ring(unsigned first, unsigned count) {
    size_t n=0;
    GlobePoint a=globe_project_trig(globe_point_trig[first]);
    for(unsigned i=0;i<count;i++) {
        unsigned j=first+(i+1)%count;
        GlobePoint b=globe_project_trig(globe_point_trig[j]);
        if(n+2>=sizeof(clipped)/sizeof(clipped[0])) {globe_failed=1;return;}
        if(a.z>=0) clipped[n++]=a;
        if((a.z>=0)!=(b.z>=0)) {
            double t=a.z/(a.z-b.z), x=a.x+t*(b.x-a.x), y=a.y+t*(b.y-a.y), norm=hypot(x,y);
            if(norm>0) clipped[n++]=(GlobePoint){x/norm,y/norm,0};
        }
        a=b;
    }
    if(n<3)return;
    globe_append_xy('M',330+246*clipped[0].x,326-246*clipped[0].y,2);
    for(size_t i=0;i<n;i++) {
        GlobePoint a=clipped[i],b=clipped[(i+1)%n];
        if(fabs(a.z)<1e-9 && fabs(b.z)<1e-9)
            globe_append_arc(a.x*b.y-a.y*b.x<0,330+246*b.x,326-246*b.y);
        else globe_append_xy('L',330+246*b.x,326-246*b.y,2);
    }
    globe_append_char('Z');
}
EMSCRIPTEN_KEEPALIVE void codaris_globe_rotate(double dx, double dy, int reset) {
    if(!isfinite(dx)||!isfinite(dy))return;
    codaris_globe_highlight(-1);
    if(reset) {globe_lon=-35;globe_lat=30;globe_zoom=1;globe_camera(globe_zoom);}
    else { globe_lon=fmod(globe_lon-dx*.3+540,360)-180; globe_lat=fmax(-80,fmin(80,globe_lat+dy*.3)); }
    globe_prepare_projection();
    char id[48];
    for(size_t i=0;i<sizeof(globe_polys)/sizeof(globe_polys[0]);i++) {
        globe_begin();
        for(unsigned j=0;j<globe_polys[i][1];j++) {
            const unsigned *r=globe_rings[globe_polys[i][0]+j];globe_ring(r[0],r[1]);
        }
        int n=snprintf(id,sizeof(id),"geo-%zu",i);
        if(n>0 && (size_t)n<sizeof(id))globe_publish(id);
    }
    for(int axis=0;axis<2;axis++)for(int fixed=axis?-60:-180;fixed<=(axis?60:180);fixed+=30) {
        globe_begin();int active=0;
        for(int v=axis?-180:-90;v<=(axis?180:90);v+=2) {
            GlobePoint p=axis?globe_project(v,fixed):globe_project(fixed,v);
            if(p.z<0){active=0;continue;}
            globe_append_xy(active?'L':'M',330+246*p.x,326-246*p.y,1);active=1;
        }
        int n=snprintf(id,sizeof(id),"grid-%d-%d",axis,fixed);
        if(n>0 && (size_t)n<sizeof(id))globe_publish(id);
    }
    const double locations[][2]={{-.12,51.5},{-74,40.7},{-122.4,37.8},{-46.6,-23.5},{18.4,-33.9},{13.4,52.5}};
    GlobePoint nodes[6];
    for(int i=0;i<6;i++) {
        nodes[i]=globe_project(locations[i][0],locations[i][1]);
        globe_node(i,330+246*nodes[i].x,326-246*nodes[i].y,nodes[i].z>=0);
    }
    for(int i=1;i<6;i++) {
        globe_begin();
        if(nodes[0].z>=0 && nodes[i].z>=0) {
            double ax=330+246*nodes[0].x,ay=326-246*nodes[0].y,bx=330+246*nodes[i].x,by=326-246*nodes[i].y;
            globe_append_xy('M',ax,ay,1);
            globe_append_char('Q');
            globe_append_fixed((ax+bx)/2,1); globe_append_char(',');
            globe_append_fixed((ay+by)/2-65,1); globe_append_char(' ');
            globe_append_fixed(bx,1); globe_append_char(','); globe_append_fixed(by,1);
        }
        int n=snprintf(id,sizeof(id),"route-%d",i);if(n>0&&(size_t)n<sizeof(id))globe_publish(id);
        n=snprintf(id,sizeof(id),"route-base-%d",i);if(n>0&&(size_t)n<sizeof(id))globe_publish(id);
    }
    for(int i=0;i<2;i++) {
        globe_begin();globe_append_xy('M',330+246*nodes[i].x,326-246*nodes[i].y,1);
        globe_append_text(i?"L164 302H65":"L467 137H602");
        globe_publish(i?"leader-1":"leader-0");
    }
    globe_country_visibility();
}
#endif
