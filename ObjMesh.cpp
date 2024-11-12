#include "common.h"


bool Mesh::loadOBJ(const char * path){
  std::vector< unsigned int > vertexIndices, uvIndices, normalIndices;
  std::vector< vec3 > temp_vertices;
  std::vector< vec2 > temp_uvs;
  std::vector< vec3 > temp_normals;
  
  hasUV = true;
  
#ifdef _WIN32
  std::wstring wcfn;
  if (u8names_towc(path, wcfn) != 0)
    return NULL;
  FILE * file = _wfopen(wcfn.c_str(), L"r");
#else
  FILE * file = fopen(path, "r");
#endif //_WIN32

  if( file == NULL ){
    printf("Impossible to open the file !\n");
    return false;
  }
  
  char *line = new char[128];
  char *lineHeader = new char[128];
  
  while(true){
    memset(line, 0 , 128);
    memset(lineHeader, 0 , 128);
    
    if(fgets(line, 128, file) == NULL){ break; }
    sscanf(line, "%s ", lineHeader);
    
    if ( strcmp( lineHeader, "v" ) == 0 ){
      vec3 vertex;
      sscanf(&line[2], "%f %f %f", &vertex.x, &vertex.y, &vertex.z );
      temp_vertices.push_back(vertex);
      if(vertex.x < box_min.x){box_min.x = vertex.x; }
      if(vertex.y < box_min.y){box_min.y = vertex.y; }
      if(vertex.z < box_min.z){box_min.z = vertex.z; }
      if(vertex.x > box_max.x){box_max.x = vertex.x; }
      if(vertex.y > box_max.y){box_max.y = vertex.y; }
      if(vertex.z > box_max.z){box_max.z = vertex.z; }
    }else if ( strcmp( lineHeader, "vt" ) == 0 ){
      vec2 uv;
      sscanf(&line[3], "%f %f", &uv.x, &uv.y );
      temp_uvs.push_back(uv);
    }else if ( strcmp( lineHeader, "vn" ) == 0 ){
      vec3 normal;
      sscanf(&line[3], "%f %f %f", &normal.x, &normal.y, &normal.z );
      temp_normals.push_back(normal);
    }else if ( strcmp( lineHeader, "f" ) == 0 ){
      std::string vertex1, vertex2, vertex3;
      int vertexIndex[3], uvIndex[3], normalIndex[3];
      int matches = sscanf(&line[2], "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0],
                           &vertexIndex[1], &uvIndex[1], &normalIndex[1],
                           &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
      if (matches != 9){
        int matches = sscanf(&line[2], "%d//%d %d//%d %d//%d\n", &vertexIndex[0], &normalIndex[0],
                             &vertexIndex[1], &normalIndex[1],
                             &vertexIndex[2], &normalIndex[2]);
        if (matches == 6){
          hasUV = false;
        } else {
          printf("File can't be read by our simple parser : ( Try exporting with other options\n");
          return false;
        }
      }
      
      /* handle negative indices */
      /* (adjust for size during processing of each face, as per the old
       *  OBJ specification, instead of after the end of the file) */
      for (int negati = 0; negati < 3; negati++){
        if (vertexIndex[negati] < 0){
          vertexIndex[negati]+=temp_vertices.size();
          vertexIndex[negati]++; /* <- OBJ indices are one-based */
        }
        if (uvIndex[negati] < 0){
          uvIndex[negati]+=temp_uvs.size();
          uvIndex[negati]++;
        }
        if (normalIndex[negati] < 0){
          normalIndex[negati]+=temp_normals.size();
          normalIndex[negati]++;
        }
      }
      
      vertexIndices.push_back(vertexIndex[0]);
      vertexIndices.push_back(vertexIndex[1]);
      vertexIndices.push_back(vertexIndex[2]);
      if(hasUV){
        uvIndices    .push_back(uvIndex[0]);
        uvIndices    .push_back(uvIndex[1]);
        uvIndices    .push_back(uvIndex[2]);
      }
      normalIndices.push_back(normalIndex[0]);
      normalIndices.push_back(normalIndex[1]);
      normalIndices.push_back(normalIndex[2]);
    }
  }
  
  delete[] line;
  delete[] lineHeader;
  
  // For each vertex of each triangle
  for( unsigned int i=0; i<vertexIndices.size(); i++ ){
    unsigned int vertexIndex = vertexIndices[i];
    vec4 vertex = vec4(temp_vertices[ vertexIndex-1 ], 1.0);
    vertices.push_back(vertex);
  }
  
  if(hasUV){
    for( unsigned int i=0; i<uvIndices.size(); i++ ){
      unsigned int uvIndex = uvIndices[i];
      vec2 uv = temp_uvs[ uvIndex-1 ];
      uvs.push_back(uv);
    }
  }
  
  for( unsigned int i=0; i<normalIndices.size(); i++ ){
    unsigned int normalIndex = normalIndices[i];
    vec3 normal = temp_normals[ normalIndex-1 ];
    normals.push_back(normal);
  }
    
  center = box_min+(box_max-box_min)/2.0;
  scale = (std::max)(box_max.x - box_min.x, box_max.y-box_min.y);
  
  model_view = Scale(1.0/scale,           //Make the extents 0-1
                     1.0/scale,
                     1.0/scale)*
  Translate(-center);  //Orient Model About Center
  
  
  return true;
}

bool Mesh::makeSphere(int steps) {
    if (steps < 2) return false;

    // Step sizes for theta (longitude) and phi (latitude)
    double step_theta = (2 * M_PI) / steps;
    double step_phi = M_PI / steps;

    // Clear previous data
    vertices.clear();
    normals.clear();
    uvs.clear();

    // Generate vertices, normals, and UVs
    std::vector<vec3> pstrip0;
    std::vector<vec3> pstrip1;

    for (int i = 0; i <= steps; ++i) {
        double phi = i * step_phi;
        double y = cos(phi);
        double sin_phi = sin(phi);

        for (int j = 0; j <= steps; ++j) {
            double theta = j * step_theta;
            double x = cos(theta) * sin_phi;
            double z = sin(theta) * sin_phi;

            vec3 position(x, y, z);
            pstrip1.push_back(position);

            // Directly push position to vertices
            vertices.push_back(position);

            // Normal (same as position for a unit sphere)
            normals.push_back(normalize(position));

            // Texture coordinates (u, v) in the range [0, 1]
            float u = (float)j / steps;  // Longitude (theta)
            float v = (float)i / steps;  // Latitude (phi)
            uvs.push_back(vec2(u, v));
        }

        if (i > 0) {
            for (unsigned int k = 0; (k + 1) < pstrip0.size(); k++) {
                // Triangle 1
                vertices.push_back(pstrip0[k]);
                normals.push_back(normalize(pstrip0[k]));
                uvs.push_back(vec2((float)k / steps, (float)(i - 1) / steps));

                vertices.push_back(pstrip1[k]);
                normals.push_back(normalize(pstrip1[k]));
                uvs.push_back(vec2((float)k / steps, (float)i / steps));

                vertices.push_back(pstrip0[k + 1]);
                normals.push_back(normalize(pstrip0[k + 1]));
                uvs.push_back(vec2((float)(k + 1) / steps, (float)(i - 1) / steps));

                // Triangle 2
                vertices.push_back(pstrip0[k + 1]);
                normals.push_back(normalize(pstrip0[k + 1]));
                uvs.push_back(vec2((float)(k + 1) / steps, (float)(i - 1) / steps));

                vertices.push_back(pstrip1[k]);
                normals.push_back(normalize(pstrip1[k]));
                uvs.push_back(vec2((float)k / steps, (float)i / steps));

                vertices.push_back(pstrip1[k + 1]);
                normals.push_back(normalize(pstrip1[k + 1]));
                uvs.push_back(vec2((float)(k + 1) / steps, (float)i / steps));
            }
        }

        // Swap current strip with the previous strip
        pstrip1.swap(pstrip0);
        pstrip1.clear();
    }

    return true;
}