#include "scene/mesh.hpp"
#include <ctime>

namespace _462 {

bool Mesh::subdivide() {
    generateEdgeNeighbor();

    //---------------------step 1: generate odd vertices------------------------
    std::clock_t start = std::clock();
    generateOddVertices();
    double duration = (std::clock() - start)/double(CLOCKS_PER_SEC);
    std::cout << "generate odd vert took " << duration << " seconds." << std::endl;

    //---------------------step 2: adjust even vertices------------------------
    adjustEvenVertices();


    //---------------------step 3: clean and creat gl data --------------------
    triangles = temp_triangles;
    vertices = temp_vertices;
    temp_triangles.clear();
    temp_vertices.clear();
    neighbors.clear();
    faces.clear();

    create_gl_data();

    return true;
}

void Mesh::generateEdgeNeighbor() {
    int numVertices = vertices.size();
    int numTriangles = triangles.size();
    VertMap vertMap;
    EdgeMap edgeMap;

    MeshNeighbor neighbor_temp;
    MeshFace face_temp;

    for (int i=0; i<numVertices; i++) {
      neighbors.push_back(neighbor_temp);
    }

    // generate vertex map for neighbors
    for (int i=0; i<numTriangles; i++) {
      vertMap.insert(VertMap::value_type(triangles[i].vertices[0], i));
      vertMap.insert(VertMap::value_type(triangles[i].vertices[1], i));
      vertMap.insert(VertMap::value_type(triangles[i].vertices[2], i));
    }

    // generate edge map for neighbors
    for (int i=0; i<numTriangles; i++) {
      std::string key0, key1, key2;
      key0 = genKeyforEdge(triangles[i].vertices[0], triangles[i].vertices[1]);
      key1 = genKeyforEdge(triangles[i].vertices[1], triangles[i].vertices[2]);
      key2 = genKeyforEdge(triangles[i].vertices[0], triangles[i].vertices[2]);
      edgeMap.insert(EdgeMap::value_type(key0, i));
      edgeMap.insert(EdgeMap::value_type(key1, i));
      edgeMap.insert(EdgeMap::value_type(key2, i));
    }

    // generate vertex neighbors
    for (int i=0; i<numVertices; i++) {
      std::pair <VertMapIterator, VertMapIterator> ret;
      ret = vertMap.equal_range(i);
      for (VertMapIterator it=ret.first; it != ret.second; ++it) {
        int v1, v2;
        getAdjacentVertices(i, v1, v2, triangles[it->second]);
        generateNeighbor(i, v1, v2);
      }
    }

    // generate edge neighbors
    for (int i=0; i<numTriangles; i++) {
      for (int j=0; j<3; j++) {
        face_temp.vertices[j] = triangles[i].vertices[j];
      }

      faces.push_back(face_temp);

      for (int j=0; j<3; j++){
        unsigned int v1 = faces[i].vertices[index1[j]];
        unsigned int v2 = faces[i].vertices[index2[j]];
        std::string key = genKeyforEdge(v1, v2);
        std::pair <EdgeMapIterator, EdgeMapIterator> ret;
        ret = edgeMap.equal_range(key);
        for (EdgeMapIterator it=ret.first; it != ret.second; ++it) {
          faces[i].vertices_neighbor[index3[j]] = -1;
          int index = edgeIndexInTriangle(faces[i].vertices[index1[j]],
                                          faces[i].vertices[index2[j]],
                                          triangles[it->second]);
          if (index >= 0 && it->second != i) {
            faces[i].vertices_neighbor[index3[j]] = index;
            break;
          }
        }
      }
    }
}

int Mesh::edgeIndexInTriangle(unsigned int v1, unsigned int v2, MeshTriangle t3) {
    for(int i=0; i<3; i++) {
      if (v1 == t3.vertices[i]) {
        if (v2 == t3.vertices[index2[i]])
          return t3.vertices[index3[i]];
        if (v2 == t3.vertices[index3[i]])
          return t3.vertices[index2[i]];
      }
    }
    return -1;
}

void Mesh::generateNeighbor(unsigned int v, unsigned int v1, unsigned int v2) {
    bool flag1 = false;
    bool flag2 = false;
    for (size_t i = 0; i < neighbors[v].indices.size(); i++) {
        flag1 = flag1 || (neighbors[v].indices[i] == v1);
        flag2 = flag2 || (neighbors[v].indices[i] == v2);
    }
    if (!flag1) //if not found, then add it
    {
      neighbors[v].indices.push_back(v1);
    }
    if (!flag2) //if not found, then add it
    {
      neighbors[v].indices.push_back(v2);
    }
}

void Mesh::getAdjacentVertices(unsigned int v,int &v1,int &v2, MeshTriangle t3) {
    v1 = -1;
    v2 = -1;
    for(int i=0; i<3; i++) {
      if (v == t3.vertices[i]) {
        v1 = t3.vertices[index2[i]];
        v2 = t3.vertices[index3[i]];
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
    double duration = 0.0;
    double duration2 = 0.0;

    for (int i=0; i<numTriangles; i++) {
      for (int j=0; j<3; j++) {
        std::clock_t start = std::clock();
        int new_index = isEdgegenerateed(faces[i].vertices[index1[j]],
                                  faces[i].vertices[index2[j]],
                                  edges, &edgeMap, duration);
        duration2 += (std::clock() - start) / double(CLOCKS_PER_SEC);
        new_vertex[index3[j]] = new_index;
        if (new_index < 0) {
          if (faces[i].vertices_neighbor[index3[j]] >= 0) {
            odd_vertices_temp.position =
              0.375*(vertices[faces[i].vertices[index1[j]]].position
                    + vertices[faces[i].vertices[index2[j]]].position)
              + 0.125*(vertices[faces[i].vertices_neighbor[index3[j]]].position
                    + vertices[faces[i].vertices[index3[j]]].position);
            odd_vertices_temp.normal =
              0.375*(vertices[faces[i].vertices[index1[j]]].normal
                    + vertices[faces[i].vertices[index2[j]]].normal)
              + 0.125*(vertices[faces[i].vertices_neighbor[index3[j]]].normal
                    + vertices[faces[i].vertices[index3[j]]].normal);
            odd_vertices_temp.tex_coord =
              0.375*(vertices[faces[i].vertices[index1[j]]].tex_coord
                    + vertices[faces[i].vertices[index2[j]]].tex_coord)
              + 0.125*(vertices[faces[i].vertices_neighbor[index3[j]]].tex_coord
                    + vertices[faces[i].vertices[index3[j]]].tex_coord);
          } else {
            odd_vertices_temp.position =
              0.5*(vertices[faces[i].vertices[index1[j]]].position
                  + vertices[faces[i].vertices[index2[j]]].position);
            odd_vertices_temp.normal =
              0.5*(vertices[faces[i].vertices[index1[j]]].normal
                  + vertices[faces[i].vertices[index2[j]]].normal);
            odd_vertices_temp.tex_coord =
              0.5*(vertices[faces[i].vertices[index1[j]]].tex_coord
                  + vertices[faces[i].vertices[index2[j]]].tex_coord);
          }

          new_vertex[index3[j]] = temp_vertices.size();
          temp_vertices.push_back(odd_vertices_temp);

          edge_temp.vertices[0] = faces[i].vertices[index1[j]];
          edge_temp.vertices[1] = faces[i].vertices[index2[j]];
          edge_temp.vertex_new = new_vertex[index3[j]];
          edges.push_back(edge_temp);
        }
      }
      triangle_temp.vertices[0] = faces[i].vertices[0];
      triangle_temp.vertices[1] = new_vertex[2];
      triangle_temp.vertices[2] = new_vertex[1];
      temp_triangles.push_back(triangle_temp);

      triangle_temp.vertices[0] = faces[i].vertices[1];
      triangle_temp.vertices[1] = new_vertex[0];
      triangle_temp.vertices[2] = new_vertex[2];
      temp_triangles.push_back(triangle_temp);

      triangle_temp.vertices[0] = faces[i].vertices[2];
      triangle_temp.vertices[1] = new_vertex[1];
      triangle_temp.vertices[2] = new_vertex[0];
      temp_triangles.push_back(triangle_temp);

      triangle_temp.vertices[0] = new_vertex[0];
      triangle_temp.vertices[1] = new_vertex[1];
      triangle_temp.vertices[2] = new_vertex[2];
      temp_triangles.push_back(triangle_temp);
    }
    std::cout << "Is edge took " << duration << " seconds." << std::endl;
    std::cout << "Is edge took " << duration2 << " seconds." << std::endl;
}

int Mesh::isEdgegenerateed(unsigned int v1, unsigned int v2,
                      EdgeList &e, EdgeSingleMap* edgeMap,
                      double &duration) {
  std::clock_t start = std::clock();
  std::string key = genKeyforEdge(v1, v2);
  int count = edgeMap->count(key);
  if (count == 0) {
      edgeMap->insert(EdgeSingleMap::value_type(key, e.size()));
      duration += (std::clock() - start) / double(CLOCKS_PER_SEC);
      return -1;
  }
  int index = edgeMap->find(key)->second;
  duration += (std::clock() - start) / double(CLOCKS_PER_SEC);
  return e[index].vertex_new;
}

void Mesh::adjustEvenVertices() {
    MeshVertex even_vertices_temp;		//temp storage
    MeshVertexList even_vertices_temp_list;		//temp storage

    for (unsigned int i = 0; i < neighbors.size(); i++) {
      int N = neighbors[i].indices.size();
      if (N == 2) {
        even_vertices_temp.position =
            0.75*(vertices[i].position)
            + 0.125*(vertices[neighbors[i].indices[0]].position
            + vertices[neighbors[i].indices[1]].position);
        even_vertices_temp.normal =
            0.75*(vertices[i].normal)
            + 0.125*(vertices[neighbors[i].indices[0]].normal
            + vertices[neighbors[i].indices[1]].normal);
        even_vertices_temp.tex_coord =
            0.75*(vertices[i].tex_coord)
            + 0.125*(vertices[neighbors[i].indices[0]].tex_coord
            + vertices[neighbors[i].indices[1]].tex_coord);
      } else {
        double beta;
        double temp_beta = cos(2.0 * 3.1415926 / N);
        beta = (5.0/8.0-(3.0/8.0+0.25*temp_beta)*(3.0/8.0+0.25*temp_beta))/N;
        even_vertices_temp.position=(1-N*beta)*vertices[i].position;
        even_vertices_temp.normal=(1-N*beta)*vertices[i].normal;
        even_vertices_temp.tex_coord=(1-N*beta)*vertices[i].tex_coord;

        for (int j = 0; j < N; j++ ) {
          even_vertices_temp.position +=
              beta*vertices[neighbors[i].indices[j]].position;
          even_vertices_temp.normal +=
              beta*vertices[neighbors[i].indices[j]].normal;
          even_vertices_temp.tex_coord +=
              beta*vertices[neighbors[i].indices[j]].tex_coord;
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

std::string Mesh::genKeyforEdge(int v1, int v2) {
  if(v1 > v2) std::swap(v1, v2);
  std::string s1 = std::to_string(v1);
  std::string s2 = std::to_string(v2);
  return s1 + 'x' + s2;
}

} /* _462 */
