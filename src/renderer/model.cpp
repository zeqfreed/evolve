
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

void Model::parse(void *bytes, size_t size, bool dry_run = false)
{
  int vi = 0;
  int fi = 0;
  int ni = 0;
  int ti = 0;

  Vec3f acc = {0, 0, 0};
  min = (Vec3f){0, 0, 0};
  max = (Vec3f){0, 0, 0};

  char *p = (char *) bytes;
  while(p < (char *) bytes + size) {

    float x = 0;
    float y = 0;
    float z = 0;

    if (*p == 'v') {
      p++;
      if (*p == ' ') {
        int consumed = 0;
        if (sscanf(p, "%f %f %f%n", &x, &y, &z, &consumed) != 3) {
          continue;
        }
        p += consumed;

        Vec3f vertex = (Vec3f){x, y, z};
        if (dry_run) {
          vi++;
        } else {
          vertices[vi++] = vertex;
        }

        acc = acc + vertex;
        min.x = MIN(min.x, x);
        min.y = MIN(min.y, y);
        min.z = MIN(min.z, z);
        max.x = MAX(max.x, x);
        max.y = MAX(max.y, y);
        max.z = MAX(max.z, z);

      } else if (*p == 'n') {
        p++;
        int consumed = 0;
        if (sscanf(p, "%f %f %f%n", &x, &y, &z, &consumed) != 3) {
          continue;
        }
        p += consumed;

        if (dry_run) {
          ni++;
        } else {
          normals[ni++] = (Vec3f){-x, -y, -z};
        }
      } else if (*p == 't') {
        p++;
        int consumed = 0;
        if (sscanf(p, "%f %f %f%n", &x, &y, &z, &consumed) < 2) {
          continue;
        }
        p += consumed;
        
        if (dry_run) {
          ti++;
        } else {
          texture_coords[ti++] = (Vec3f){x, y, z};
        }
      }
    } else if (*p == 'f') {
      p++;

      int vi0, ti0, ni0, vi1, ti1, ni1, vi2, ti2, ni2;
      ModelFace face;
      int consumed = 0;
      if (sscanf(p, "%d/%d/%d %d/%d/%d %d/%d/%d%n",
                 &vi0, &ti0, &ni0, &vi1, &ti1, &ni1, &vi2, &ti2, &ni2, &consumed) != 9) {
        continue;
      }
      p += consumed;

      if (dry_run) {
        fi++;
      } else {
        face.vi[0] = vi0 - 1;
        face.ti[0] = ti0 - 1;
        face.ni[0] = ni0 - 1;
 
        face.vi[1] = vi1 - 1;
        face.ti[1] = ti1 - 1;
        face.ni[1] = ni1 - 1;

        face.vi[2] = vi2 - 1;
        face.ti[2] = ti2 - 1;
        face.ni[2] = ni2 - 1;

        faces[fi++] = face;
      }
    }

    p++;
  }

  vcount = vi;
  fcount = fi;
  ncount = ni;
  tcount = ti;

  center = acc * (1.0 / vcount);
}

void Model::normalize(bool move_to_center = false)
{
  Vec3f d = max - min;
  float scale = 1.0 / MAX(MAX(d.x, d.y), d.z);

  Vec3f offset = {0, 0, 0};
  if (move_to_center) {
    offset = -center;
  }

  for (int i = 0; i < vcount; i++) {
    vertices[i] = vertices[i] * scale - offset * scale;
  }  
}
