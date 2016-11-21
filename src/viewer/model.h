#pragma once

typedef struct ModelFace {
  int vi[3];
  int ti[3];
  int ni[3];
} ModelFace;

typedef struct Model {
  int vcount;
  int tcount;
  int ncount;
  int fcount;
  Vec3f min;
  Vec3f max;
  Vec3f center;
  Vec3f *vertices;
  Vec3f *normals;
  Vec3f *texture_coords;
  ModelFace *faces;

  void parse(void *bytes, size_t size, bool dry_run);
  void normalize(bool move_to_center);
} Model;
