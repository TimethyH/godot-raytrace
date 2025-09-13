/**************************************************************************/
/*  renderer_ray_trace_settings.cpp                                       */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "renderer_ray_trace_settings.h"
#include "core/config/project_settings.h"

RendererRayTraceSettings *RendererRayTraceSettings::singleton = nullptr;

RendererRayTraceSettings::RendererRayTraceSettings() {
	if (singleton == nullptr) {
		singleton = this;
	}

	ProjectSettings::get_singleton()->connect("settings_changed", callable_mp(this, &RendererRayTraceSettings::on_settings_changed));

	enable_shadows = GLOBAL_GET("rendering/ray_tracing/ray_traced_shadows");
}

bool RendererRayTraceSettings::get_shadows() const {
	return enable_shadows;
}

void RendererRayTraceSettings::set_shadows(bool p_enable) {
	enable_shadows = p_enable;
}

void RendererRayTraceSettings::on_settings_changed() {
	bool new_shadows_setting = GLOBAL_GET("rendering/ray_tracing/ray_traced_shadows");

	if (enable_shadows != new_shadows_setting)
	{
		enable_shadows = new_shadows_setting;
	}
}

RendererRayTraceSettings *RendererRayTraceSettings::get_singleton() {
	return singleton;
}


