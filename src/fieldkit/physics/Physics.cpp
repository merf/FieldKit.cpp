/*                                                                           
 *      _____  __  _____  __     ____                                   
 *     / ___/ / / /____/ / /    /    \   FieldKit
 *    / ___/ /_/ /____/ / /__  /  /  /   (c) 2010, FIELD. All rights reserved.              
 *   /_/        /____/ /____/ /_____/    http://www.field.io           
 *   
 *	 Created by Marcus Wendt on 20/05/2010.
 */

#include "fieldkit/physics/Physics.h"

#include "fieldkit/physics/space/Space.h"
#include "fieldkit/physics/Emitter.h"
#include "fieldkit/physics/Spring.h"
#include "fieldkit/physics/strategy/ParticleAllocator.h"
#include "fieldkit/physics/strategy/SpringAllocator.h"
#include "fieldkit/physics/strategy/ParticleUpdate.h"
#include "fieldkit/physics/strategy/SpringUpdate.h"
#include "fieldkit/physics/strategy/NeighbourUpdate.h"

using namespace fieldkit::physics;

Physics::Physics(Space* space)
{
	this->space = space;
	
	numActiveParticles = 0;
	numActiveSprings = 0;
	numAllocatedParticles = 0;
	numAllocatedSprings = 0;

	nextID = 0;
	
	emitter = NULL;
	ownsSpace = true;
	
	particleAllocator = NULL;
	particleUpdate = NULL;
	springUpdate = NULL;
	neighbourUpdate = NULL;

//	setParticleAllocator(new ParticleAllocator());
//	setParticleUpdate(new ParticleUpdate());
//	setSpringUpdate(new SpringUpdate());
	// no neighbour update strategy by default
	
	particleAllocator = new ParticleAllocator();
	springAllocator = new SpringAllocator();
	particleUpdate = new ParticleUpdate();
	springUpdate = new SpringUpdate();
}

Physics::~Physics() 
{
	// strategies
	if(particleAllocator != NULL) {
		delete particleAllocator;
		particleAllocator = NULL;
	}

	if(springAllocator != NULL) {
		delete springAllocator;
		springAllocator = NULL;
	}

	if(particleUpdate != NULL) {
		delete particleUpdate;
		particleUpdate = NULL;
	}

	if(springUpdate != NULL) {
		delete springUpdate;
		springUpdate = NULL;
	}

	if(neighbourUpdate != NULL) {
		delete neighbourUpdate;
		neighbourUpdate = NULL;
	}

	// springs & particles
	destroySprings();
	destroyParticles();

	// emitter
	if(emitter != NULL) {
		delete emitter;
		emitter = NULL;
	}
	
	// space
	if(ownsSpace) {
		delete space;
		space = NULL;
	}
}

void Physics::update(float dt)
{
	if(emitter != NULL)
		emitter->update(dt);
	
	if(particleUpdate != NULL)
		particleUpdate->apply(this, dt);

	if(springUpdate != NULL)
		springUpdate->apply(this);

	if(neighbourUpdate != NULL)
		neighbourUpdate->apply(this);
}

// -- Particles ----------------------------------------------------------------
// check if we still have a dead particle that we can reuse, otherwise create a new one
// in either case the 'new' particle is assigned a fresh id.
Particle* Physics::createParticle() 
{
	numActiveParticles++;
	BOOST_FOREACH(Particle* p, particles) {
		if(!p->isAlive)
		{
			p->id = getNextID();
			return p;
		}
	}
	
	if(particleAllocator != NULL)
		particleAllocator->apply(this);

	Particle* p = particles.back();
	p->id = getNextID();
	return p;
}

// allocates a bunch of new particles
void Physics::allocParticles(int count) 
{
	numAllocatedParticles = particles.size() + count;
	space->reserve(numAllocatedParticles);
	particles.reserve(numAllocatedParticles);
	
	for(int i=0; i<count; i++)
		particleAllocator->apply(this);
}

void Physics::addParticle(Particle* particle)
{
	particles.push_back(particle);
}

// retiring a particle sets its isAlive flag to false allowing it to recycled later
// the retireX functions are designed to be used alongside the createX functions
// as this is where ids are assigned
void Physics::retireParticle(const int id)
{
	BOOST_FOREACH(Particle* p, particles)
	{
		if(id == p->id)
		{
			//we assume that a spring is never apped to the list twice.
			p->isAlive = false;
			numActiveParticles--;
			return;
		}
	}
}

// retireParticleRange retires a range of particles beginning with id_1 and ending with id_2
void Physics::retireParticleRange(const int id_1, const int id_2)
{
	BOOST_FOREACH(Particle* p, particles)
	{
		if(id_1 <= p->id && p->id <= id_2)
		{
			p->isAlive = false;
			numActiveParticles--;
		}
	}
}

void Physics::destroyParticles()
{
	BOOST_FOREACH(Particle* p, particles) {
		delete p;
		p = NULL;
	}
	particles.clear();

	numAllocatedParticles = 0;
	numActiveParticles = 0;
}


// -- Springs ------------------------------------------------------------------
// check if we still have a dead particle that we can reuse, otherwise create a new one
Spring* Physics::createSpring() 
{
	numActiveSprings++;
	BOOST_FOREACH(Spring* s, springs) {
		if(!s->isAlive)
		{
			s->id = getNextID();
			return s;
		}
	}

	if(springAllocator != NULL)
		springAllocator->apply(this);

	Spring* s = springs.back();
	s->id = getNextID();
	return s;
}

// allocates a bunch of new springs
void Physics::allocSprings(int count) 
{
	numAllocatedSprings = springs.size() + count;
	space->reserve(numAllocatedSprings);
	particles.reserve(numAllocatedSprings);

	for(int i=0; i<count; i++)
		springAllocator->apply(this);
}

void Physics::addSpring(Spring* spring) 
{
	springs.push_back(spring);
}

void Physics::retireSpring(const int id)
{
	BOOST_FOREACH(Spring* s, springs)
	{
		if(id == s->id)
		{
			//we assume that a spring is never apped to the list twice.
			s->isAlive = false;
			numActiveSprings--;
			return;
		}
	}
}

void Physics::retireSpringRange(const int id_1, const int id_2)
{
	bool in_range = false;
	BOOST_FOREACH(Spring* s, springs)
	{
		if(id_1 <= s->id && s->id <= id_2)
		{
			s->isAlive = false;
			numActiveSprings--;
		}
	}

	printf("end spring in range not found");
}

void Physics::removeSpring(Spring* spring)
{
	// TODO
//	springs.erase(spring);
}

void Physics::destroySprings()
{
	if(springs.size() == 0) return;
	
	BOOST_FOREACH(Spring* s, springs) {
		delete s;
	}
	springs.clear();

	numAllocatedSprings = 0;
	numActiveSprings = 0;
}

// -- Setters -----------------------------------------------------------------
void Physics::setParticleAllocator(ParticleAllocator* strategy )
{
	if(particleAllocator != NULL) {
		delete particleAllocator;
	}
	particleAllocator = strategy;
}

void Physics::setSpringAllocator(SpringAllocator* strategy )
{
	if(springAllocator != NULL) {
		delete springAllocator;
	}
	springAllocator = strategy;
}

void Physics::setParticleUpdate(ParticleUpdate* strategy )
{
	if(particleUpdate != NULL) {
		delete particleUpdate;
	}
	particleUpdate = strategy;
}

void Physics::setSpringUpdate(SpringUpdate* strategy )
{
	if(springUpdate != NULL) {
		delete springUpdate;
	}
	springUpdate = strategy;
}

void Physics::setNeighbourUpdate(NeighbourUpdate* strategy )
{
	if(neighbourUpdate != NULL) {
		delete neighbourUpdate;
	}
	neighbourUpdate = strategy;
}
