/*                                                                           
 *      _____  __  _____  __     ____                                   
 *     / ___/ / / /____/ / /    /    \   FieldKit
 *    / ___/ /_/ /____/ / /__  /  /  /   (c) 2010, FIELD. All rights reserved.              
 *   /_/        /____/ /____/ /_____/    http://www.field.io           
 *   
 *	 Created by Marcus Wendt on 20/05/2010.
 */

#pragma once

#include <vector>
#include "fieldkit/physics/PhysicsKit_Prefix.h"
#include "fieldkit/physics/Behavioural.h"

namespace fieldkit { namespace physics {

	// FWD
    class Space;
	class Emitter;
    class Spring;
	class ParticleAllocator;
	class SpringAllocator;
	class SpringUpdate;
	class NeighbourUpdate;
	class ParticleUpdate;

	//! base class for all types of physics systems
	class Physics : public Behavioural {
	public:
		Emitter* emitter;
		Space* space;

		//! number of currently active particles
		int numActiveParticles;
		//! number of currently active springs
		int numActiveSprings;
		
		// Constructors
		Physics(Space* space);
		virtual ~Physics();
			
		virtual void update(float dt);
	
		// Particles
        std::vector<Particle*> particles;
		
		void allocParticles(int count);
		int getNumAllocatedParticles() { return numAllocatedParticles; }
		Particle* createParticle();
		void addParticle(Particle* particle);
		void retireParticle(const int id);
		void retireParticleRange(const int id_1, const int id_2);
		int getNumParticles() { return numActiveParticles; }
		bool hasParticlesAvailable(int num) { return num <= numAllocatedParticles - numActiveParticles; }
		void destroyParticles();

		// Springs
        std::vector<Spring*> springs;

		void allocSprings(int count);
		int getNumAllocated() { return numAllocatedSprings; }
		Spring* createSpring();
		void addSpring(Spring* spring);
		void removeSpring(Spring* spring);
		void retireSpring(const int id);
		void retireSpringRange(const int id_1, const int id_2);
		int getNumSprings() { return numActiveSprings; }
		bool hasSpringsAvailable(int num) { return num <= numAllocatedSprings - numActiveSprings; }
		void destroySprings();

		// Strategies
		void setParticleAllocator(ParticleAllocator* strategy);
		ParticleAllocator* getParticleAllocator() { return particleAllocator; };

		void setSpringAllocator(SpringAllocator* strategy);
		SpringAllocator* getSpringAllocator() { return springAllocator; };

		void setParticleUpdate(ParticleUpdate* strategy);
		ParticleUpdate* getParticleUpdate() { return particleUpdate; };

		void setSpringUpdate(SpringUpdate* strategy);
		SpringUpdate* getSpringUpdate() { return springUpdate; };

		void setNeighbourUpdate(NeighbourUpdate* strategy);
		NeighbourUpdate* getNeighbourUpdate() { return neighbourUpdate; };

		// Accessors
		void setOwnsSpace(bool isOwner) { ownsSpace = isOwner; }
		bool getOwnsSpace() { return ownsSpace; }

		int getNextID() { return ++nextID; }
	
	protected:
		bool ownsSpace;
		int numAllocatedParticles;
		int numAllocatedSprings;

		int nextID;

		ParticleAllocator* particleAllocator;
		SpringAllocator* springAllocator;
		ParticleUpdate* particleUpdate;
		SpringUpdate* springUpdate;
		NeighbourUpdate* neighbourUpdate;
	};

} } // namespace fieldkit::physics