/*                                                                           
 *      _____  __  _____  __     ____                                   
 *     / ___/ / / /____/ / /    /    \   FieldKit
 *    / ___/ /_/ /____/ / /__  /  /  /   (c) 2010, FIELD. All rights reserved.              
 *   /_/        /____/ /____/ /_____/    http://www.field.io           
 *   
 *	 Created by Marcus Wendt on 27/05/2010.
 */

#include "fieldkit/physics/Spring.h"

using namespace fieldkit::physics;
	
Spring::Spring() : 
isAlive(false), restLength(0), strength(0), isALocked(false), isBLocked(false) {
}

Spring::Spring(Particle* a, Particle* b, float restLength, float strength) :
a(a), b(b), isAlive(true), restLength(restLength), strength(strength), isALocked(false), isBLocked(false) {
}

void Spring::update() {
	Vec3f delta = b->position - a->position;
	float dist = delta.length() + (float)EPSILON_VALUE;
	float normDistStrength = (dist - restLength) / (dist * (a->invWeight + b->invWeight)) * strength;
	
	if(!a->isLocked && !isALocked) {
		a->position += delta * normDistStrength * a->invWeight;
	}
	
	if(!b->isLocked && !isBLocked) {
		b->position += delta * -normDistStrength * b->invWeight;
	}
}

void Spring::reset() {
	isAlive = false;
	a = NULL;
	b = NULL;
}

void Spring::init(Particle* a, Particle* b, float restLength, float strength) {
	this->a = a;
	this->b = b;
	this->restLength = restLength;
	this->strength = strength;

	isAlive = true;
}
