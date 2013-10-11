#include <QApplication>
#include <QKeyEvent>
#include <glm/glm.hpp>
#include "view-gl-widget.hpp"
#include "renderer.hpp"
#include "view-mouse-movement.hpp"
#include "axis.hpp"
#include "state.hpp"
#include "macro.hpp"
#include "camera.hpp"
#include "winged-util.hpp"
#include "winged-mesh.hpp"
#include "intersection.hpp"
#include "ray.hpp"
#include "cursor.hpp"
#include "winged-face.hpp"
#include "tool.hpp"
#include "tool-rotate.hpp"
#include "history.hpp"

#include <iostream> // delete this
#include "adjacent-iterator.hpp" // delete this
#include "winged-edge.hpp" // delete this
#include "action/test.hpp" // delete this

struct GLWidgetImpl {
  Axis          axis;
};

GLWidget :: GLWidget (const QGLFormat& format) : QGLWidget (format) 
                                               , impl (new GLWidgetImpl)
                                               {}

GLWidget :: ~GLWidget () {
  delete this->impl;
}

void GLWidget :: initializeGL () {
  Renderer :: initialize ();
  State    :: initialize ();

  this->impl->axis.initialize ();
  this->setMouseTracking (true);

  Renderer :: updateLights (State :: camera ());
}

void GLWidget :: paintGL () {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  State :: render ();
  this->impl->axis.render ();
}

void GLWidget :: resizeGL (int w, int h) {
  State :: camera ().updateResolution (glm::uvec2 (w,h));
  glViewport (0,0,w, h < 1 ? 1 : h );
}

void GLWidget :: keyPressEvent (QKeyEvent* e) {
  switch (e->key()) {
    case Qt::Key_Escape:
      QCoreApplication::instance()->quit();
      break;
    case Qt::Key_W:
      State :: mesh ().toggleRenderMode ();
      this->update ();
      break;
    case Qt::Key_I:
      WingedUtil :: printStatistics (State :: mesh ());
      break;
    case Qt::Key_Z:
      if (e->modifiers () == Qt::ControlModifier) {
        State :: history ().undo ();
        State :: writeAllData  ();
        State :: bufferAllData ();
        this->update ();
      }
      else
        QGLWidget::keyPressEvent (e);
      break;
    case Qt::Key_Y:
      if (e->modifiers () == Qt::ControlModifier) {
        State :: history ().redo ();
        State :: writeAllData  ();
        State :: bufferAllData ();
        this->update ();
      }
      else
        QGLWidget::keyPressEvent (e);
      break;
    case Qt::Key_X: {
        ActionTest a;
        a.run (State :: mesh (), State::mesh ().edgeSLOW (Id (138)));
        //a.run (State :: mesh (), State::mesh ().edgeSLOW (Id (924)));
        State::writeAllData ();
        State::bufferAllData ();
        this->update ();
        break;
      }
    default:
      QGLWidget::keyPressEvent (e);
  }
}

void GLWidget :: mouseMoveEvent (QMouseEvent* e) {
  State :: mouseMovement ().update (e->pos ());
  if (e->buttons () == Qt :: NoButton) {
    WingedMesh& mesh = State :: mesh ();
    Ray ray          = State :: camera ().getRay (glm::uvec2 (e->x (), e->y ()));

    FaceIntersection intersection;
    mesh.intersectRay (ray, intersection);

    if (intersection.isIntersection ()) {
      /*
      std::cout << intersection.face ().id () << " ";
      for (ADJACENT_EDGE_ITERATOR (it,intersection.face ())) {
        std::cout << it.element ().id () << " ";
      }
      std::cout << std::endl;
      */

      glm::vec3 pos    = intersection.position ();
      glm::vec3 normal = intersection.face ().normal (mesh);

      State :: cursor ().enable      ();
      State :: cursor ().setPosition (pos);
      State :: cursor ().setNormal   (normal);
      this->update ();
    }
    else if (State :: cursor ().isEnabled ()) {
      State :: cursor ().disable ();
      this->update ();
    }
  }
  if (e->buttons () == Qt :: MiddleButton) {
    ToolRotate :: run ();
    this->update ();
  }
}

void GLWidget :: mousePressEvent (QMouseEvent* e) {
  if (e->buttons () == Qt :: LeftButton) {
    if (Tool :: click ())
//    if (ToolCarve :: run ())
      this->update ();
  }
}

void GLWidget :: mouseReleaseEvent (QMouseEvent*) {
  State :: mouseMovement ().invalidate ();
}

void GLWidget :: resizeEvent (QResizeEvent* e) {
  State :: camera ().updateResolution (glm::uvec2 ( e->size ().width  ()
                                                  , e->size ().height ()));
}

void GLWidget :: wheelEvent (QWheelEvent* e) {
  if (e->orientation () == Qt :: Vertical) {
    if (e->delta () > 0)
      State :: camera ().stepAlongGaze (true);
    else if (e->delta () < 0)
      State :: camera ().stepAlongGaze (false);
  }
  this->update ();
}
