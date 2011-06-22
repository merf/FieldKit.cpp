/*                                                                           
 *      _____  __  _____  __     ____                                   
 *     / ___/ / / /____/ / /    /    \   FieldKit
 *    / ___/ /_/ /____/ / /__  /  /  /   (c) 2010, FIELD. All rights reserved.              
 *   /_/        /____/ /____/ /_____/    http://www.field.io           
 *   
 *	 Created by Marcus Wendt on 27/05/2010.
 */

#include "fieldkit/physics/behaviour/Force.h"

using namespace fieldkit::physics;

// -- Force --------------------------------------------------------------------
void Force::setDirection(Vec3f value) { 
	direction.set(value.normalized());
}

Vec3f Force::getDirection() { 
	return direction; 
}

void Force::prepare(float dt) {
	acceleration = direction * weight;
}

void Force::apply(Particle* p) {
	p->force += acceleration * p->weight;
}