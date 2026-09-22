#ifndef CODARIS_GLOBE_H
#define CODARIS_GLOBE_H
#include <math.h>
#include <stdarg.h>
#include "globe_data.h"

/* Projection and rotation state are C-owned. DOM bridge only copies geometry. */
typedef struct { double x, y, z; } GlobePoint;
static double globe_lon = -35, globe_lat = 30;
static double globe_zoom = 1;
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
    document.querySelectorAll('.globe-country').forEach(function (country) {
        const visible = Array.from(country.querySelectorAll('.country-surface path'))
            .some(function (path) { return !!path.getAttribute('d'); });
        country.setAttribute('tabindex', visible ? '0' : '-1');
    });
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
    document.getElementById(UTF8ToString(id)).setAttribute('d', UTF8ToString(path));
})
EM_JS(void, globe_node, (int i, double x, double y, int visible), {
    const node = document.getElementById('node-' + i);
    node.style.display = visible ? "" : 'none';
    for (const circle of node.children) {
        circle.setAttribute('cx', x); circle.setAttribute('cy', y);
    }
    if (i < 2) document.getElementById('label-' + i).style.display = visible ? "" : 'none';
})
static GlobePoint globe_project(double lon, double lat) {
    const double rad = 0.017453292519943295;
    double l=(lon-globe_lon)*rad, p=lat*rad, c=globe_lat*rad;
    return (GlobePoint){cos(p)*sin(l), cos(c)*sin(p)-sin(c)*cos(p)*cos(l),
                        sin(c)*sin(p)+cos(c)*cos(p)*cos(l)};
}
static void globe_append(const char *format, ...) {
    if (globe_failed) return;
    va_list args; va_start(args, format);
    int n=vsnprintf(globe_path+globe_used,sizeof(globe_path)-globe_used,format,args);
    va_end(args);
    if(n<0 || (size_t)n>=sizeof(globe_path)-globe_used) { globe_failed=1; return; }
    globe_used+=(size_t)n;
}
static void globe_begin(void) { globe_used=0; globe_failed=0; globe_path[0]='\0'; }
static void globe_publish(const char *id) {
    if (!globe_failed) globe_set_path(id,globe_path);
}
static void globe_ring(unsigned first, unsigned count) {
    size_t n=0;
    for(unsigned i=0;i<count;i++) {
        unsigned j=first+(i+1)%count;
        GlobePoint a=globe_project(globe_points[first+i][0],globe_points[first+i][1]);
        GlobePoint b=globe_project(globe_points[j][0],globe_points[j][1]);
        if(n+2>=sizeof(clipped)/sizeof(clipped[0])) {globe_failed=1;return;}
        if(a.z>=0) clipped[n++]=a;
        if((a.z>=0)!=(b.z>=0)) {
            double t=a.z/(a.z-b.z), x=a.x+t*(b.x-a.x), y=a.y+t*(b.y-a.y), norm=hypot(x,y);
            if(norm>0) clipped[n++]=(GlobePoint){x/norm,y/norm,0};
        }
    }
    if(n<3)return;
    globe_append("M%.2f,%.2f",330+246*clipped[0].x,326-246*clipped[0].y);
    for(size_t i=0;i<n;i++) {
        GlobePoint a=clipped[i],b=clipped[(i+1)%n];
        if(fabs(a.z)<1e-9 && fabs(b.z)<1e-9)
            globe_append("A246,246 0 0,%d %.2f,%.2f",a.x*b.y-a.y*b.x<0,330+246*b.x,326-246*b.y);
        else globe_append("L%.2f,%.2f",330+246*b.x,326-246*b.y);
    }
    globe_append("Z");
}
EMSCRIPTEN_KEEPALIVE void codaris_globe_rotate(double dx, double dy, int reset) {
    if(!isfinite(dx)||!isfinite(dy))return;
    codaris_globe_highlight(-1);
    if(reset) {globe_lon=-35;globe_lat=30;globe_zoom=1;globe_camera(globe_zoom);}
    else { globe_lon=fmod(globe_lon-dx*.3+540,360)-180; globe_lat=fmax(-80,fmin(80,globe_lat+dy*.3)); }
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
            globe_append("%c%.1f,%.1f",active?'L':'M',330+246*p.x,326-246*p.y);active=1;
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
            globe_append("M%.1f,%.1fQ%.1f,%.1f %.1f,%.1f",ax,ay,(ax+bx)/2,(ay+by)/2-65,bx,by);
        }
        int n=snprintf(id,sizeof(id),"route-%d",i);if(n>0&&(size_t)n<sizeof(id))globe_publish(id);
        n=snprintf(id,sizeof(id),"route-base-%d",i);if(n>0&&(size_t)n<sizeof(id))globe_publish(id);
    }
    for(int i=0;i<2;i++) {
        globe_begin();globe_append("M%.1f,%.1f%s",330+246*nodes[i].x,326-246*nodes[i].y,i?"L164 302H65":"L467 137H602");
        globe_publish(i?"leader-1":"leader-0");
    }
    globe_country_visibility();
}
#endif
