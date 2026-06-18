import * as THREE from "three";
import { LineMaterial } from "three/addons/lines/LineMaterial.js";
import { LineSegments2 } from "three/addons/lines/LineSegments2.js";
import { LineSegmentsGeometry } from "three/addons/lines/LineSegmentsGeometry.js";
import { OrbitControls } from "three/addons/controls/OrbitControls.js";
import { ViewHelper } from "three/addons/helpers/ViewHelper.js";

type RenderObject = {
  group: THREE.Group;
  geometries: THREE.BufferGeometry[];
  materials: THREE.Material[];
};

function buildEdges(indices: Uint32Array): Uint32Array {
  if (indices.length < 4) {
    return new Uint32Array();
  }
  const base = indices.reduce((a, b) => Math.max(a, b), 0) + 1;
  const used = new Set<number>();
  const edges: number[] = [];
  for (let i = 0; i < indices.length; i += 4) {
    for (let s = 0; s < 4; s++) {
      for (let t = s + 1; t < 4; t++) {
        const i0 = Math.min(indices[i + s], indices[i + t]);
        const i1 = Math.max(indices[i + s], indices[i + t]);
        if (used.has(i0 * base + i1)) continue;
        edges.push(i0, i1);
        used.add(i0 * base + i1);
      }
    }
  }
  return new Uint32Array(edges);
}

function buildEdgePositions(
  edges: Uint32Array,
  positions: Float32Array,
): Float32Array {
  const edgePositions = new Float32Array(edges.length * 3);
  edges.forEach((vid, i) => {
    edgePositions.set(positions.subarray(3 * vid, 3 * vid + 3), 3 * i);
  });

  return edgePositions;
}

export class Viewer {
  private readonly container: HTMLElement;
  private readonly scene: THREE.Scene;
  private readonly camera: THREE.PerspectiveCamera;
  private readonly initialCameraPosition = new THREE.Vector3(0.0, 4.0, 4.0);

  private readonly renderer: THREE.WebGLRenderer;
  private readonly controls: OrbitControls;
  private readonly viewHelper: ViewHelper;

  private mesh: RenderObject | null = null;

  constructor(container: HTMLElement) {
    this.container = container;

    this.scene = new THREE.Scene();
    this.scene.background = new THREE.Color().setRGB(0.0, 0.0, 0.0);

    // Camera

    this.camera = new THREE.PerspectiveCamera(
      45,
      this.container.clientWidth / this.container.clientHeight,
      0.01,
      1000,
    );
    this.camera.position.copy(this.initialCameraPosition);
    this.scene.add(this.camera);

    // Renderer

    this.renderer = new THREE.WebGLRenderer({ antialias: true });
    this.renderer.autoClear = false;
    this.renderer.setPixelRatio(window.devicePixelRatio);
    this.renderer.setSize(
      this.container.clientWidth,
      this.container.clientHeight,
    );
    this.container.appendChild(this.renderer.domElement);

    // Controls

    this.controls = new OrbitControls(this.camera, this.renderer.domElement);
    this.controls.zoomSpeed = 2.0;
    this.controls.panSpeed = 1.0;
    this.controls.enableDamping = true;
    this.controls.maxDistance = 50.0;

    // View Helper

    this.viewHelper = new ViewHelper(this.camera, this.renderer.domElement);
    this.viewHelper.location = {
      bottom: 20,
      right: 20,
      top: null,
      left: null,
    };
    this.viewHelper.setLabels("X", "Y", "Z");
    this.renderer.domElement.addEventListener("pointerup", this.onPointerUp);

    // Setup

    this.addLights();

    window.addEventListener("resize", () => this.resize());
  }

  update() {
    this.controls.update();
  }

  render() {
    this.renderer.render(this.scene, this.camera);
    this.viewHelper.render(this.renderer);
  }

  setMesh(vertices: Float64Array, indices: Uint32Array): void {
    this.clearMesh();

    const group = new THREE.Group();
    const geometries: THREE.BufferGeometry[] = [];
    const materials: THREE.Material[] = [];

    const positions = Float32Array.from(vertices);

    // Points
    {
      const geometry = new THREE.SphereGeometry(0.025);

      const material = new THREE.MeshBasicMaterial({
        color: 0xaaaaff,
      });

      const vertexCount = vertices.length / 3;

      const points = new THREE.InstancedMesh(geometry, material, vertexCount);

      const matrix = new THREE.Matrix4();
      for (let i = 0; i < vertexCount; i++) {
        matrix.makeTranslation(
          positions[3 * i + 0],
          positions[3 * i + 1],
          positions[3 * i + 2],
        );
        points.setMatrixAt(i, matrix);
      }

      points.instanceMatrix.needsUpdate = true;

      group.add(points);

      geometries.push(geometry);
      materials.push(material);
    }

    // Edges
    {
      const edges = buildEdges(indices);
      const edgePositions = buildEdgePositions(edges, positions);
      const geometry = new LineSegmentsGeometry();
      geometry.setPositions(edgePositions);

      const material = new LineMaterial({
        color: 0x5555ff,
        transparent: true,
        opacity: 0.8,
        linewidth: 0.01,
        worldUnits: true,
      });
      material.resolution.set(
        this.renderer.domElement.clientWidth,
        this.renderer.domElement.clientHeight,
      );

      const lines = new LineSegments2(geometry, material);
      lines.computeLineDistances();

      group.add(lines);

      geometries.push(geometry);
      materials.push(material);
    }

    this.scene.add(group);
    this.mesh = { group, geometries, materials };

    const centroid = this.computeCentroid(positions);
    this.camera.lookAt(centroid);
    this.controls.target.copy(centroid);
  }

  private clearMesh(): void {
    if (this.mesh === null) return;

    this.scene.remove(this.mesh.group);

    for (const geometry of this.mesh.geometries) {
      geometry.dispose();
    }

    for (const material of this.mesh.materials) {
      material.dispose();
    }

    this.mesh = null;
  }

  private computeCentroid(positions: Float32Array): THREE.Vector3 {
    if (positions.length == 0) {
      return new THREE.Vector3();
    }
    const centroid = new THREE.Vector3();
    const vertexCount = positions.length / 3;
    for (let i = 0; i < vertexCount; i++) {
      centroid.x += positions[3 * i + 0];
      centroid.y += positions[3 * i + 1];
      centroid.z += positions[3 * i + 2];
    }
    centroid.divideScalar(vertexCount);
    return centroid;
  }

  private addLights() {
    const ambient = new THREE.AmbientLight(0xffffff, 0.6);
    this.scene.add(ambient);

    const directional = new THREE.DirectionalLight(0xffffff);
    directional.position.set(2.0, 3.0, 4.0);
    this.scene.add(directional);
  }

  private onPointerUp = (event: PointerEvent) => {
    this.viewHelper.handleClick(event);
  };

  private resize(): void {
    const width = Math.max(1, this.container.clientWidth);
    const height = Math.max(1, this.container.clientHeight);

    this.camera.aspect = width / height;
    this.camera.updateProjectionMatrix();

    this.renderer.setSize(width, height);
  }
}
