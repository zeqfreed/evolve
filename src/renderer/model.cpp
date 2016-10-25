
typedef struct Model {
  int vcount;
  int tcount;
  int ncount;
  int fcount;
  Vec3f vertices[2048];
  Vec3f normals[2048];
  Vec3f texture_coords[2048];
  int faces[4000][9];
} Model;

static void model_read(void *bytes, size_t size, Model *model)
{
  int vi = 0;
  int fi = 0;
  int ni = 0;
  int ti = 0;

  float minx = 0;
  float maxx = 0;
  float miny = 0;
  float maxy = 0;
  float minz = 0;
  float maxz = 0;  
  float accx = 0.0;
  float accy = 0.0;
  float accz = 0.0;

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

        model->vertices[vi++] = (Vec3f){x, y, z};

        accx += x;
        accy += y;
        accz += z;

        if (x < minx) {
          minx = x;
        } else if (x > maxx) {
          maxx = x;
        }

        if (y < miny) {
          miny = y;
        } else if (y > maxy) {
          maxy = y;
        }        

        if (z < minz) {
          minz = z;
        } else if (z > maxz) {
          maxz = z;
        }        
      } else if (*p == 'n') {
        p++;
        int consumed = 0;
        if (sscanf(p, "%f %f %f%n", &x, &y, &z, &consumed) != 3) {
          continue;
        }
        p += consumed;

        model->normals[ni++] = (Vec3f){-x, -y, -z};
      } else if (*p == 't') {
        p++;
        int consumed = 0;
        if (sscanf(p, "%f %f %f%n", &x, &y, &z, &consumed) < 2) {
          continue;
        }
        p += consumed;

        model->texture_coords[ti++] = (Vec3f){x, y, z};
      }
    } else if (*p == 'f') {
      p++;

      int vi0, ti0, ni0, vi1, ti1, ni1, vi2, ti2, ni2;
      int consumed = 0;
      if (sscanf(p, "%d/%d/%d %d/%d/%d %d/%d/%d%n", &vi0, &ti0, &ni0, &vi1, &ti1, &ni1, &vi2, &ti2, &ni2, &consumed) != 9) {
        continue;
      }
      p += consumed;

      int *face = model->faces[fi++];
      face[0] = vi0 - 1;
      face[1] = ti0 - 1;
      face[2] = ni0 - 1;

      face[3] = vi1 - 1;
      face[4] = ti1 - 1;
      face[5] = ni1 - 1;

      face[6] = vi2 - 1;
      face[7] = ti2 - 1;
      face[8] = ni2 - 1;
    }

    p++;
  }

  model->vcount = vi;
  model->fcount = fi;
  model->ncount = ni;
  model->tcount = ti;
  printf("read %d vertices, %d texture coords, %d normals, %d faces\n", model->vcount, model->tcount, model->ncount, model->fcount);

  printf("Model bbox x in (%.2f .. %.2f); y in (%.2f .. %.2f); z in (%.2f .. %.2f)\n",
         minx, maxx, miny, maxy, minz, maxz);

  float dx = maxx - minx;
  float dy = maxy - miny;
  float dz = maxz - minz;
  float scale = 1.0 / dx;
  if (dy > dx) {
    scale = 1.0 / dy;
  }
  if ((dz > dy) && (dz > dx)) {
    scale = 1.0 / dz;
  }

#if 0
  float ox = accx / vi;
  float oy = accy / vi;
  float oz = accz / vi;
  printf("Model center offset x: %.3f, y: %.3f, z: %.3f\n", ox, oy, oz);
#else
  float ox = 0.0;
  float oy = 0.0;
  float oz = 0.0;
#endif

  for (int i = 0; i < model->vcount; i++) {
    model->vertices[i].x = model->vertices[i].x * scale - ox * scale;
    model->vertices[i].y = model->vertices[i].y * scale - oy * scale;
    model->vertices[i].z = model->vertices[i].z * scale - oz * scale;
  }  
}
