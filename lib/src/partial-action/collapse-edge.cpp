#include <glm/glm.hpp>
#include "adjacent-iterator.hpp"
#include "affected-faces.hpp"
#include "partial-action/collapse-edge.hpp"
#include "partial-action/collapse-valence-3-vertex.hpp"
#include "partial-action/delete-edge-face.hpp"
#include "winged/edge.hpp"
#include "winged/face.hpp"
#include "winged/mesh.hpp"
#include "winged/vertex.hpp"

void PartialAction::collapseEdge ( WingedMesh& mesh, WingedEdge& edge
                                 , AffectedFaces& affectedFaces ) 
{
  assert (edge.leftFaceRef  ().numEdges () == 3);
  assert (edge.rightFaceRef ().numEdges () == 3);

  WingedVertex& vertex1        = edge.vertex1Ref ();
  WingedVertex& vertex2        = edge.vertex2Ref ();
  WingedEdge&   edgeToDelete1  = edge.leftSuccessorRef ();
  WingedEdge&   edgeToDelete2  = edge.rightSuccessorRef ();

  const unsigned int vertex3Index = edgeToDelete1.otherVertexRef (vertex2).index ();
  const unsigned int vertex4Index = edgeToDelete2.otherVertexRef (vertex1).index ();

#ifndef NDEBUG
  const unsigned int valence1 = vertex1.valence ();
  const unsigned int valence2 = vertex2.valence ();
  const unsigned int valence3 = edgeToDelete1.otherVertexRef (vertex2).valence ();
  const unsigned int valence4 = edgeToDelete2.otherVertexRef (vertex1).valence ();
#endif
  assert (valence1 > 3);
  assert (valence2 > 3);
  assert (valence3 > 3);
  assert (valence4 > 3);

  WingedVertex& newVertex = mesh.addVertex (edge.middle (mesh));

#ifndef NDEBUG
  const unsigned int newVertexIndex = newVertex.index ();
#endif

  PartialAction::deleteEdgeFace (mesh, edgeToDelete1, affectedFaces);
  PartialAction::deleteEdgeFace (mesh, edgeToDelete2, affectedFaces);

  EdgePtrVec remainingEdges1 = vertex1.adjacentEdges ().collect ();
  EdgePtrVec remainingEdges2 = vertex2.adjacentEdges ().collect ();

  WingedFace& left  = edge.leftFaceRef ();
  WingedFace& right = edge.rightFaceRef ();
  WingedEdge& lp    = edge.leftPredecessorRef ();
  WingedEdge& ls    = edge.leftSuccessorRef ();
  WingedEdge& rp    = edge.rightPredecessorRef ();
  WingedEdge& rs    = edge.rightSuccessorRef ();

  lp.successor   (left , &ls);
  ls.predecessor (left , &lp);
  rp.successor   (right, &rs);
  rs.predecessor (right, &rp);

  newVertex.edge (&ls);
  left     .edge (&ls);
  right    .edge (&rs);

  for (WingedEdge* e : remainingEdges1) {
    e->vertex (vertex1, &newVertex);
  }
  for (WingedEdge* e : remainingEdges2) {
    e->vertex (vertex2, &newVertex);
  }

  mesh.deleteEdge   (edge);
  mesh.deleteVertex (vertex1);
  mesh.deleteVertex (vertex2);

  for (WingedFace& f : newVertex.adjacentFaces ()) {
    assert (f.numEdges () == 3);
    affectedFaces.insert (f);
    f.writeIndices (mesh);
  }
  assert (newVertex.valence () == (valence1 - 3) + (valence2 - 3) + 2);
  assert (mesh.vertex (vertex3Index));

  WingedVertex& vertex3 = mesh.vertexRef (vertex3Index);

  if (vertex3.valence () == 3) {
    PartialAction::collapseValence3Vertex (mesh, vertex3, true, affectedFaces);
  }

  WingedVertex* vertex4 = mesh.vertex (vertex4Index);
  if (vertex4 && vertex4->valence () == 3) {
    PartialAction::collapseValence3Vertex (mesh, *vertex4, true, affectedFaces);
  }
  else if (vertex4 == false || vertex4->valence () < 3) {
    mesh         .reset ();
    affectedFaces.reset ();
    return;
  }

  if (mesh.vertex (newVertexIndex) == nullptr) {
    mesh         .reset ();
    affectedFaces.reset ();
    return;
  }
#ifndef NDEBUG
  else {
    for (WingedVertex& v : newVertex.adjacentVertices ()) {
      assert (v.valence () > 3);
    }
  }
#endif
}
