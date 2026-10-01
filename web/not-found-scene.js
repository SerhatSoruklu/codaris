import * as THREE from '/three.module.js';

const host = document.querySelector('[data-route-scene]');
const canvas = host?.querySelector('[data-route-scene-canvas]');
const selectionLabel = host?.querySelector('[data-route-scene-selection]');
if (!host || !canvas || !selectionLabel) throw new Error('CODARIS route scene mount is incomplete');

function bootScene() {
const reduceMotion = window.matchMedia('(prefers-reduced-motion: reduce)');
const partNames = {
  wall: ['ROUTE MAP', 'NETWORK / TRACE ACTIVE'],
  chair: ['OPERATOR CHAIR', 'SEAT SUPPORT / OCCUPIED'],
  head: ['OPERATOR', 'TRACE ANALYSIS / ACTIVE'],
  jacket: ['FIELD JACKET', 'OPERATOR / OUTER LAYER'],
  arms: ['INPUT', 'HANDS / ROUTE RETRY'],
  pants: ['FIELD TROUSERS', 'OPERATOR / LOWER LAYER'],
  boots: ['FLOOR CONTACT', 'OPERATOR / BOOTS'],
  carpet: ['FLOOR MAT', 'WORKSPACE / FLOOR PLANE'],
  desk: ['WORKSTATION WS-04', 'DESK / SYSTEM INTERFACE'],
  keyboard: ['KEYBOARD', 'PRIMARY INPUT / INTERFACE READY'],
  mouse: ['MOUSE', 'POINTER INPUT / READY'],
  phone: ['FIELD DEVICE', 'REFERENCE NOTES / READY'],
  mug: ['COFFEE MUG', 'DESK / COFFEE BREAK'],
  monitor: ['DISPLAY / NODE 04', 'ROUTE TRACE / HTTP 404'],
  pc: ['COMPUTE NODE', 'RUNTIME / ACTIVE'],
};

let renderer;
try {
  renderer = new THREE.WebGLRenderer({
    canvas,
    antialias: true,
    alpha: false,
    powerPreference: 'low-power',
    preserveDrawingBuffer: false,
  });
} catch (error) {
  selectionLabel.textContent = 'NODE 04 · WEBGL UNAVAILABLE';
  console.info('CODARIS 404 scene is using its accessible fallback.', error);
  return;
}

renderer.outputColorSpace = THREE.SRGBColorSpace;
renderer.toneMapping = THREE.ACESFilmicToneMapping;
renderer.toneMappingExposure = 1.12;
renderer.shadowMap.enabled = true;
renderer.shadowMap.type = THREE.PCFShadowMap;

const scene = new THREE.Scene();
scene.background = new THREE.Color(0x07121a);
scene.fog = new THREE.FogExp2(0x07121a, 0.031);

const camera = new THREE.PerspectiveCamera(23, 1, 0.1, 60);
const cameraAim = new THREE.Vector3(0.1, 2.0, 0);
const cameraHome = new THREE.Vector3(6.4, 4.15, 13.1);
camera.position.copy(cameraHome);
camera.lookAt(cameraAim);

const hemi = new THREE.HemisphereLight(0x94d8e8, 0x111318, 0.95);
scene.add(hemi);
const keyLight = new THREE.DirectionalLight(0xd4f5ff, 2.5);
keyLight.position.set(-3.8, 7.5, 5.2);
keyLight.castShadow = true;
keyLight.shadow.mapSize.set(512, 512);
keyLight.shadow.camera.left = -6;
keyLight.shadow.camera.right = 6;
keyLight.shadow.camera.top = 7;
keyLight.shadow.camera.bottom = -4;
keyLight.shadow.bias = -0.00045;
scene.add(keyLight);
const cyanLight = new THREE.PointLight(0x09c5e5, 15, 9, 1.8);
cyanLight.intensity = 20;
cyanLight.position.set(1.45, 3.3, 1.25);
scene.add(cyanLight);
const warmLight = new THREE.PointLight(0xff914f, 15, 7, 2);
warmLight.intensity = 19;
warmLight.position.set(-3.0, 4.4, 0.65);
scene.add(warmLight);
const operatorFill = new THREE.PointLight(0xffb27b, 34, 7.5, 1.7);
operatorFill.intensity = 16;
operatorFill.position.set(-2.65, 3.25, 1.65);
scene.add(operatorFill);

const objectMeshes = new Map();
const partMaterials = new Map();
const geometryCache = new Map();
const staticBatches = new Map();
const interactiveBatches = new Map();
const lineBatches = new Map();
const emissiveBarMaterials = new Map();
const materials = {
  wall: new THREE.MeshStandardMaterial({ color: 0x101d25, roughness: 0.88 }),
  wallPanel: new THREE.MeshStandardMaterial({ color: 0x0b171e, roughness: 0.72, metalness: 0.18 }),
  metal: new THREE.MeshStandardMaterial({ color: 0x293941, roughness: 0.35, metalness: 0.82 }),
  metalDark: new THREE.MeshStandardMaterial({ color: 0x121d23, roughness: 0.42, metalness: 0.78 }),
  wood: new THREE.MeshStandardMaterial({ color: 0x3b2d25, roughness: 0.62, metalness: 0.12 }),
  woodEdge: new THREE.MeshStandardMaterial({ color: 0x8b5d3e, roughness: 0.52, metalness: 0.08 }),
  jacket: new THREE.MeshStandardMaterial({ color: 0x18262d, roughness: 0.9 }),
  jacketLight: new THREE.MeshStandardMaterial({ color: 0x34454c, roughness: 0.84 }),
  pants: new THREE.MeshStandardMaterial({ color: 0x202d34, roughness: 0.93 }),
  skin: new THREE.MeshStandardMaterial({ color: 0xb9785e, roughness: 0.82 }),
  hair: new THREE.MeshStandardMaterial({ color: 0x211b1a, roughness: 0.93 }),
  boot: new THREE.MeshStandardMaterial({ color: 0x303b40, roughness: 0.62, metalness: 0.1 }),
  rubber: new THREE.MeshStandardMaterial({ color: 0x151d22, roughness: 0.78 }),
  plastic: new THREE.MeshStandardMaterial({ color: 0x1b262c, roughness: 0.58, metalness: 0.08 }),
  chairFabric: new THREE.MeshStandardMaterial({ color: 0x202d34, roughness: 0.94 }),
  plantStem: new THREE.MeshStandardMaterial({ color: 0x285a50, roughness: 0.9 }),
  glow: new THREE.MeshStandardMaterial({ color: 0x42e8de, emissive: 0x20a9a5, emissiveIntensity: 1.1, roughness: 0.32 }),
  amber: new THREE.MeshStandardMaterial({ color: 0xffa56d, emissive: 0x9c3614, emissiveIntensity: 1.2, roughness: 0.42 }),
};

function freezeTransform(mesh) {
  mesh.matrixAutoUpdate = false;
  mesh.updateMatrix();
  return mesh;
}

function geometryFor(key, create) {
  let geometry = geometryCache.get(key);
  if (!geometry) {
    geometry = create();
    geometryCache.set(key, geometry);
  }
  return geometry;
}

function boxGeometry(size) {
  return geometryFor(`box:${size.join(',')}`, () => new THREE.BoxGeometry(...size));
}

function roundedBoxGeometry(size, bevel = 0.025) {
  const [width, height, depth] = size;
  const edge = Math.min(bevel, width * 0.12, height * 0.28, depth * 0.28);
  const key = `rounded:${size.join(',')}:${edge}`;
  return geometryFor(key, () => {
    const shapeWidth = width - edge * 2;
    const shapeHeight = height - edge * 2;
    const radius = Math.max(0.004, Math.min(edge * 1.2, shapeWidth * 0.14, shapeHeight * 0.18));
    const halfWidth = shapeWidth / 2;
    const halfHeight = shapeHeight / 2;
    const shape = new THREE.Shape();
    shape.moveTo(-halfWidth + radius, -halfHeight);
    shape.lineTo(halfWidth - radius, -halfHeight);
    shape.quadraticCurveTo(halfWidth, -halfHeight, halfWidth, -halfHeight + radius);
    shape.lineTo(halfWidth, halfHeight - radius);
    shape.quadraticCurveTo(halfWidth, halfHeight, halfWidth - radius, halfHeight);
    shape.lineTo(-halfWidth + radius, halfHeight);
    shape.quadraticCurveTo(-halfWidth, halfHeight, -halfWidth, halfHeight - radius);
    shape.lineTo(-halfWidth, -halfHeight + radius);
    shape.quadraticCurveTo(-halfWidth, -halfHeight, -halfWidth + radius, -halfHeight);
    const extrudeDepth = Math.max(0.008, depth - edge * 2);
    const geometry = new THREE.ExtrudeGeometry(shape, {
      depth: extrudeDepth,
      bevelEnabled: true,
      bevelSegments: 1,
      bevelSize: edge,
      bevelThickness: edge,
      curveSegments: 2,
      steps: 1,
    });
    geometry.translate(0, 0, -extrudeDepth / 2);
    geometry.computeVertexNormals();
    return geometry;
  });
}

function sphereGeometry(radius, widthSegments, heightSegments) {
  return geometryFor(`sphere:${radius}:${widthSegments}:${heightSegments}`, () => new THREE.SphereGeometry(radius, widthSegments, heightSegments));
}

function capsuleGeometry(radius, length, radialSegments) {
  return geometryFor(`capsule:${radius}:${length}:${radialSegments}`, () => new THREE.CapsuleGeometry(radius, length, 4, radialSegments));
}

function materialForPart(material, part) {
  const key = `${part}:${material.uuid}`;
  let clone = partMaterials.get(key);
  if (!clone) {
    clone = material.clone();
    partMaterials.set(key, clone);
  }
  return clone;
}

function queueInteractiveMesh(mesh, parent, part) {
  const materialKey = Array.isArray(mesh.material) ? mesh.material.map(material => material.uuid).join(',') : mesh.material.uuid;
  const key = `${part}:${parent.uuid}:${mesh.geometry.uuid}:${materialKey}`;
  let batch = interactiveBatches.get(key);
  if (!batch) {
    batch = { part, parent, geometry: mesh.geometry, material: mesh.material, matrices: [] };
    interactiveBatches.set(key, batch);
  }
  batch.matrices.push(mesh.matrix.clone());
}

function register(mesh, part) {
  mesh.castShadow = false;
  mesh.receiveShadow = false;
  mesh.userData.part = part || '';
  if (part) {
    mesh.material = Array.isArray(mesh.material) ? mesh.material.map(material => materialForPart(material, part)) : materialForPart(mesh.material, part);
    freezeTransform(mesh);
    queueInteractiveMesh(mesh, scene, part);
    return mesh;
  }
  freezeTransform(mesh);
  const materialKey = Array.isArray(mesh.material) ? mesh.material.map(material => material.uuid).join(',') : mesh.material.uuid;
  const key = `${mesh.geometry.uuid}:${materialKey}`;
  let batch = staticBatches.get(key);
  if (!batch) {
    batch = { geometry: mesh.geometry, material: mesh.material, meshes: [] };
    staticBatches.set(key, batch);
  }
  batch.meshes.push(mesh);
  return mesh;
}

function registerChild(mesh, parent, part) {
  mesh.castShadow = false;
  mesh.receiveShadow = false;
  mesh.userData.part = part || '';
  if (part) {
    mesh.material = Array.isArray(mesh.material) ? mesh.material.map(material => materialForPart(material, part)) : materialForPart(mesh.material, part);
    freezeTransform(mesh);
    queueInteractiveMesh(mesh, parent, part);
    return mesh;
  }
  freezeTransform(mesh);
  parent.add(mesh);
  return mesh;
}

function box(size, position, material, part = '', parent = scene, rotation = null) {
  const mesh = new THREE.Mesh(boxGeometry(size), material);
  mesh.position.set(...position);
  if (rotation) mesh.rotation.set(...rotation);
  if (parent === scene) return register(mesh, part);
  return registerChild(mesh, parent, part);
}

function roundedBox(size, position, material, part = '', parent = scene, rotation = null, bevel = 0.025) {
  const mesh = new THREE.Mesh(roundedBoxGeometry(size, bevel), material);
  mesh.position.set(...position);
  if (rotation) mesh.rotation.set(...rotation);
  if (parent === scene) return register(mesh, part);
  return registerChild(mesh, parent, part);
}

function sphere(radius, position, scale, material, part = '', parent = scene, segments = 16) {
  const heightSegments = Math.max(8, Math.floor(segments * 0.7));
  const mesh = new THREE.Mesh(sphereGeometry(radius, segments, heightSegments), material);
  mesh.position.set(...position);
  mesh.scale.set(...scale);
  if (parent === scene) return register(mesh, part);
  return registerChild(mesh, parent, part);
}

function segment(start, end, radius, material, part = '', parent = scene, radialSegments = 10) {
  const from = new THREE.Vector3(...start);
  const to = new THREE.Vector3(...end);
  const direction = new THREE.Vector3().subVectors(to, from);
  const length = Math.max(0.03, direction.length() - radius * 2);
  const mesh = new THREE.Mesh(capsuleGeometry(radius, length, radialSegments), material);
  mesh.position.copy(from).add(to).multiplyScalar(0.5);
  mesh.quaternion.setFromUnitVectors(new THREE.Vector3(0, 1, 0), direction.normalize());
  if (parent === scene) return register(mesh, part);
  return registerChild(mesh, parent, part);
}

function line(points, color, opacity = 1, width = 1) {
  const key = `${color}:${opacity}:${width}`;
  let batch = lineBatches.get(key);
  if (!batch) {
    batch = { color, opacity, width, positions: [] };
    lineBatches.set(key, batch);
  }
  for (let index = 1; index < points.length; index += 1) {
    batch.positions.push(...points[index - 1], ...points[index]);
  }
}

function emissiveBar(size, position, color, intensity = 1) {
  const key = `${color}:${intensity}`;
  let mat = emissiveBarMaterials.get(key);
  if (!mat) {
    mat = new THREE.MeshStandardMaterial({ color, emissive: color, emissiveIntensity: intensity, roughness: 0.35 });
    emissiveBarMaterials.set(key, mat);
  }
  const mesh = new THREE.Mesh(boxGeometry(size), mat);
  mesh.position.set(...position);
  return register(mesh, '');
}

function finalizeStaticBatches() {
  for (const { geometry, material, meshes } of staticBatches.values()) {
    if (meshes.length === 1) {
      scene.add(meshes[0]);
      continue;
    }
    const batch = new THREE.InstancedMesh(geometry, material, meshes.length);
    for (let index = 0; index < meshes.length; index += 1) {
      batch.setMatrixAt(index, meshes[index].matrix);
    }
    batch.instanceMatrix.needsUpdate = true;
    batch.castShadow = false;
    batch.receiveShadow = false;
    freezeTransform(batch);
    scene.add(batch);
  }
  staticBatches.clear();
  for (const { part, parent, geometry, material, matrices } of interactiveBatches.values()) {
    const batch = new THREE.InstancedMesh(geometry, material, matrices.length);
    const center = new THREE.Vector3();
    for (const matrix of matrices) {
      center.x += matrix.elements[12];
      center.y += matrix.elements[13];
      center.z += matrix.elements[14];
    }
    center.divideScalar(matrices.length);
    for (let index = 0; index < matrices.length; index += 1) {
      const matrix = matrices[index];
      matrix.elements[12] -= center.x;
      matrix.elements[13] -= center.y;
      matrix.elements[14] -= center.z;
      batch.setMatrixAt(index, matrix);
    }
    batch.position.copy(center);
    batch.instanceMatrix.needsUpdate = true;
    batch.computeBoundingBox();
    batch.userData.part = part;
    batch.userData.initialScale = batch.scale.clone();
    batch.castShadow = false;
    batch.receiveShadow = false;
    freezeTransform(batch);
    parent.add(batch);
    const meshes = objectMeshes.get(part) || [];
    meshes.push(batch);
    objectMeshes.set(part, meshes);
  }
  interactiveBatches.clear();
  for (const { color, opacity, width, positions } of lineBatches.values()) {
    const geometry = new THREE.BufferGeometry();
    geometry.setAttribute('position', new THREE.Float32BufferAttribute(positions, 3));
    geometry.computeBoundingSphere();
    const material = new THREE.LineBasicMaterial({ color, transparent: opacity < 1, opacity, linewidth: width });
    const mesh = new THREE.LineSegments(geometry, material);
    freezeTransform(mesh);
    scene.add(mesh);
  }
  lineBatches.clear();
  geometryCache.clear();
  partMaterials.clear();
  emissiveBarMaterials.clear();
}

// A shallow room shell, built as geometry so the camera move reveals real depth.
const floor = new THREE.Mesh(new THREE.PlaneGeometry(22, 18), new THREE.MeshStandardMaterial({ color: 0x111a20, roughness: 0.92, metalness: 0.08 }));
floor.rotation.x = -Math.PI / 2;
floor.position.set(0, -0.08, 0);
floor.receiveShadow = true;
freezeTransform(floor);
scene.add(floor);
box([10.6, 5.8, 0.22], [0, 2.88, -2.55], materials.wall);
box([10.6, 0.15, 0.4], [0, 0.05, -2.38], materials.metalDark);
box([10.6, 0.11, 0.13], [0, 5.62, -2.37], materials.metalDark);
box([0.12, 5.6, 0.12], [-5.18, 2.8, -2.36], materials.metalDark);
box([0.12, 5.6, 0.12], [5.18, 2.8, -2.36], materials.metalDark);

// Floor seams and restrained low-level guide lighting.
for (let x = -5; x <= 5; x += 0.78) line([[x, -0.061, -2.2], [x, -0.061, 3.2]], 0x31505a, 0.22);
for (let z = -2; z <= 3; z += 0.62) line([[-5, -0.06, z], [5, -0.06, z]], 0x31505a, 0.18);
emissiveBar([4.2, 0.025, 0.035], [0.8, 0.006, 1.45], 0x16a9c0, 0.75);
emissiveBar([0.035, 0.02, 3.8], [-4.7, 0.006, 0.2], 0x16a9c0, 0.7);

// Rear route board: real panel, etched connectors, and embedded status LEDs.
roundedBox([2.35, 1.8, 0.16], [-3.06, 3.73, -2.38], materials.wallPanel, 'wall', scene, null, 0.035);
roundedBox([2.25, 1.68, 0.04], [-3.06, 3.73, -2.27], materials.metalDark, 'wall', scene, null, 0.018);
for (let i = 0; i < 6; i += 1) {
  const y = 3.06 + i * 0.27;
  line([[-4.04, y, -2.24], [-2.13, y + (i % 2 ? -0.18 : 0.2), -2.23], [-1.98, y + (i % 2 ? -0.12 : 0.26), -2.23]], i % 2 ? 0x238da0 : 0x315360, 0.82);
}
const routeNodes = [[-3.92, 4.36, -2.18], [-3.37, 4.51, -2.18], [-2.87, 4.2, -2.18], [-2.38, 4.44, -2.18], [-3.5, 3.88, -2.18], [-2.95, 3.72, -2.18], [-2.34, 3.55, -2.18], [-3.74, 3.28, -2.18], [-2.8, 3.18, -2.18]];
const routeSignals = [];
for (const [index, point] of routeNodes.entries()) {
  const material = (index === 2 ? materials.amber : materials.glow).clone();
  const node = new THREE.Mesh(sphereGeometry(index === 2 ? 0.055 : 0.035, 10, 8), material);
  node.position.set(...point);
  node.updateMatrix();
  node.matrixAutoUpdate = false;
  scene.add(node);
  routeSignals.push({ node, material, phase: index * 0.71 });
}
for (let i = 0; i < 5; i += 1) {
  box([0.46, 0.57, 0.06], [-4.45 + i * 0.42, 2.65, -2.3], materials.metalDark);
  emissiveBar([0.24, 0.018, 0.02], [-4.45 + i * 0.42, 2.38, -2.25], i % 2 ? 0x13c7df : 0xff9b63, 0.38);
}

// Right side glass and distant server equipment set up a second depth plane.
box([2.42, 4.42, 0.09], [3.75, 3.0, -2.31], new THREE.MeshStandardMaterial({ color: 0x091923, roughness: 0.24, metalness: 0.24 }));
for (let x = 2.61; x <= 4.91; x += 0.57) box([0.024, 4.32, 0.035], [x, 3.0, -2.23], materials.metal);
for (let y = 0.9; y <= 5.05; y += 0.63) box([2.35, 0.022, 0.03], [3.75, y, -2.22], materials.metal);
for (let index = 0; index < 19; index += 1) {
  const x = 2.7 + ((index * 37) % 210) / 100;
  const y = 1.2 + ((index * 23) % 350) / 100;
  emissiveBar([0.025, 0.025, 0.02], [x, y, -2.19], index % 4 ? 0x20a8c5 : 0xffa16a, index % 4 ? 0.55 : 0.44);
}
roundedBox([0.72, 3.45, 0.62], [4.28, 1.75, -1.78], materials.metalDark, 'pc', scene, null, 0.045);
roundedBox([0.72, 3.45, 0.62], [3.39, 1.75, -1.78], materials.metalDark, 'pc', scene, null, 0.045);
for (const x of [3.39, 4.28]) {
  roundedBox([0.57, 3.24, 0.035], [x, 1.76, -1.445], materials.wallPanel, 'pc', scene, null, 0.018);
  for (const edgeX of [x - 0.31, x + 0.31]) box([0.018, 3.1, 0.035], [edgeX, 1.76, -1.405], materials.metal, 'pc');
  for (let y = 0.43; y < 3.35; y += 0.49) {
    roundedBox([0.5, 0.04, 0.44], [x, y, -1.385], materials.metal, 'pc', scene, null, 0.012);
    emissiveBar([0.035, 0.025, 0.018], [x - 0.18, y + 0.08, -1.18], 0x18b8d4, 0.58);
    emissiveBar([0.035, 0.025, 0.018], [x + 0.17, y + 0.08, -1.18], 0xffa16a, 0.4);
  }
}

// Carpet is a raised fabric plane with a stitched edge and sparse woven lines.
roundedBox([5.55, 0.055, 3.32], [-0.7, 0.02, 0.47], new THREE.MeshStandardMaterial({ color: 0x18232b, roughness: 1 }), 'carpet', scene, null, 0.025);
for (const x of [-3.39, 1.99]) box([0.026, 0.006, 3.2], [x, 0.052, 0.47], materials.jacketLight, 'carpet');
for (let z = -0.99; z <= 1.91; z += 0.28) line([[-3.3, 0.053, z], [1.9, 0.053, z]], 0x36515a, 0.22);

// Operator's chair with a visible back, seat, column, five spokes, and castors.
const chair = new THREE.Group();
chair.position.set(-1.9, 0, -0.1);
scene.add(chair);
roundedBox([0.22, 1.16, 0.78], [-0.38, 1.97, -0.015], materials.chairFabric, 'chair', chair, [-0.045, 0, 0.08], 0.07);
roundedBox([0.18, 0.18, 0.62], [-0.42, 2.54, -0.015], materials.jacketLight, 'chair', chair, [0, 0, 0.06], 0.06);
roundedBox([1.02, 0.22, 0.82], [0.07, 1.32, 0.015], materials.chairFabric, 'chair', chair, [-0.025, 0, -0.025], 0.065);
roundedBox([0.84, 0.08, 0.64], [0.07, 1.43, 0.015], materials.jacketLight, 'chair', chair, [-0.025, 0, -0.025], 0.035);
for (const z of [-0.32, 0.34]) segment([-0.27, 1.19, z], [-0.27, 0.83, z], 0.035, materials.metal, 'chair', chair);
segment([0.1, 1.17, 0], [0.1, 0.36, 0], 0.085, materials.metalDark, 'chair', chair, 12);
for (let index = 0; index < 5; index += 1) {
  const angle = (index / 5) * Math.PI * 2;
  const x = 0.1 + Math.cos(angle) * 0.62;
  const z = Math.sin(angle) * 0.46;
  segment([0.1, 0.4, 0], [x, 0.15, z], 0.042, materials.metal, 'chair', chair);
  segment([x - 0.065, 0.105, z], [x + 0.065, 0.105, z], 0.074, materials.rubber, 'chair', chair, 10);
}
for (const side of [-1, 1]) {
  for (const x of [-0.12, 0.57]) segment([x, 1.48, side * 0.45], [x, 1.78, side * 0.45], 0.035, materials.metal, 'chair', chair);
  roundedBox([0.82, 0.085, 0.14], [0.22, 1.82, side * 0.45], materials.jacketLight, 'chair', chair, [0, 0, -0.025], 0.035);
}

// The seated operator is built from independent articulated pieces, not a cutout.
const operator = new THREE.Group();
operator.position.x = -1.88;
scene.add(operator);
const torsoRig = new THREE.Group();
torsoRig.position.y = 1.58;
operator.add(torsoRig);
const toRig = ([x, y, z]) => [x, y - 1.5, z];

const torsoProfile = new THREE.SplineCurve([
  new THREE.Vector2(0.08, -0.1),
  new THREE.Vector2(0.25, -0.07),
  new THREE.Vector2(0.31, 0.12),
  new THREE.Vector2(0.37, 0.46),
  new THREE.Vector2(0.42, 0.79),
  new THREE.Vector2(0.48, 1.01),
  new THREE.Vector2(0.4, 1.19),
  new THREE.Vector2(0.2, 1.25),
  new THREE.Vector2(0, 1.25),
]);
const jacketGeometry = new THREE.LatheGeometry(torsoProfile.getPoints(32), 28);
const jacketMesh = new THREE.Mesh(jacketGeometry, materials.jacket);
jacketMesh.position.set(-0.025, -0.1, 0);
jacketMesh.rotation.z = -0.11;
registerChild(jacketMesh, torsoRig, 'jacket');
// A compact hip volume makes the jacket, thighs, and chair seat meet as one seated pose.
sphere(0.29, [0.02, -0.055, 0.015], [1.15, 0.48, 1.34], materials.pants, 'pants', torsoRig, 18);
segment(toRig([0.31, 2.5, 0.02]), toRig([0.29, 1.6, 0.02]), 0.012, materials.metal, 'jacket', torsoRig, 6);
box([0.035, 0.2, 0.22], toRig([0.34, 2.17, -0.13]), materials.jacketLight, 'jacket', torsoRig, [0, 0, -0.12]);
box([0.02, 0.024, 0.17], toRig([0.36, 2.1, -0.13]), materials.metalDark, 'jacket', torsoRig);
segment(toRig([0.31, 2.38, -0.24]), toRig([0.33, 1.91, 0.22]), 0.024, materials.jacketLight, 'jacket', torsoRig, 8);

// Neck, hood, face, hair, brow, and a small illuminated headset form a readable profile.
segment(toRig([0.0, 2.48, 0.11]), toRig([0.03, 2.76, 0.12]), 0.13, materials.skin, 'head', torsoRig);
const headRig = new THREE.Group();
headRig.position.set(0.02, 1.47, 0.12);
headRig.rotation.y = 0.9;
headRig.scale.setScalar(0.86);
torsoRig.add(headRig);
// Softer facial forms and a restrained over-ear headset give the operator a human profile.
sphere(0.31, [-0.11, -0.09, -0.12], [1.04, 0.95, 0.86], materials.jacket, 'head', headRig, 22);
sphere(0.31, [0.015, 0.015, 0.005], [0.89, 1.08, 0.9], materials.skin, 'head', headRig, 28);
sphere(0.275, [-0.06, 0.225, -0.055], [1.05, 0.58, 0.98], materials.hair, 'head', headRig, 24);
// A swept fringe and sideburn replace the bead-like hair clumps from the first pass.
segment([-0.25, 0.25, 0.12], [-0.06, 0.29, 0.21], 0.07, materials.hair, 'head', headRig, 12);
segment([-0.08, 0.28, 0.21], [0.15, 0.18, 0.2], 0.058, materials.hair, 'head', headRig, 12);
sphere(0.14, [0.04, -0.16, 0.045], [0.92, 0.5, 0.86], materials.hair, 'head', headRig, 20);
// Eyes, brows and a forward bridge are small at scene scale, but establish a face rather than a visor.
const eyeWhite = new THREE.MeshStandardMaterial({ color: 0xd8d1c0, roughness: 0.42 });
const eyeDark = new THREE.MeshStandardMaterial({ color: 0x19252a, roughness: 0.38 });
for (const x of [-0.095, 0.095]) {
  sphere(0.022, [x, 0.065, 0.267], [1.05, 0.72, 0.55], eyeWhite, 'head', headRig, 12);
  sphere(0.011, [x + 0.006, 0.064, 0.282], [1, 1, 0.55], eyeDark, 'head', headRig, 10);
  segment([x - 0.055, 0.12, 0.261], [x + 0.045, 0.12, 0.262], 0.009, materials.hair, 'head', headRig, 8);
}
sphere(0.055, [0.0, -0.005, 0.283], [0.68, 0.7, 0.78], materials.skin, 'head', headRig, 14);
segment([0.03, -0.105, 0.27], [0.13, -0.11, 0.252], 0.01, materials.rubber, 'head', headRig, 8);
// Ear cups sit on the sides of the skull; the band follows a true arc over the crown.
sphere(0.082, [-0.27, 0.02, 0.015], [0.56, 1.32, 1.04], materials.metalDark, 'head', headRig, 18);
const headsetRing = new THREE.Mesh(new THREE.TorusGeometry(0.068, 0.009, 8, 20), materials.glow.clone());
headsetRing.position.set(-0.319, 0.02, 0.015);
headsetRing.rotation.y = Math.PI / 2;
freezeTransform(headsetRing);
headRig.add(headsetRing);
const headsetPath = new THREE.CatmullRomCurve3([
  new THREE.Vector3(-0.29, 0.02, -0.01), new THREE.Vector3(-0.25, 0.24, -0.01),
  new THREE.Vector3(-0.08, 0.34, -0.01), new THREE.Vector3(0.12, 0.29, -0.01),
  new THREE.Vector3(0.27, 0.08, -0.01),
]);
const headsetBand = new THREE.Mesh(new THREE.TubeGeometry(headsetPath, 28, 0.025, 8, false), materials.metalDark.clone());
freezeTransform(headsetBand);
headRig.add(headsetBand);

// Bent knees and boots stay separate so selecting each garment hits its actual mesh.
segment([-0.08, 1.45, -0.08], [0.75, 1.11, 0.18], 0.195, materials.pants, 'pants', operator, 12);
segment([0.75, 1.11, 0.18], [0.43, 0.42, 0.29], 0.16, materials.pants, 'pants', operator, 12);
segment([-0.28, 1.4, -0.31], [0.47, 1.0, -0.25], 0.18, materials.pants, 'pants', operator, 12);
segment([0.47, 1.0, -0.25], [0.72, 0.38, -0.22], 0.15, materials.pants, 'pants', operator, 12);
segment([0.39, 0.3, 0.29], [0.88, 0.23, 0.34], 0.108, materials.boot, 'boots', operator, 12);
segment([0.68, 0.28, -0.22], [1.08, 0.22, -0.19], 0.105, materials.boot, 'boots', operator, 12);
roundedBox([0.46, 0.07, 0.25], [0.67, 0.082, 0.35], materials.rubber, 'boots', operator, [0, 0, -0.04], 0.025);
roundedBox([0.44, 0.07, 0.245], [0.88, 0.082, -0.18], materials.rubber, 'boots', operator, [0, 0, -0.02], 0.025);
for (let index = 0; index < 3; index += 1) {
  segment([0.65 + index * 0.07, 0.34, 0.48], [0.7 + index * 0.06, 0.3, 0.49], 0.009, materials.metal, 'boots', operator, 6);
  segment([0.89 + index * 0.035, 0.32, -0.01], [0.94 + index * 0.035, 0.29, -0.005], 0.009, materials.metal, 'boots', operator, 6);
}

// Desk shell, machined supports, drawers, keyboard and small working details.
roundedBox([4.45, 0.19, 1.45], [0.87, 2.12, 0.39], materials.wood, 'desk', scene, null, 0.055);
roundedBox([4.35, 0.018, 1.34], [0.87, 2.218, 0.39], materials.woodEdge, 'desk', scene, null, 0.008);
for (const x of [-1.06, 2.78]) {
  roundedBox([0.11, 2.04, 0.12], [x, 1.02, 0.02], materials.metalDark, 'desk', scene, null, 0.025);
  roundedBox([0.62, 0.085, 0.79], [x, 1.04, 0.38], materials.metalDark, 'desk', scene, null, 0.025);
  roundedBox([0.82, 0.035, 0.11], [x, 0.065, 0.37], materials.metal, 'desk', scene, null, 0.012);
  for (let index = 0; index < 3; index += 1) box([0.13, 0.014, 0.014], [x, 1.13 + index * 0.17, 0.445], materials.woodEdge, 'desk');
}
for (let index = 0; index < 2; index += 1) {
  box([1.2, 0.045, 0.65], [-2.4 + index * 0.35, 0.8 + index * 0.12, -0.63], materials.metalDark);
}

// Monitor assembly: a textured, animated display in a real frame and stand.
roundedBox([2.05, 1.37, 0.13], [1.46, 3.16, 0.06], materials.metalDark, 'monitor', scene, null, 0.035);
roundedBox([1.95, 1.27, 0.025], [1.46, 3.16, 0.139], materials.metal, 'monitor', scene, null, 0.012);
const screenCanvas = document.createElement('canvas');
screenCanvas.width = 640;
screenCanvas.height = 390;
const screenContext = screenCanvas.getContext('2d', { alpha: false });
const screenTexture = new THREE.CanvasTexture(screenCanvas);
screenTexture.colorSpace = THREE.SRGBColorSpace;
screenTexture.generateMipmaps = false;
screenTexture.minFilter = THREE.LinearFilter;
screenTexture.magFilter = THREE.LinearFilter;
screenTexture.anisotropy = 1;
const screen = new THREE.Mesh(new THREE.PlaneGeometry(1.88, 1.19), new THREE.MeshBasicMaterial({ map: screenTexture, toneMapped: false }));
screen.position.set(1.46, 3.16, 0.156);
freezeTransform(screen);
scene.add(screen);
roundedBox([0.28, 0.045, 0.03], [1.46, 2.465, 0.08], materials.metal, 'monitor', scene, null, 0.01);
segment([1.46, 2.49, 0.04], [1.46, 2.27, 0.04], 0.058, materials.metalDark, 'monitor');
roundedBox([0.72, 0.07, 0.42], [1.46, 2.235, 0.13], materials.metalDark, 'monitor', scene, null, 0.025);
let lastMonitorDraw = -1000;
const monitorStates = [
  ['404', 'PATH NOT FOUND', 'ROUTE / REQUEST ABSENT'],
  ['RETRYING', 'DESTINATION UNREACHABLE', 'TRACE / NEXT HOP TIMEOUT'],
  ['ROUTE MISSING', 'NO RESPONSE FROM NODE', 'RECOVERY / RETURN TO CODARIS'],
];
function drawMonitor(time = 0) {
  if (!screenContext) return;
  const context = screenContext;
  const width = 900;
  const height = 550;
  context.setTransform(screenCanvas.width / width, 0, 0, screenCanvas.height / height, 0, 0);
  const stateIndex = Math.floor(time / 4300) % monitorStates.length;
  const [headline, subtitle, route] = monitorStates[stateIndex];
  const scan = (time % 5600) / 5600;
  context.fillStyle = '#06121c';
  context.fillRect(0, 0, width, height);
  const gradient = context.createLinearGradient(0, 0, width, height);
  gradient.addColorStop(0, '#0d2632');
  gradient.addColorStop(0.6, '#071923');
  gradient.addColorStop(1, '#051017');
  context.fillStyle = gradient;
  context.fillRect(0, 0, width, height);
  context.strokeStyle = 'rgba(57, 178, 189, .13)';
  context.lineWidth = 1;
  for (let x = 0; x < width; x += 36) { context.beginPath(); context.moveTo(x, 0); context.lineTo(x, height); context.stroke(); }
  for (let y = 0; y < height; y += 36) { context.beginPath(); context.moveTo(0, y); context.lineTo(width, y); context.stroke(); }
  context.fillStyle = '#07131c';
  context.fillRect(0, 0, 190, height);
  context.fillStyle = '#4ae0de';
  context.font = '700 24px monospace';
  context.fillText('C', 24, 44);
  context.fillStyle = '#e3f3f3';
  context.font = '700 18px monospace';
  context.fillText('CODARIS', 58, 42);
  context.fillStyle = '#6e939e';
  context.font = '12px monospace';
  ['SYSTEMS', 'NETWORK', 'ROUTES', 'SERVERS', 'ASSETS', 'DEPLOY'].forEach((label, index) => {
    const y = 104 + index * 36;
    context.fillStyle = '#83aab4';
    context.fillText(label, 22, y);
    context.fillStyle = index === 2 ? '#ffa879' : '#ef7d76';
    context.fillText(index === 2 ? '404' : 'ERR', 129, y);
  });
  context.strokeStyle = 'rgba(45, 151, 163, .5)';
  context.beginPath(); context.moveTo(204, 75); context.lineTo(204, 485); context.stroke();
  context.fillStyle = '#83aab4';
  context.font = '12px monospace';
  context.fillText('NODE 04  /  LOCATION UNAVAILABLE', 232, 47);
  context.fillStyle = 'rgba(42, 198, 203, .13)';
  context.beginPath(); context.arc(565, 257, 156, 0, Math.PI * 2); context.fill();
  context.strokeStyle = 'rgba(48, 196, 201, .34)';
  context.lineWidth = 1;
  for (let radius = 58; radius < 160; radius += 32) {
    context.beginPath(); context.arc(565, 257, radius, 0, Math.PI * 2); context.stroke();
  }
  context.strokeStyle = 'rgba(43, 210, 207, .28)';
  for (let x = 248; x < 850; x += 40) {
    context.beginPath(); context.moveTo(x, 90); context.lineTo(x - 15, 478); context.stroke();
  }
  context.textAlign = 'center';
  context.shadowColor = '#39efe0';
  context.shadowBlur = 24;
  context.fillStyle = '#63efdb';
  context.font = '800 118px monospace';
  context.fillText(headline, 565, 267, 570);
  context.shadowBlur = 0;
  context.fillStyle = '#c0e3e5';
  context.font = '700 25px monospace';
  context.fillText(subtitle, 565, 316, 580);
  context.fillStyle = '#62bdc0';
  context.font = '14px monospace';
  context.fillText(route, 565, 352, 580);
  context.textAlign = 'left';
  context.fillStyle = '#75a3ad';
  context.font = '11px monospace';
  context.fillText('00:12:44   CHECKING DESTINATION', 232, 414);
  context.fillStyle = '#df8775';
  context.fillText('00:12:45   HTTP 404 / ROUTE MISSING', 232, 435);
  context.fillStyle = '#72d6cd';
  context.fillText('00:12:46   RETRY QUEUED', 232, 456);
  const scanY = 80 + scan * 400;
  const scanGradient = context.createLinearGradient(0, scanY - 22, 0, scanY + 22);
  scanGradient.addColorStop(0, 'rgba(73, 249, 225, 0)');
  scanGradient.addColorStop(0.5, 'rgba(73, 249, 225, .18)');
  scanGradient.addColorStop(1, 'rgba(73, 249, 225, 0)');
  context.fillStyle = scanGradient;
  context.fillRect(205, scanY - 22, width - 205, 44);
  context.fillStyle = '#60e6db';
  context.fillRect(0, height - 5, width * (0.32 + 0.12 * Math.sin(time / 1300)), 5);
  screenTexture.needsUpdate = true;
}
drawMonitor();

// The operator reaches toward an illuminated keyboard, with separate sleeve and hand forms.
const armPoints = [
  [[0.18, 2.4, 0.36], [0.78, 2.02, 0.42], [1.47, 2.13, 0.59]],
  [[-0.04, 2.35, -0.12], [0.68, 2.06, -0.04], [1.38, 2.13, 0.05]],
];
for (const [index, [shoulder, elbow, wrist]] of armPoints.entries()) {
  sphere(0.16, toRig(shoulder), [1.02, 0.9, 1], materials.jacket, 'arms', torsoRig, 14);
  segment(toRig(shoulder), toRig(elbow), 0.145, materials.jacket, 'arms', torsoRig, 12);
  sphere(0.15, toRig(elbow), [1, 1, 1], materials.jacketLight, 'arms', torsoRig, 12);
  segment(toRig(elbow), toRig(wrist), 0.105, materials.jacketLight, 'arms', torsoRig, 12);
  segment(toRig([wrist[0] - 0.02, wrist[1], wrist[2]]), toRig([wrist[0] + 0.18, wrist[1] - 0.025, wrist[2] + 0.03]), 0.073, materials.skin, 'arms', torsoRig, 10);
  sphere(0.115, toRig([wrist[0] + 0.18, wrist[1] - 0.035, wrist[2] + 0.02]), [1.5, 0.42, 0.92], materials.skin, 'arms', torsoRig, 16);
  for (let finger = 0; finger < 3; finger += 1) {
    const z = wrist[2] - 0.055 + finger * 0.072;
    segment(toRig([wrist[0] + 0.2, wrist[1] - 0.04, z]), toRig([wrist[0] + 0.34, wrist[1] - 0.06, z + 0.012]), 0.021, materials.skin, 'arms', torsoRig, 10);
  }
  roundedBox([0.14, 0.075, 0.14], toRig([elbow[0] - 0.025, elbow[1] - 0.005, elbow[2]]), materials.metalDark, 'arms', torsoRig, [0, index * 0.08, -0.16], 0.022);
}

// Keyboard, keys, mouse, lamp, mug, plant, and field notes add scale to the set.
const keyboardOrigin = new THREE.Vector3(0.22, 2.275, 0.77);
const keyboardRotation = new THREE.Euler(-0.035, 0, -0.018);
const keyboardQuaternion = new THREE.Quaternion().setFromEuler(keyboardRotation);
roundedBox([1.22, 0.1, 0.44], keyboardOrigin.toArray(), materials.plastic, 'keyboard', scene, keyboardRotation, 0.032);
const keyboardInstances = new Map();
for (let row = 0; row < 4; row += 1) {
  for (let column = 0; column < 13; column += 1) {
    const width = row === 3 && column === 5 ? 0.25 : 0.061;
    const x = -0.3 + column * 0.082 + (row === 3 && column > 5 ? 0.19 : 0);
    const material = column === 12 && row === 0 ? materials.glow : materials.metal;
    const geometry = roundedBoxGeometry([width, 0.045, 0.058], 0.012);
    const batchKey = `${geometry.uuid}:${material.uuid}`;
    let batch = keyboardInstances.get(batchKey);
    if (!batch) {
      batch = { geometry, material, matrices: [] };
      keyboardInstances.set(batchKey, batch);
    }
    const localX = x - keyboardOrigin.x;
    const localZ = 0.62 + row * 0.076 - keyboardOrigin.z;
    const offset = new THREE.Vector3(localX, 0.073, localZ).applyQuaternion(keyboardQuaternion);
    const position = keyboardOrigin.clone().add(offset);
    batch.matrices.push(new THREE.Matrix4().compose(position, keyboardQuaternion, new THREE.Vector3(1, 1, 1)));
  }
}
for (const { geometry, material, matrices } of keyboardInstances.values()) {
  const keys = new THREE.InstancedMesh(geometry, materialForPart(material, 'keyboard'), matrices.length);
  const center = new THREE.Vector3();
  for (const matrix of matrices) {
    center.x += matrix.elements[12];
    center.y += matrix.elements[13];
    center.z += matrix.elements[14];
  }
  center.divideScalar(matrices.length);
  for (let index = 0; index < matrices.length; index += 1) {
    const matrix = matrices[index];
    matrix.elements[12] -= center.x;
    matrix.elements[13] -= center.y;
    matrix.elements[14] -= center.z;
    keys.setMatrixAt(index, matrix);
  }
  keys.position.copy(center);
  keys.instanceMatrix.needsUpdate = true;
  keys.computeBoundingBox();
  keys.userData.part = 'keyboard';
  keys.userData.initialScale = keys.scale.clone();
  keys.castShadow = false;
  keys.receiveShadow = false;
  freezeTransform(keys);
  const meshes = objectMeshes.get('keyboard') || [];
  meshes.push(keys);
  objectMeshes.set('keyboard', meshes);
  scene.add(keys);
}
keyboardInstances.clear();
roundedBox([0.32, 0.05, 0.49], [1.12, 2.252, 0.76], materials.rubber, 'mouse', scene, null, 0.025);
const mouseShellGeometry = new THREE.BufferGeometry();
const mouseStations = [-1, -0.82, -0.45, 0, 0.48, 0.82, 1];
const mouseWidths = [0.34, 0.72, 0.94, 1, 0.94, 0.72, 0.34];
const mouseHeights = [0.01, 0.035, 0.073, 0.094, 0.105, 0.078, 0.01];
const mouseAcross = [-1, -0.5, 0, 0.5, 1];
const mouseVertices = [];
const mouseIndices = [];
for (let station = 0; station < mouseStations.length; station += 1) {
  for (const across of mouseAcross) {
    const profile = Math.sqrt(Math.max(0, 1 - across * across));
    mouseVertices.push(
      across * 0.17 * mouseWidths[station],
      mouseHeights[station] * profile,
      mouseStations[station] * 0.255,
    );
  }
}
for (let station = 0; station < mouseStations.length - 1; station += 1) {
  for (let across = 0; across < mouseAcross.length - 1; across += 1) {
    const a = station * mouseAcross.length + across;
    const b = (station + 1) * mouseAcross.length + across;
    mouseIndices.push(a, b, b + 1, a, b + 1, a + 1);
  }
}
mouseShellGeometry.setAttribute('position', new THREE.Float32BufferAttribute(mouseVertices, 3));
mouseShellGeometry.setIndex(mouseIndices);
mouseShellGeometry.computeVertexNormals();
const mouseShell = new THREE.Mesh(mouseShellGeometry, materials.plastic);
mouseShell.position.set(1.12, 2.276, 0.76);
register(mouseShell, 'mouse');
roundedBox([0.09, 0.012, 0.13], [1.12, 2.382, 0.72], materials.rubber, 'mouse', scene, null, 0.008);
const scrollWheel = new THREE.Mesh(new THREE.CylinderGeometry(0.033, 0.033, 0.014, 12), materials.metal);
scrollWheel.position.set(1.12, 2.39, 0.72);
scrollWheel.rotation.z = Math.PI / 2;
register(scrollWheel, 'mouse');

roundedBox([0.34, 0.045, 0.44], [2.66, 2.25, 0.72], materials.plastic, 'phone', scene, [0, 0, -0.04], 0.022);
roundedBox([0.29, 0.012, 0.37], [2.66, 2.279, 0.72], materials.metalDark, 'phone', scene, [0, 0, -0.04], 0.014);
for (let index = 0; index < 3; index += 1) line([[2.55, 2.287, 0.61 + index * 0.08], [2.77, 2.287, 0.61 + index * 0.08]], 0x66818a, 0.48);
const mug = new THREE.Mesh(new THREE.CylinderGeometry(0.135, 0.12, 0.26, 18), new THREE.MeshStandardMaterial({ color: 0x18232a, roughness: 0.48, metalness: 0.2 }));
mug.position.set(2.55, 2.36, -0.06);
register(mug, 'mug');
const mugCoffee = new THREE.Mesh(new THREE.CircleGeometry(0.116, 18), new THREE.MeshStandardMaterial({ color: 0x21140e, roughness: 0.3 }));
mugCoffee.rotation.x = -Math.PI / 2;
mugCoffee.position.set(2.55, 2.492, -0.06);
register(mugCoffee, 'mug');
const mugRim = new THREE.Mesh(new THREE.TorusGeometry(0.128, 0.012, 6, 18), materials.metal);
mugRim.rotation.x = Math.PI / 2;
mugRim.position.set(2.55, 2.492, -0.06);
register(mugRim, 'mug');
const mugHandle = new THREE.Mesh(new THREE.TorusGeometry(0.087, 0.022, 7, 16), materials.metal);
mugHandle.position.set(2.68, 2.37, -0.06);
register(mugHandle, 'mug');

// A compact articulated task light casts a warm pool across the desktop.
segment([2.55, 2.25, -0.25], [2.55, 3.23, -0.25], 0.035, materials.metal, 'desk');
segment([2.55, 3.23, -0.25], [2.04, 3.72, -0.23], 0.035, materials.metal, 'desk');
const lampShade = new THREE.Mesh(new THREE.ConeGeometry(0.3, 0.34, 16, 1, true), new THREE.MeshStandardMaterial({ color: 0x26363b, roughness: 0.38, metalness: 0.6, side: THREE.DoubleSide }));
lampShade.position.set(1.99, 3.65, -0.19);
lampShade.rotation.z = -0.64;
register(lampShade, 'desk');
emissiveBar([0.21, 0.035, 0.16], [2.02, 3.49, -0.15], 0xffbd7b, 1.3);
const lampLight = new THREE.PointLight(0xffa362, 14, 4, 2);
lampLight.position.set(2.01, 3.48, 0.0);
scene.add(lampLight);

// A floor pot and a few broad leaves create an organic silhouette at the edge.
const pot = new THREE.Mesh(new THREE.CylinderGeometry(0.21, 0.245, 0.38, 16), materials.metalDark);
pot.position.set(4.12, 0.24, 0.16);
freezeTransform(pot);
scene.add(pot);
const potRim = new THREE.Mesh(new THREE.TorusGeometry(0.205, 0.018, 6, 18), materials.metal);
potRim.rotation.x = Math.PI / 2;
potRim.position.set(4.12, 0.435, 0.16);
freezeTransform(potRim);
scene.add(potRim);
const plantLeafMaterials = [
  new THREE.MeshStandardMaterial({ color: 0x235344, roughness: 0.96 }),
  new THREE.MeshStandardMaterial({ color: 0x193f34, roughness: 0.96 }),
];
for (let index = 0; index < 6; index += 1) {
  const angle = index * Math.PI * 0.333;
  const end = [4.12 + Math.cos(angle) * (0.22 + (index % 2) * 0.07), 0.72 + (index % 3) * 0.075, 0.16 + Math.sin(angle) * 0.23];
  segment([4.12, 0.42, 0.16], end, 0.018, materials.plantStem, '', scene, 7);
  const leaf = sphere(0.19, end, [0.5, 0.8, 0.28], plantLeafMaterials[index % 2], '', scene, 10);
  leaf.rotation.z = -Math.cos(angle) * 0.54;
  leaf.rotation.y = Math.sin(angle) * 0.5;
  leaf.updateMatrix();
}

// Sparse dust particles move in scene space; no screen overlay or DOM hotspot markers.
const dustGeometry = new THREE.BufferGeometry();
const dustPositions = new Float32Array(24 * 3);
for (let index = 0; index < 24; index += 1) {
  dustPositions[index * 3] = -4.4 + ((index * 43) % 88) / 10;
  dustPositions[index * 3 + 1] = 0.45 + ((index * 29) % 450) / 100;
  dustPositions[index * 3 + 2] = -1.5 + ((index * 17) % 32) / 10;
}
dustGeometry.setAttribute('position', new THREE.BufferAttribute(dustPositions, 3));
const dust = new THREE.Points(dustGeometry, new THREE.PointsMaterial({ color: 0x7ae8e1, size: 0.024, transparent: true, opacity: 0.42, sizeAttenuation: true }));
freezeTransform(dust);
scene.add(dust);

finalizeStaticBatches();
// One shared radial decal grounds static contacts without another light or AO pass.
const shadowCanvas = document.createElement('canvas');
shadowCanvas.width = 64;
shadowCanvas.height = 64;
const shadowContext = shadowCanvas.getContext('2d');
if (shadowContext) {
  const gradient = shadowContext.createRadialGradient(32, 32, 2, 32, 32, 31);
  gradient.addColorStop(0, 'rgba(0, 0, 0, 0.48)');
  gradient.addColorStop(0.36, 'rgba(0, 0, 0, 0.3)');
  gradient.addColorStop(1, 'rgba(0, 0, 0, 0)');
  shadowContext.fillStyle = gradient;
  shadowContext.fillRect(0, 0, 64, 64);
  const shadowTexture = new THREE.CanvasTexture(shadowCanvas);
  shadowTexture.generateMipmaps = false;
  shadowTexture.minFilter = THREE.LinearFilter;
  shadowTexture.magFilter = THREE.LinearFilter;
  const shadowMaterial = new THREE.MeshBasicMaterial({
    map: shadowTexture,
    transparent: true,
    opacity: 0.62,
    depthWrite: false,
    side: THREE.DoubleSide,
    toneMapped: false,
  });
  const shadowPlane = new THREE.PlaneGeometry(1, 1);
  const contactPoints = [
    [0.22, 2.231, 0.77, 1.34, 0.55],
    [1.12, 2.231, 0.76, 0.44, 0.6],
    [2.66, 2.231, 0.72, 0.42, 0.49],
    [2.55, 2.231, -0.06, 0.34, 0.34],
    [1.46, 2.231, 0.13, 0.82, 0.48],
    [-1.82, 1.475, -0.085, 0.7, 0.65],
    [-1.21, 0.058, 0.25, 0.62, 0.42],
    [-1.0, 0.058, -0.28, 0.58, 0.4],
    [-1.8, 0.058, -0.1, 1.35, 0.95],
    [-1.06, 0.058, 0.37, 0.56, 0.58],
    [2.78, 0.058, 0.37, 0.56, 0.58],
    [4.12, 0.058, 0.16, 0.56, 0.48],
  ];
  for (let index = 0; index < 5; index += 1) {
    const angle = (index / 5) * Math.PI * 2;
    contactPoints.push([-1.9 + 0.1 + Math.cos(angle) * 0.62, 0.058, -0.1 + Math.sin(angle) * 0.46, 0.28, 0.28]);
  }
  const shadows = new THREE.InstancedMesh(shadowPlane, shadowMaterial, contactPoints.length);
  const shadowRotation = new THREE.Quaternion().setFromEuler(new THREE.Euler(-Math.PI / 2, 0, 0));
  contactPoints.forEach(([x, y, z, width, depth], index) => {
    const matrix = new THREE.Matrix4().compose(
      new THREE.Vector3(x, y, z),
      shadowRotation,
      new THREE.Vector3(width, depth, 1),
    );
    shadows.setMatrixAt(index, matrix);
  });
  shadows.instanceMatrix.needsUpdate = true;
  shadows.frustumCulled = false;
  freezeTransform(shadows);
  scene.add(shadows);
}
scene.updateMatrixWorld(true);
const interactiveRoots = new Set();
for (const meshes of objectMeshes.values()) {
  for (const mesh of meshes) {
    if (mesh.parent === scene) interactiveRoots.add(mesh);
  }
}
const animatedRoots = new Set([operator, ...routeSignals.map(signal => signal.node), ...interactiveRoots]);
for (const object of scene.children) {
  if (animatedRoots.has(object)) continue;
  object.matrixAutoUpdate = false;
  object.matrixWorldAutoUpdate = false;
}
operator.matrixAutoUpdate = false;
for (const signal of routeSignals) signal.node.matrixWorldAutoUpdate = false;
const partPickBounds = [];
for (const [part, meshes] of objectMeshes) {
  for (const mesh of meshes) {
    const bounds = new THREE.Box3().setFromObject(mesh);
    if (!bounds.isEmpty()) partPickBounds.push({ part, bounds });
  }
}

const shadowCasters = new Set(['chair', 'head', 'jacket', 'arms', 'pants', 'boots', 'desk', 'monitor', 'pc']);
const shadowReceivers = new Set(['chair', 'desk', 'carpet', 'monitor', 'pc']);
for (const [part, meshes] of objectMeshes) {
  for (const mesh of meshes) {
    mesh.castShadow = shadowCasters.has(part);
    mesh.receiveShadow = shadowReceivers.has(part);
  }
}

const raycaster = new THREE.Raycaster();
raycaster.params.Line.threshold = 0.08;
const pointer = new THREE.Vector2(8, 8);
const pickPoint = new THREE.Vector3();
const candidateParts = new Set();
const pickCandidates = [];
const rayIntersections = [];
const keyboardPartIds = Object.keys(partNames);
let pointerActive = false;
let hoverPart = '';
let pinnedPart = '';
let keyboardPart = '';
let activePart = '';
let resizeObserver;
let pickRaf = 0;
let lastPickedX = Number.NEGATIVE_INFINITY;
let lastPickedY = Number.NEGATIVE_INFINITY;
let interactionActiveUntil = 0;

function getPartAt(clientX, clientY) {
  const bounds = canvas.getBoundingClientRect();
  if (!bounds.width || !bounds.height) return '';
  pointer.set(((clientX - bounds.left) / bounds.width) * 2 - 1, -((clientY - bounds.top) / bounds.height) * 2 + 1);
  raycaster.setFromCamera(pointer, camera);
  candidateParts.clear();
  for (const entry of partPickBounds) {
    if (raycaster.ray.intersectBox(entry.bounds, pickPoint)) candidateParts.add(entry.part);
  }
  if (!candidateParts.size) return '';
  pickCandidates.length = 0;
  for (const part of candidateParts) {
    const meshes = objectMeshes.get(part) || [];
    for (const mesh of meshes) pickCandidates.push(mesh);
  }
  rayIntersections.length = 0;
  raycaster.intersectObjects(pickCandidates, false, rayIntersections);
  return rayIntersections[0]?.object.userData.part || '';
}

function setActivePart(part) {
  if (activePart === part) return;
  const previousPart = activePart;
  const scalePart = (name, factor) => {
    for (const mesh of objectMeshes.get(name) || []) {
      const base = mesh.userData.initialScale;
      if (!base) continue;
      mesh.scale.copy(base).multiplyScalar(factor);
      mesh.updateMatrix();
    }
  };
  scalePart(previousPart, 1);
  for (const mesh of objectMeshes.get(activePart) || []) {
    const meshMaterials = Array.isArray(mesh.material) ? mesh.material : [mesh.material];
    for (const material of meshMaterials) {
      if (material.emissive && Object.hasOwn(material.userData, 'codarisBaseEmissive')) {
        material.emissive.setHex(material.userData.codarisBaseEmissive);
        material.emissiveIntensity = material.userData.codarisBaseEmissiveIntensity;
      }
    }
  }
  activePart = part;
  scalePart(activePart, 1.035);
  for (const mesh of objectMeshes.get(activePart) || []) {
    const meshMaterials = Array.isArray(mesh.material) ? mesh.material : [mesh.material];
    for (const material of meshMaterials) {
      if (material.emissive) {
        if (!Object.hasOwn(material.userData, 'codarisBaseEmissive')) {
          material.userData.codarisBaseEmissive = material.emissive.getHex();
          material.userData.codarisBaseEmissiveIntensity = material.emissiveIntensity;
        }
        material.emissive.setHex(0x0a5652);
        material.emissiveIntensity = 0.28;
      }
    }
  }
  host.classList.toggle('is-inspecting', Boolean(part));
  const detail = partNames[part];
  selectionLabel.textContent = detail ? `${detail[0]} // ${detail[1]}` : 'SCENE READY · SELECT A COMPONENT';
  drawOnce();
}

canvas.addEventListener('pointermove', event => {
  if (event.pointerType === 'mouse') {
    keyboardPart = '';
    pointerActive = true;
    if (Math.abs(event.clientX - lastPickedX) < 1.5 && Math.abs(event.clientY - lastPickedY) < 1.5) return;
    markSceneInteraction();
    pickClientX = event.clientX;
    pickClientY = event.clientY;
    if (!pickRaf) {
      pickRaf = requestAnimationFrame(() => {
        pickRaf = 0;
        hoverPart = getPartAt(pickClientX, pickClientY);
        lastPickedX = pickClientX;
        lastPickedY = pickClientY;
        const cursor = hoverPart ? 'crosshair' : 'default';
        if (canvas.style.cursor !== cursor) canvas.style.cursor = cursor;
        setActivePart(hoverPart || pinnedPart || keyboardPart);
      });
    }
  }
}, { passive: true });
canvas.addEventListener('pointerleave', () => {
  if (pickRaf) cancelAnimationFrame(pickRaf);
  pickRaf = 0;
  hoverPart = '';
  pointerActive = false;
  if (canvas.style.cursor !== 'default') canvas.style.cursor = 'default';
  markSceneInteraction();
  setActivePart(pinnedPart || keyboardPart);
});
canvas.addEventListener('click', event => {
  markSceneInteraction();
  const hit = getPartAt(event.clientX, event.clientY);
  if (event.pointerType === 'touch') {
    keyboardPart = '';
    pinnedPart = hit && pinnedPart !== hit ? hit : '';
    setActivePart(pinnedPart || keyboardPart);
    return;
  }
  keyboardPart = '';
  hoverPart = hit;
  if (hit) {
    pinnedPart = pinnedPart === hit ? '' : hit;
  } else {
    pinnedPart = '';
  }
  setActivePart(hoverPart || pinnedPart || keyboardPart);
});
host.addEventListener('keydown', event => {
  if (event.key !== 'ArrowRight' && event.key !== 'ArrowDown' && event.key !== 'ArrowLeft' && event.key !== 'ArrowUp') return;
  event.preventDefault();
  markSceneInteraction();
  let index = keyboardPartIds.indexOf(keyboardPart);
  const step = event.key === 'ArrowRight' || event.key === 'ArrowDown' ? 1 : -1;
  index = (index + step + keyboardPartIds.length) % keyboardPartIds.length;
  keyboardPart = keyboardPartIds[index];
  setActivePart(keyboardPart);
});

let cssWidth = 0;
let cssHeight = 0;
let rawPixelRatio = 0;
let pixelRatioCap = 1.25;
let pickClientX = 0;
let pickClientY = 0;
let animationFrame = 0;
let lastFrame = 0;
let lastAnimationTime = 0;
let animationElapsed = 0;
let lastAmbientUpdate = 0;
let frameTimeAverage = 1000 / 30;
let slowFrameCount = 0;
let qualityRecoveryFrames = 0;
let pageLeaving = false;
const idleFrameInterval = 1000 / 24;
const activeFrameInterval = 1000 / 30;
const ambientUpdateInterval = 125;
const monitorInterval = 250;
let staticShadowCaptured = false;

function canRender() {
  return inViewport && document.visibilityState === 'visible' && !pageLeaving && cssWidth > 0 && cssHeight > 0;
}

function setRendererPixelRatio() {
  renderer.setPixelRatio(Math.min(rawPixelRatio || 1, pixelRatioCap));
  renderer.setSize(cssWidth, cssHeight, false);
}

function resize() {
  const width = Math.max(1, host.clientWidth);
  const height = Math.max(1, host.clientHeight);
  const dpr = window.devicePixelRatio || 1;
  if (width === cssWidth && height === cssHeight && dpr === rawPixelRatio) return;
  cssWidth = width;
  cssHeight = height;
  rawPixelRatio = dpr;
  const aspect = width / height;
  camera.aspect = aspect;
  camera.updateProjectionMatrix();
  const pullback = Math.max(0, 1.46 - aspect) * 2.4;
  camera.position.set(cameraHome.x, cameraHome.y, cameraHome.z + pullback);
  camera.lookAt(cameraAim);
  setRendererPixelRatio();
  drawOnce();
}

function drawMonitorIfNeeded(time) {
  if (time - lastMonitorDraw < monitorInterval) return;
  lastMonitorDraw = time;
  drawMonitor(time);
}

function adjustQuality(frameGap) {
  frameTimeAverage += (frameGap - frameTimeAverage) * 0.08;
  if (frameTimeAverage > 42) {
    slowFrameCount += 1;
    qualityRecoveryFrames = 0;
    if (slowFrameCount >= 90 && pixelRatioCap > 0.9) {
      pixelRatioCap = Math.max(0.9, pixelRatioCap - 0.15);
      slowFrameCount = 0;
      setRendererPixelRatio();
    }
  } else {
    slowFrameCount = 0;
    if (frameTimeAverage < 35.5 && pixelRatioCap < 1.25) {
      qualityRecoveryFrames += 1;
      if (qualityRecoveryFrames >= 300) {
        pixelRatioCap = Math.min(1.25, pixelRatioCap + 0.05);
        qualityRecoveryFrames = 0;
        setRendererPixelRatio();
      }
    } else {
      qualityRecoveryFrames = 0;
    }
  }
}

function renderScene() {
  renderer.render(scene, camera);
  if (!staticShadowCaptured) {
    renderer.shadowMap.autoUpdate = false;
    keyLight.shadow.autoUpdate = false;
    staticShadowCaptured = true;
  }
}

function renderFrame(time = performance.now(), frameGap = 1000 / 24) {
  const t = time * 0.001;
  adjustQuality(frameGap);
  if (!reduceMotion.matches) {
    torsoRig.scale.y = 1 + Math.sin(t * 1.15) * 0.006;
    headRig.rotation.z = Math.sin(t * 0.48 + 0.6) * 0.012;
    if (time - lastAmbientUpdate >= ambientUpdateInterval) {
      lastAmbientUpdate = time;
      lampLight.intensity = 13.6 + Math.sin(t * 0.9) * 0.25;
      for (const signal of routeSignals) {
        const pulse = 0.5 + 0.5 * Math.sin(t * 1.25 + signal.phase);
        signal.node.scale.setScalar(0.9 + pulse * 0.2);
        signal.node.updateMatrix();
        signal.node.matrixWorld.copy(signal.node.matrix);
        signal.material.emissiveIntensity = 0.55 + pulse * 0.65;
      }
      const positions = dustGeometry.attributes.position;
      for (let index = 0; index < positions.count; index += 1) {
        const y = dustPositions[index * 3 + 1];
        positions.setY(index, 0.45 + ((y - 0.45 + t * (0.025 + (index % 3) * 0.009)) % 4.5));
      }
      positions.needsUpdate = true;
      drawMonitorIfNeeded(time);
    }
    if (!pointerActive) {
      camera.position.x += (cameraHome.x + Math.sin(t * 0.13) * 0.12 - camera.position.x) * 0.012;
      camera.position.y += (cameraHome.y + Math.sin(t * 0.19) * 0.05 - camera.position.y) * 0.012;
      camera.lookAt(cameraAim);
    } else {
      camera.position.x += (cameraHome.x + pointer.x * 0.2 - camera.position.x) * 0.035;
      camera.position.y += (cameraHome.y - pointer.y * 0.11 - camera.position.y) * 0.035;
      camera.lookAt(cameraAim);
    }
  }
  renderScene();
}

function drawOnce() {
  if (!renderer || !canRender()) return;
  renderScene();
}

let inViewport = false;
function frameIntervalAt(time) {
  return time < interactionActiveUntil ? activeFrameInterval : idleFrameInterval;
}

function requestNextFrame() {
  if (!animationFrame && canRender() && !reduceMotion.matches) animationFrame = requestAnimationFrame(animationTick);
}

function markSceneInteraction() {
  interactionActiveUntil = performance.now() + 900;
  if (!animationFrame && canRender() && !reduceMotion.matches) animationFrame = requestAnimationFrame(animationTick);
}

function animationTick(time) {
  animationFrame = 0;
  if (!canRender() || reduceMotion.matches) return;
  const frameInterval = frameIntervalAt(time);
  if (!lastAnimationTime) {
    lastAnimationTime = time;
    lastFrame = time;
    renderFrame(time, frameInterval);
  } else {
    animationElapsed += time - lastAnimationTime;
    lastAnimationTime = time;
    if (animationElapsed >= frameInterval) {
      const delta = lastFrame ? time - lastFrame : frameInterval;
      animationElapsed %= frameInterval;
      lastFrame = time;
      renderFrame(time, delta);
    }
  }
  requestNextFrame();
}

function stopAnimation() {
  if (animationFrame) cancelAnimationFrame(animationFrame);
  animationFrame = 0;
}

function syncAnimation() {
  if (!canRender()) {
    stopAnimation();
    return;
  }
  if (reduceMotion.matches) {
    stopAnimation();
    drawOnce();
  } else if (!animationFrame) {
    lastFrame = 0;
    lastAnimationTime = 0;
    animationElapsed = 0;
    animationFrame = requestAnimationFrame(animationTick);
  }
}

resizeObserver = new ResizeObserver(resize);
resizeObserver.observe(host);
const viewportObserver = new IntersectionObserver(entries => {
  inViewport = entries[0]?.isIntersecting === true;
  syncAnimation();
}, { rootMargin: '0px' });
viewportObserver.observe(host);
window.addEventListener('resize', resize, { passive: true });
window.addEventListener('orientationchange', resize, { passive: true });
document.addEventListener('visibilitychange', syncAnimation);
reduceMotion.addEventListener?.('change', syncAnimation);
canvas.addEventListener('webglcontextrestored', () => {
  staticShadowCaptured = false;
  renderer.shadowMap.autoUpdate = true;
  keyLight.shadow.autoUpdate = true;
  keyLight.shadow.needsUpdate = true;
  syncAnimation();
});
window.addEventListener('pagehide', () => {
  pageLeaving = true;
  stopAnimation();
  if (pickRaf) cancelAnimationFrame(pickRaf);
  pickRaf = 0;
});
window.addEventListener('pageshow', () => {
  pageLeaving = false;
  syncAnimation();
});
host.classList.add('has-scene');
selectionLabel.textContent = 'SCENE READY · SELECT A COMPONENT';
syncAnimation();
}

bootScene();
