/* This file is part of Dilay
 * Copyright © 2015-2018 Alexander Bau
 * Use and redistribute under the terms of the GNU General Public License
 */
#include <QCheckBox>
#include <QFrame>
#include <QRadioButton>
#include "cache.hpp"
#include "dimension.hpp"
#include "scene.hpp"
#include "sketch/mesh.hpp"
#include "sketch/node-intersection.hpp"
#include "sketch/path-intersection.hpp"
#include "state.hpp"
#include "tools.hpp"
#include "util.hpp"
#include "view/pointing-event.hpp"
#include "view/tool-tip.hpp"
#include "view/two-column-grid.hpp"
#include "view/util.hpp"

namespace
{
  enum class Mode
  {
    DeleteSketch,
    DeleteNode,
    DeleteSpheres
  };
}

struct ToolDeleteSketch::Impl
{
  ToolDeleteSketch* self;
  Mode              mode;
  bool              deleteChildren;

  Impl (ToolDeleteSketch* s)
    : self (s)
    , mode (Mode (s->cache ().get<int> ("mode", int(Mode::DeleteNode))))
    , deleteChildren (s->cache ().get<bool> ("delete-children", false))
  {
  }

  ToolResponse runInitialize ()
  {
    this->self->supportsMirror ();

    this->setupProperties ();
    this->setupToolTip ();

    return ToolResponse::None;
  }

  void setupProperties ()
  {
    ViewTwoColumnGrid& properties = this->self->properties ();

    QCheckBox& deleteChildrenEdit =
      ViewUtil::checkBox (QObject::tr ("Delete children"), this->deleteChildren);
    ViewUtil::connect (deleteChildrenEdit, [this](bool m) {
      this->deleteChildren = m;
      this->self->cache ().set ("delete-children", m);
    });
    deleteChildrenEdit.setEnabled (this->mode == Mode::DeleteNode);

    QRadioButton& deleteSketchEdit =
      ViewUtil::radioButton (QObject::tr ("Delete sketch"), this->mode == Mode::DeleteSketch);
    ViewUtil::connect (deleteSketchEdit, [this, &deleteChildrenEdit](bool m) {
      this->mode = Mode::DeleteSketch;
      this->self->cache ().set ("mode", int(this->mode));

      deleteChildrenEdit.setEnabled (!m);
      this->self->enableMirrorProperties (!m);
    });

    QRadioButton& deleteNodeEdit =
      ViewUtil::radioButton (QObject::tr ("Delete node"), this->mode == Mode::DeleteNode);
    ViewUtil::connect (deleteNodeEdit, [this, &deleteChildrenEdit](bool m) {
      this->mode = Mode::DeleteNode;
      this->self->cache ().set ("mode", int(this->mode));

      deleteChildrenEdit.setEnabled (m);
      this->self->enableMirrorProperties (m);
    });

    QRadioButton& deleteSpheresEdit =
      ViewUtil::radioButton (QObject::tr ("Delete spheres"), this->mode == Mode::DeleteSpheres);
    ViewUtil::connect (deleteSpheresEdit, [this, &deleteChildrenEdit](bool m) {
      this->mode = Mode::DeleteSpheres;
      this->self->cache ().set ("mode", int(this->mode));

      deleteChildrenEdit.setEnabled (!m);
      this->self->enableMirrorProperties (m);
    });

    properties.add (deleteSketchEdit);
    properties.add (deleteNodeEdit);
    properties.add (deleteSpheresEdit);
    properties.add (ViewUtil::horizontalLine ());

    this->self->addMirrorProperties ();
    this->self->enableMirrorProperties (this->mode != Mode::DeleteSketch);

    properties.add (deleteChildrenEdit);
  }

  void setupToolTip ()
  {
    ViewToolTip toolTip;
    toolTip.add (ViewInputEvent::MouseLeft, QObject::tr ("Delete selection"));
    this->self->state ().setToolTip (&toolTip);
  }

  ToolResponse runReleaseEvent (const ViewPointingEvent& e)
  {
    if (e.leftButton ())
    {
      switch (this->mode)
      {
        case Mode::DeleteSketch:
        {
          SketchMeshIntersection intersection;
          if (this->self->intersectsScene (e, intersection))
          {
            this->self->snapshotSketchMeshes ();
            this->self->state ().scene ().deleteMesh (intersection.mesh ());
          }
          return ToolResponse::Redraw;
        }
        case Mode::DeleteNode:
        {
          SketchNodeIntersection intersection;
          if (this->self->intersectsScene (e, intersection))
          {
            this->self->snapshotSketchMeshes ();
            intersection.mesh ().deleteNode (intersection.node (), this->deleteChildren,
                                             this->self->mirrorDimension ());
            if (intersection.mesh ().isEmpty ())
            {
              this->self->state ().scene ().deleteMesh (intersection.mesh ());
            }
          }
          return ToolResponse::Redraw;
        }
        case Mode::DeleteSpheres:
        {
          SketchPathIntersection intersection;
          if (this->self->intersectsScene (e, intersection))
          {
            this->self->snapshotSketchMeshes ();
            intersection.mesh ().deletePath (intersection.path (), this->self->mirrorDimension ());
            if (intersection.mesh ().isEmpty ())
            {
              this->self->state ().scene ().deleteMesh (intersection.mesh ());
            }
          }
          return ToolResponse::Redraw;
        }
      }
    }
    return ToolResponse::None;
  }
};

DELEGATE_TOOL (ToolDeleteSketch)
DELEGATE_TOOL_RUN_RELEASE_EVENT (ToolDeleteSketch)
