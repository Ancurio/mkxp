/*
** plane-binding.cpp
**
** This file is part of mkxp.
**
** Copyright (C) 2013 Jonas Kulla <Nyocurio@gmail.com>
**
** mkxp is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 2 of the License, or
** (at your option) any later version.
**
** mkxp is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with mkxp.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "binding-types.h"
#include "binding-util.h"
#include "disposable-binding.h"
#include "plane.h"
#include "viewportelement-binding.h"

#if RAPI_FULL > 187
DEF_TYPE(Plane);
#else
DEF_ALLOCFUNC(Plane);
#endif

RB_METHOD(planeInitialize) {
  Plane *p = viewportElementInitialize<Plane>(argc, argv, self);

  setPrivateData(self, p);

  p->initDynAttribs();

  wrapProperty(self, &p->getColor(), "color", ColorType);
  wrapProperty(self, &p->getTone(), "tone", ToneType);

  return self;
}

DEF_PROP_OBJ_REF(Plane, Bitmap, Bitmap, "bitmap")
DEF_PROP_OBJ_VAL(Plane, Color, Color, "color")
DEF_PROP_OBJ_VAL(Plane, Tone, Tone, "tone")

DEF_PROP_I(Plane, OX)
DEF_PROP_I(Plane, OY)
DEF_PROP_I(Plane, Opacity)
DEF_PROP_I(Plane, BlendType)

DEF_PROP_F(Plane, ZoomX)
DEF_PROP_F(Plane, ZoomY)

void planeBindingInit() {
  VALUE klass = rb_define_class("Plane", rb_cObject);
#if RAPI_FULL > 187
  rb_define_alloc_func(klass, classAllocate<&PlaneType>);
#else
  rb_define_alloc_func(klass, PlaneAllocate);
#endif

  disposableBindingInit<Plane>(klass);
  viewportElementBindingInit<Plane>(klass);

  _rb_define_method(klass, "initialize", planeInitialize);

  INIT_PROP_BIND(Plane, Bitmap, "bitmap");
  INIT_PROP_BIND(Plane, OX, "ox");
  INIT_PROP_BIND(Plane, OY, "oy");
  INIT_PROP_BIND(Plane, ZoomX, "zoom_x");
  INIT_PROP_BIND(Plane, ZoomY, "zoom_y");
  INIT_PROP_BIND(Plane, Opacity, "opacity");
  INIT_PROP_BIND(Plane, BlendType, "blend_type");
  INIT_PROP_BIND(Plane, Color, "color");
  INIT_PROP_BIND(Plane, Tone, "tone");
}
