#include "scene/mesh.hpp"

namespace _462 {

bool Mesh::subdivide() {
    // step 1: generate neighbor data
    generateEdgeNeighbor();

    // step 2: generate odd vertices
    generateOddVertices();

    // step 3: adjust even vertices
    adjustEvenVertices();

    // step 4: copy temp data
    triangles = temp_triangles;
    vertices = temp_vertices;

    // step 5: clean data
    temp_triangles.clear();
    temp_vertices.clear();
    vertexNeighbors.clear();
    faceNeighbors.clear();

    // step 6: creat gl data
    create_gl_data();
    return true;
}

void Mesh::generateEdgeNeighbor() {
    int numVertices = vertices.size();
    int numTriangles = triangles.size();
    VertMap vertMap;
    EdgeMap edgeMap;

    MeshVertexNeighbor neighbor_temp;
    MeshFaceNeighbor face_temp;

    for (int i=0; i<numVertices; i++) {
      vertexNeighbors.push_back(neighbor_temp);
    }

    // generate vertex map for vertex neighbors
    for (int i=0; i<numTriangles; i++) {
      vertMap.insert(VertMap::value_type(triangles[i].vertices[0], i));
      vertMap.insert(VertMap::value_type(triangles[i].vertices[1], i));
      vertMap.insert(VertMap::value_type(triangles[i].vertices[2], i));
    }

    // generate edge map for neighbors
    for (int i=0; i<numTriangles; i++) {
      std::string key0, key1, key2;
      key0 = genKeyforEdge(triangles[i].vertices[0], triangles[i].vertices[1]);
      edgeMap.insert(EdgeMap::value_type(key0, i));
      key1 = genKeyforEdge(triangles[i].vertices[1], triangles[i].vertices[2]);
      edgeMap.insert(EdgeMap::value_type(key1, i));
      key2 = genKeyforEdge(triangles[i].vertices[0], triangles[i].vertices[2]);
      edgeMap.insert(EdgeMap::value_type(key2, i));
    }

    // generate vertex neighbors
    for (int i=0; i<numVertices; i++) {
      std::pair <VertMapIterator, VertMapIterator> ret;
      ret = vertMap.equal_range(i);
      for (VertMapIterator it=ret.first; it != ret.second; ++it) {
        int u1, u2;
        getAdjacentVertices(i, u1, u2, triangles[it->second]);
        generateVertexNeighbor(i, u1, u2);
      }
    }

    // generate edge neighbors
    for (int i=0; i<numTriangles; i++) {
      for (int j=0; j<3; j++) {
        face_temp.vertices[j] = triangles[i].vertices[j];
      }

      faceNeighbors.push_back(face_temp);

      for (int j=0; j<3; j++){
        unsigned int u1 = faceNeighbors[i].vertices[index1[j]];
        unsigned int u2 = faceNeighbors[i].vertices[index2[j]];
        std::string key = genKeyforEdge(u1, u2);
        std::pair <EdgeMapIterator, EdgeMapIterator> ret;
        ret = edgeMap.equal_range(key);
        for (EdgeMapIterator it=ret.first; it != ret.second; ++it) {
          faceNeighbors[i].vertices_neighbor[index3[j]] = -1;
          int index = edgeIndexInTriangle(faceNeighbors[i].vertices[index1[j]],
                                          faceNeighbors[i].vertices[index2[j]],
                                          triangles[it->second]);
          if (index >= 0 && it->second != i) {
            faceNeighbors[i].vertices_neighbor[index3[j]] = index;
            break;
          }
        }
      }
    }
}

int Mesh::edgeIndexInTriangle(unsigned int u1, unsigned int u2, MeshTriangle t3) {
    for(int i=0; i<3; i++) {
      if (u1 == t3.vertices[i]) {
        if (u2 == t3.vertices[index2[i]])
          return t3.vertices[index3[i]];
        if (u2 == t3.vertices[index3[i]])
          return t3.vertices[index2[i]];
      }
    }
    return -1;
}

void Mesh::generateVertexNeighbor(unsigned int v, unsigned int u1, unsigned int u2) {
    bool flag1 = false;
    bool flag2 = false;
    for (size_t i = 0; i<vertexNeighbors[v].indices.size(); i++) {
        flag1 = flag1 || (vertexNeighbors[v].indices[i] == u1);
        flag2 = flag2 || (vertexNeighbors[v].indices[i] == u2);
    }
    if (!flag1) //if not found, then add it
    {
      vertexNeighbors[v].indices.push_back(u1);
    }
    if (!flag2) //if not found, then add it
    {
      vertexNeighbors[v].indices.push_back(u2);
    }
}

void Mesh::getAdjacentVertices(unsigned int v,int &u1,int &u2, MeshTriangle t3) {
    u1 = -1;
    u2 = -1;
    for(int i=0; i<3; i++) {
      if (v == t3.vertices[i]) {
        u1 = t3.vertices[index2[i]];
        u2 = t3.vertices[index3[i]];
      }
    }
}

void Mesh::generateOddVertices() {
    int numTriangles = triangles.size();
    MeshTriangle triangle_temp;
    MeshVertex odd_vertices_temp;
    MeshEdge edge_temp;
    EdgeList edges;
    EdgeSingleMap edgeMap;
    unsigned int new_vertex[3];
    temp_vertices = vertices;

    for (int i=0; i<numTriangles; i++) {
      for (int j=0; j<3; j++) {
        int new_index = isEdgeGenerateed(faceNeighbors[i].vertices[index1[j]],
                                  faceNeighbors[i].vertices[index2[j]],
                                  edges, &edgeMap);
        new_vertex[index3[j]] = new_index;
        if (new_index < 0) {
          if (faceNeighbors[i].vertices_neighbor[index3[j]] >= 0) {
            odd_vertices_temp.position =
              0.375*(vertices[faceNeighbors[i].vertices[index1[j]]].position
                    + vertices[faceNeighbors[i].vertices[index2[j]]].position)
              + 0.125*(vertices[faceNeighbors[i].vertices_neighbor[index3[j]]].position
                    + vertices[faceNeighbors[i].vertices[index3[j]]].position);
            odd_vertices_temp.normal =
              0.375*(vertices[faceNeighbors[i].vertices[index1[j]]].normal
                    + vertices[faceNeighbors[i].vertices[index2[j]]].normal)
              + 0.125*(vertices[faceNeighbors[i].vertices_neighbor[index3[j]]].normal
                    + vertices[faceNeighbors[i].vertices[index3[j]]].normal);
            odd_vertices_temp.tex_coord =
              0.375*(vertices[faceNeighbors[i].vertices[index1[j]]].tex_coord
                    + vertices[faceNeighbors[i].vertices[index2[j]]].tex_coord)
              + 0.125*(vertices[faceNeighbors[i].vertices_neighbor[index3[j]]].tex_coord
                    + vertices[faceNeighbors[i].vertices[index3[j]]].tex_coord);
          } else {
            odd_vertices_temp.position =
              0.5*(vertices[faceNeighbors[i].vertices[index1[j]]].position
                  + vertices[faceNeighbors[i].vertices[index2[j]]].position);
            odd_vertices_temp.normal =
              0.5*(vertices[faceNeighbors[i].vertices[index1[j]]].normal
                  + vertices[faceNeighbors[i].vertices[index2[j]]].normal);
            odd_vertices_temp.tex_coord =
              0.5*(vertices[faceNeighbors[i].vertices[index1[j]]].tex_coord
                  + vertices[faceNeighbors[i].vertices[index2[j]]].tex_coord);
          }

          new_vertex[index3[j]] = temp_vertices.size();
          temp_vertices.push_back(odd_vertices_temp);

          edge_temp.vertices[0] = faceNeighbors[i].vertices[index1[j]];
          edge_temp.vertices[1] = faceNeighbors[i].vertices[index2[j]];
          edge_temp.vertex_new = new_vertex[index3[j]];
          edges.push_back(edge_temp);
        }
      }
      triangle_temp.vertices[0] = faceNeighbors[i].vertices[0];
      triangle_temp.vertices[1] = new_vertex[2];
      triangle_temp.vertices[2] = new_vertex[1];
      temp_triangles.push_back(triangle_temp);

      triangle_temp.vertices[0] = faceNeighbors[i].vertices[1];
      triangle_temp.vertices[1] = new_vertex[0];
      triangle_temp.vertices[2] = new_vertex[2];
      temp_triangles.push_back(triangle_temp);

      triangle_temp.vertices[0] = faceNeighbors[i].vertices[2];
      triangle_temp.vertices[1] = new_vertex[1];
      triangle_temp.vertices[2] = new_vertex[0];
      temp_triangles.push_back(triangle_temp);

      triangle_temp.vertices[0] = new_vertex[0];
      triangle_temp.vertices[1] = new_vertex[1];
      triangle_temp.vertices[2] = new_vertex[2];
      temp_triangles.push_back(triangle_temp);
    }
}

int Mesh::isEdgeGenerateed(unsigned int u1, unsigned int u2,
                      EdgeList &e, EdgeSingleMap* edgeMap) {
  std::string key = genKeyforEdge(u1, u2);
  int count = edgeMap->count(key);
  if (count == 0) {
      edgeMap->insert(EdgeSingleMap::value_type(key, e.size()));
      return -1;
  }
  int index = edgeMap->find(key)->second;
  return e[index].vertex_new;
}

void Mesh::adjustEvenVertices() {
    MeshVertex even_vertices_temp;		//temp storage
    MeshVertexList even_vertices_temp_list;		//temp storage

    for (unsigned int i = 0; i < vertexNeighbors.size(); i++) {
      int N = vertexNeighbors[i].indices.size();
      if (N == 2) {
        even_vertices_temp.position =
            0.75*(vertices[i].position)
            + 0.125*(vertices[vertexNeighbors[i].indices[0]].position
            + vertices[vertexNeighbors[i].indices[1]].position);
        even_vertices_temp.normal =
            0.75*(vertices[i].normal)
            + 0.125*(vertices[vertexNeighbors[i].indices[0]].normal
            + vertices[vertexNeighbors[i].indices[1]].normal);
        even_vertices_temp.tex_coord =
            0.75*(vertices[i].tex_coord)
            + 0.125*(vertices[vertexNeighbors[i].indices[0]].tex_coord
            + vertices[vertexNeighbors[i].indices[1]].tex_coord);
      } else {
        double beta;
        double temp_beta = cos(2.0 * 3.1415926 / N);
        beta = (5.0/8.0-(3.0/8.0+0.25*temp_beta)*(3.0/8.0+0.25*temp_beta))/N;
        even_vertices_temp.position=(1-N*beta)*vertices[i].position;
        even_vertices_temp.normal=(1-N*beta)*vertices[i].normal;
        even_vertices_temp.tex_coord=(1-N*beta)*vertices[i].tex_coord;

        for (int j = 0; j < N; j++ ) {
          even_vertices_temp.position +=
              beta*vertices[vertexNeighbors[i].indices[j]].position;
          even_vertices_temp.normal +=
              beta*vertices[vertexNeighbors[i].indices[j]].normal;
          even_vertices_temp.tex_coord +=
              beta*vertices[vertexNeighbors[i].indices[j]].tex_coord;
        }
      }
      even_vertices_temp_list.push_back(even_vertices_temp);
    }

    //---------------------combine and generate new mesh-----------------------
    for (unsigned int i=0; i<triangles.size(); i++) {
      for (int j=0; j<3; j++) {
        temp_vertices[triangles[i].vertices[j]] =
            even_vertices_temp_list[triangles[i].vertices[j]];
      }
    }
}

std::string Mesh::genKeyforEdge(int u1, int u2) {
  if(u1 > u2) std::swap(u1, u2);
  std::string s1 = std::to_string(u1);
  std::string s2 = std::to_string(u2);
  return s1 + 'x' + s2;
}

} /* _462 */
