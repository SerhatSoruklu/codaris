"""Generate a decorative orthographic SVG from Natural Earth GeoJSON.
Usage: python3 scripts/assets/generate-globe.py /path/to/ne_50m_admin_0_countries.geojson
Offline asset tooling only; no browser application logic or runtime dependencies.
"""
import json, math, sys
from pathlib import Path
from html import escape
D=math.pi/180
R=246
cx,cy=330,326
lon0,lat0=-35*D,30*D

def point(lon,lat):
    lon,lat=lon*D-lon0,lat*D
    return (math.cos(lat)*math.sin(lon),math.cos(lat0)*math.sin(lat)-math.sin(lat0)*math.cos(lat)*math.cos(lon),math.sin(lat0)*math.sin(lat)+math.cos(lat0)*math.cos(lat)*math.cos(lon))
def xy(p):return (cx+R*p[0],cy-R*p[1])
def ring(coords):
    points=[point(*p[:2]) for p in coords]
    if not any(p[2]>=0 for p in points):return ''
    # Clip at the hemisphere plane, adding limb arcs rather than long closing chords.
    out=[]
    for a,b in zip(points,points[1:]+points[:1]):
        if a[2]>=0:out.append(a)
        if (a[2]>=0)!=(b[2]>=0):
            t=a[2]/(a[2]-b[2]);q=[a[i]+t*(b[i]-a[i]) for i in range(3)]
            norm=math.hypot(q[0],q[1]);out.append((q[0]/norm,q[1]/norm,0))
    if len(out)<3:return ''
    x,y=xy(out[0]);s=f'M{x:.2f},{y:.2f}'
    for a,b in zip(out,out[1:]+out[:1]):
        x,y=xy(b)
        if abs(a[2])<1e-9 and abs(b[2])<1e-9:
            cross=a[0]*b[1]-a[1]*b[0]
            s+=f'A{R},{R} 0 0,{int(cross<0)} {x:.2f},{y:.2f}'
        else:s+=f'L{x:.2f},{y:.2f}'
    return s+'Z'

s=['''<svg tabindex="0" aria-describedby="globe-help" class="network-globe" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 660 660" role="group" aria-labelledby="globe-title globe-desc">
<title id="globe-title">Global network — The United British Isles and United States</title>
<desc id="globe-desc">An Atlantic-facing globe using real country outlines. Great Britain, Ireland and nearby islands are blue; the United States is red. Animated routes illustrate connections, not live traffic. The United British Isles is a custom regional label.</desc>
<defs>
<radialGradient id="ocean" cx="32%" cy="24%" r="80%"><stop stop-color="#194759"/><stop offset=".55" stop-color="#0b2636"/><stop offset="1" stop-color="#020812"/></radialGradient>
<radialGradient id="atmosphere"><stop offset=".78" stop-color="#42daed" stop-opacity="0"/><stop offset=".91" stop-color="#42daed" stop-opacity=".13"/><stop offset="1" stop-color="#42daed" stop-opacity="0"/></radialGradient>
<radialGradient id="shade" cx="28%" cy="22%" r="85%"><stop offset=".25" stop-color="#010813" stop-opacity="0"/><stop offset=".8" stop-color="#010813" stop-opacity=".25"/><stop offset="1" stop-color="#010813" stop-opacity=".9"/></radialGradient>
<clipPath id="earth-clip"><circle cx="330" cy="326" r="246"/></clipPath>
</defs>
<circle cx="330" cy="326" r="285" fill="url(#atmosphere)"/>
<circle cx="330" cy="326" r="279" fill="none" stroke="#294453" stroke-dasharray="2 9"/>
<path d="M330 34v18m0 548v18M38 326h18m548 0h18" stroke="#52a4ba"/>
<circle cx="330" cy="326" r="246" fill="url(#ocean)" stroke="#5796ac" stroke-width="1.2"/>
<g clip-path="url(#earth-clip)">''']
# Geographic graticule, showing only the visible hemisphere.
for axis in [0,1]:
 for fixed in range(-180 if axis==0 else -60,181 if axis==0 else 61,30):
  path='';active=False
  for v in range(-90 if axis==0 else -180,91 if axis==0 else 181,2):
   p=point(fixed,v) if axis==0 else point(v,fixed)
   if p[2]<0:active=False;continue
   x,y=xy(p);path+=('L' if active else 'M')+f'{x:.1f},{y:.1f}';active=True
  s.append(f'<path id="grid-{axis}-{fixed}" d="{path}" fill="none" stroke="#74a9bb" stroke-opacity=".14" stroke-width=".6"/>')
geo_points=[]
geo_rings=[]
geo_polys=[]
features=json.loads(Path(sys.argv[1]).read_text())['features']
for country_id,f in enumerate(features):
 name=f['properties'].get('ADMIN','');g=f['geometry'];polys=g['coordinates'] if g['type']=='MultiPolygon' else [g['coordinates']]
 blue=name in ['United Kingdom','Ireland','Isle of Man','Guernsey','Jersey'];red=name=='United States of America'
 fill='#368dff' if blue else '#f15665' if red else '#285262';stroke='#a0d0ff' if blue else '#ffabb2' if red else '#69909e'
 kind=' usa' if red else ' isles' if blue else ''
 country_paths=[];country_uses=[];visible=False
 for poly in polys:
  # British overseas territories are not part of the custom regional highlight.
  isles=blue and any(-12<p[0]<3 and 49<p[1]<62 for p in poly[0])
  first_ring=len(geo_rings)
  for r in poly:
   # Retain detailed regional outlines; simplify other rings for interactive redraws.
   sampled=r if isles else r[::3]+[r[-1]]
   geo_rings.append((len(geo_points),len(sampled)))
   geo_points.extend(sampled)
  polygon_id=len(geo_polys)
  geo_polys.append((first_ring,len(poly)))
  d=''.join(ring(r if isles else r[::3]+[r[-1]]) for r in poly)
  visible=visible or bool(d)
  country_paths.append(f'<path id="geo-{polygon_id}" d="{d}" fill="{fill if not blue or isles else "#285262"}" stroke="{stroke if not blue or isles else "#69909e"}" stroke-width="{.9 if isles else .5}" stroke-linejoin="round" fill-rule="evenodd"><title>{escape(name)}</title></path>')
  country_uses.append(f'<use href="#geo-{polygon_id}"/>')
 s.append(f'<g class="globe-country{kind}" data-country="{country_id}" aria-label="{escape(name, quote=True)}" role="img" tabindex="{0 if visible else -1}"><g class="country-surface">'+''.join(country_paths)+'</g><g class="country-lift" aria-hidden="true">'+''.join(country_uses)+'</g></g>')
s.append('<circle class="globe-shade" cx="330" cy="326" r="246" fill="url(#shade)"/></g>')
# London, New York and other illustrative network endpoints.
nodes=[(-.12,51.5),(-74,40.7),(-122.4,37.8),(-46.6,-23.5),(18.4,-33.9),(13.4,52.5)]
pts=[xy(point(*p)) for p in nodes]
for i in range(1,len(pts)):
 a,b=pts[0],pts[i];mid=((a[0]+b[0])/2,(a[1]+b[1])/2-65)
 path=f'M{a[0]:.1f},{a[1]:.1f}Q{mid[0]:.1f},{mid[1]:.1f} {b[0]:.1f},{b[1]:.1f}'
 s.append(f'<path id="route-base-{i}" d="{path}" fill="none" stroke="#7ceff2" stroke-opacity=".25" stroke-width="1.2"/><path id="route-{i}" class="network-route" d="{path}" fill="none" stroke="#9efcff" stroke-width="2" stroke-linecap="round" pathLength="100" stroke-dasharray="5 95"/>')
for i,(x,y) in enumerate(pts):
 color='#73b4ff' if i==0 else '#ff8390' if i in [1,2] else '#8ff3eb'
 s.append(f'<g id="node-{i}"><circle cx="{x:.1f}" cy="{y:.1f}" r="8" fill="{color}" opacity=".16"/><circle cx="{x:.1f}" cy="{y:.1f}" r="3" fill="{color}"/></g>')
# Callouts remain legible and outside the geographical silhouettes.
x,y=pts[0];s.append(f'<g id="label-0"><path id="leader-0" d="M{x:.1f},{y:.1f}L467 137H602" fill="none" stroke="#70b2ff"/><text x="466" y="116" fill="#9ac9ff" font-size="12" font-family="monospace">THE UNITED</text><text x="466" y="132" fill="#9ac9ff" font-size="12" font-family="monospace">BRITISH ISLES</text><text x="466" y="153" fill="#9ac9ff" font-size="9" font-family="monospace">LONDON · 51.50° N / 0.12° W</text></g>')
x,y=pts[1];s.append(f'<g id="label-1"><path id="leader-1" d="M{x:.1f},{y:.1f}L164 302H65" fill="none" stroke="#ff7c8a"/><text x="65" y="320" fill="#ff9ba5" font-size="12" font-family="monospace">UNITED STATES</text><text x="65" y="290" fill="#ff9ba5" font-size="9" font-family="monospace">NEW YORK · 40.70° N / 74.00° W</text></g>')
s.append('</svg>')
svg='\n'.join(s)
Path('web/assets/network.svg').write_text(svg+'\n')
p=Path('web/pages/home.html');html=p.read_text();start=html.index('          <!-- GLOBE START -->');end=html.index('          <!-- GLOBE END -->',start);html=html[:start]+'          <!-- GLOBE START -->\n'+svg+'\n'+html[end:];p.write_text(html)

# C owns interactive projection; compact geographic fixture is compiled into Wasm.
header = "/* Generated by scripts/assets/generate-globe.py; Natural Earth public domain. */\n"
header += "static const unsigned globe_country_count = %d;\n" % len(features)
header += "static const float globe_points[][2] = {\n" + ",\n".join("{%.5ff,%.5ff}" % tuple(p[:2]) for p in geo_points) + "\n};\n"
header += "static const unsigned globe_rings[][2] = {" + ",".join("{%d,%d}" % r for r in geo_rings) + "};\n"
header += "static const unsigned globe_polys[][2] = {" + ",".join("{%d,%d}" % r for r in geo_polys) + "};\n"
Path("src/client/globe_data.h").write_text(header)
