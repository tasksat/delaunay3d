import { Viewer } from "./viewer";

type DelaunayModule = {
  HEAPF64: Float64Array;
  HEAPU32: Uint32Array;

  _malloc(size: number): number;
  _free(ptr: number): void;

  _dlny_build(flatPointsPtr: number, pointCount: number): number;
  _dlny_vertex_count(): number;
  _dlny_tet_count(): number;
  _dlny_get_vertices(): number;
  _dlny_get_indices(): number;
};

type DelaunayMesh = {
  vertices: Float64Array;
  tetIndices: Uint32Array;
};

async function loadDelaunayModule(): Promise<DelaunayModule> {
  // @ts-ignore
  const { default: createModule } = await import("./generated/delaunay3d.js");

  const module = await createModule({
    locateFile: (path: string) => {
      if (path.endsWith(".wasm")) {
        return "/delaunay3d.wasm";
      }
      return path;
    },
  });

  return module as DelaunayModule;
}

function runDelaunay(
  module: DelaunayModule,
  points: Float64Array,
): DelaunayMesh {
  if (points.length % 3 !== 0) {
    throw new Error("points.length must be divisible by 3.");
  }

  const pointCount = points.length / 3;
  const bytes = points.byteLength;

  const ptr = module._malloc(bytes);
  if (ptr === 0) {
    throw new Error("malloc failed");
  }

  try {
    module.HEAPF64.set(points, ptr / Float64Array.BYTES_PER_ELEMENT);

    const ok = module._dlny_build(ptr, pointCount);
    if (ok === 0) {
      throw new Error("dlny_build failed");
    }
  } finally {
    module._free(ptr);
  }

  const vertexCount = module._dlny_vertex_count();
  const tetCount = module._dlny_tet_count();

  const verticesPtr = module._dlny_get_vertices();
  const indicesPtr = module._dlny_get_indices();

  const verticesBegin = verticesPtr / Float64Array.BYTES_PER_ELEMENT;
  const indicesBegin = indicesPtr / Uint32Array.BYTES_PER_ELEMENT;

  const vertices = module.HEAPF64.slice(
    verticesBegin,
    verticesBegin + 3 * vertexCount,
  );
  const tetIndices = module.HEAPU32.slice(
    indicesBegin,
    indicesBegin + 4 * tetCount,
  );

  return { vertices, tetIndices };
}

function generateRandomPoints(count: number): Float64Array {
  const points = new Float64Array(3 * count);

  for (let i = 0; i < count; i++) {
    points[3 * i + 0] = Math.random() * 2.0 - 1.0;
    points[3 * i + 1] = Math.random() * 2.0 - 1.0;
    points[3 * i + 2] = Math.random() * 2.0 - 1.0;
  }

  return points;
}

async function main(): Promise<void> {
  const app = document.querySelector<HTMLDivElement>("#app");
  if (app === null) {
    throw new Error("#app not found");
  }

  const viewer = new Viewer(app);

  const animate = (): void => {
    requestAnimationFrame(animate);
    viewer.update();
    viewer.render();
  };

  animate();

  const panel = document.createElement("div");
  panel.style.position = "absolute";
  panel.style.left = "12px";
  panel.style.top = "12px";
  panel.style.padding = "8px 10px";
  panel.style.color = "white";
  panel.style.fontFamily = "monospace";
  panel.style.fontSize = "12px";
  panel.style.borderRadius = "6px";
  panel.style.zIndex = "10";

  const stats = document.createElement("div");

  const button = document.createElement("button");
  button.textContent = "regenerate";
  button.style.fontFamily = "monospace";
  button.style.marginTop = "8px";

  panel.appendChild(stats);
  panel.appendChild(button);
  app.appendChild(panel);

  const module = await loadDelaunayModule();

  const rebuild = (points: Float64Array): void => {
    const result = runDelaunay(module, points);
    viewer.setMesh(result.vertices, result.tetIndices);

    console.log("input point count", points.length / 3);
    console.log("vertices", result.vertices);
    console.log("tetIndices", result.tetIndices);
    console.log("vertex count", result.vertices.length / 3);
    console.log("tet count", result.tetIndices.length / 4);

    stats.textContent =
      `${points.length / 3} input points, ` +
      `${result.vertices.length / 3} vertices, ` +
      `${result.tetIndices.length / 4} tetrahedra`;
  };

  rebuild(generateRandomPoints(64));

  button.addEventListener("click", () => {
    rebuild(generateRandomPoints(64));
  });
}

main().catch((error) => {
  console.error(error);
});
