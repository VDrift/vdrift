/************************************************************************/
/*                                                                      */
/* This file is part of VDrift.                                         */
/*                                                                      */
/* VDrift is free software: you can redistribute it and/or modify       */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation, either version 3 of the License, or    */
/* (at your option) any later version.                                  */
/*                                                                      */
/* VDrift is distributed in the hope that it will be useful,            */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */
/*                                                                      */
/* You should have received a copy of the GNU General Public License    */
/* along with VDrift.  If not, see <http://www.gnu.org/licenses/>.      */
/*                                                                      */
/************************************************************************/

#include "wheel.h"
#include "world.h"
#include "fracturebody.h"
#include "wheelcontact.h"
#include "surface.h"

namespace sim
{

static btRigidBody & getFixedBody()
{
	static btRigidBody fixed(0, 0, 0);
	fixed.setMassProps(0, btVector3(0, 0, 0));
	return fixed;
}

Wheel::Wheel() :
	world(0),
	body(0),
	angvel(0),
	radius(0.3),
	width(0.2),
	mass(1),
	antiroll(0),
	has_contact(false),
	abs_enabled(false),
	tcs_enabled(false),
	abs_active(false),
	tcs_active(false)
{
	//ctor
}

void Wheel::init(
	const WheelInfo & info,
	World & nworld,
	FractureBody & nbody)
{
	tire.init(info.tire);
	suspension.init(info.suspension);
	brake.init(info.brake);
	tire.init(info.tire);
	shaft.setInertia(info.inertia);
	radius = info.radius;
	width = info.width;
	mass = info.mass;
	world = &nworld;
	body = &nbody;
}

bool Wheel::updateDisplacement(btScalar raylen)
{
	// update wheel transform
	transform.setOrigin(body->getCenterOfMassOffset() + suspension.getPosition());
	transform.setRotation(suspension.getOrientation());
	transform = body->getCenterOfMassTransform() * transform;

	// wheel contact
	btVector3 wheelPos = transform.getOrigin();
	btVector3 rayDir = -transform.getBasis().getColumn(2); // down
	btScalar rayLen = radius + raylen;
	btVector3 rayStart = wheelPos - rayDir * radius;
	ray.set(rayStart, rayDir, rayLen);
	ray.m_exclude = body;
	world->rayTest(ray);

	// surface bumpiness
	btScalar bump = 0;
	const Surface * surface = ray.getSurface();
	if (surface)
	{
		btScalar posx = ray.getPoint()[0];
		btScalar posz = ray.getPoint()[2];
		btScalar phase = 2 * M_PI * (posx + posz) / surface->bumpWaveLength;
		btScalar shift = 2 * btSin(phase * M_PI_2);
		btScalar amplitude = 0.25 * surface->bumpAmplitude;
		bump = amplitude * (btSin(phase + shift) + btSin(M_PI_2 * phase) - 2.0f);
	}

	// update suspension
	btScalar relDisplacement = 2 * radius - ray.getDepth() + bump;
	btScalar displacement = suspension.getDisplacement() + relDisplacement;
	suspension.setDisplacement(displacement);

	has_contact = surface && displacement >= 0;
	return has_contact;
}

bool Wheel::updateContact(btScalar dt, WheelContact & contact)
{
	if (!has_contact) return false;

	const Surface * surface = ray.getSurface();
	contact.frictionCoeff = tire.getTread() * surface->frictionTread +
		(1.0 - tire.getTread()) * surface->frictionNonTread;

	btRigidBody * bodyA = body;
	btRigidBody * bodyB = &getFixedBody();
	if (btRigidBody::upcast(ray.m_collisionObject))
	{
		bodyB = btRigidBody::upcast(ray.m_collisionObject);
	}

	btVector3 wheelTangent1 = transform.getBasis().getColumn(1); // forward
	btVector3 wheelTangent2 = transform.getBasis().getColumn(0); // right
	btVector3 wheelNormal = transform.getBasis().getColumn(2); // up

	btVector3 contactNormal = ray.getNormal();
	btVector3 contactPointA = ray.getPoint();
	btVector3 contactPointB = ray.getPoint();

	// suspension response
	btScalar stiffnessConstant = suspension.getStiffness() + antiroll;
	btScalar dampingConstant = suspension.getDamping();
	btScalar displacement = suspension.getDisplacement();
	if (suspension.getOvertravel() > 0.0)
	{
		// combine spring and bump stiffness
		btScalar bumpStiffness = 5E5;
		btScalar overtravel = suspension.getOvertravel();
		displacement += overtravel;
		stiffnessConstant += bumpStiffness * overtravel / displacement;
	}

	// update constraints
	btVector3 rA = contactPointA - bodyA->getCenterOfMassPosition();
	btVector3 rB = contactPointB - bodyB->getCenterOfMassPosition();

	btVector3 contactTangent1 = wheelTangent1 - contactNormal * contactNormal.dot(wheelTangent1);
	btVector3 contactTangent2 = wheelTangent2 - contactNormal * contactNormal.dot(wheelTangent2);
	contactTangent1.normalize();
	contactTangent2.normalize();

	// project wheel normal onto contact forward facing plane to calculate camber
	btVector3 projNormal = wheelNormal - wheelNormal.dot(contactTangent1) * contactTangent1;
	projNormal.normalize();

	contact.camber = 0;//btAcos(projNormal.dot(contactNormal)) * SIMD_DEGS_PER_RAD; fixme
	contact.vR = shaft.getAngularVelocity() * radius;
	contact.bodyA = bodyA;
	contact.bodyB = bodyB;
	contact.rA = rA;
	contact.rB = rB;

	btVector3 vA = bodyA->getLinearVelocity() + bodyA->getAngularVelocity().cross(rA);
	btVector3 vB = bodyB->getLinearVelocity() + bodyB->getAngularVelocity().cross(rB);
	btVector3 vAB = vA - vB;

	// set suspension constraint
	{
		// CFM and ERP from spring stiffness and damping constants
		btAssert(stiffnessConstant >= 0);
		btScalar softness = 1.0f / (dt * (dt * stiffnessConstant + dampingConstant));
		btScalar biasFactor = stiffnessConstant / (dt * stiffnessConstant + dampingConstant);
		btScalar velocityError = -biasFactor * displacement;

		btVector3 normal = contactNormal;
		btScalar denomA = bodyA->computeImpulseDenominator(contactPointA, normal);
		btScalar denomB = bodyB->computeImpulseDenominator(contactPointB, normal);
		btScalar jacDiagInv = 1 / (denomA + denomB + softness);

		contact.response.jacDiagInv = jacDiagInv;
		contact.response.rhs = -velocityError * jacDiagInv;
		contact.response.cfm = -softness * jacDiagInv;
		contact.response.lowerLimit = 0;
		contact.response.upperLimit = SIMD_INFINITY;
		contact.response.accumImpulse = 0;
		contact.response.normal = normal;
		contact.response.angularCompA = bodyA->getInvInertiaTensorWorld() * rA.cross(normal);
		contact.response.angularCompB = bodyB->getInvInertiaTensorWorld() * rB.cross(normal);
	}

	// set longitudinal friction constraint
	{
		btVector3 normal = contactTangent1;
		btScalar denomA = bodyA->computeImpulseDenominator(contactPointA, normal);
		btScalar denomB = bodyB->computeImpulseDenominator(contactPointB, normal);
		btScalar jacDiagInv =  1 / (denomA + denomB);
		btScalar velocityError = vAB.dot(normal) - contact.vR;

		contact.v1 = velocityError + contact.vR;
		contact.friction1.jacDiagInv = jacDiagInv;
		contact.friction1.rhs = -velocityError * jacDiagInv;
		contact.friction1.cfm = 0;
		contact.friction1.lowerLimit = 0;
		contact.friction1.upperLimit = 0;
		contact.friction1.accumImpulse = 0;
		contact.friction1.normal = normal;
		contact.friction1.angularCompA = bodyA->getInvInertiaTensorWorld() * rA.cross(normal);
		contact.friction1.angularCompB = bodyB->getInvInertiaTensorWorld() * rB.cross(normal);
	}

	// set lateral friction constraint
	{
		btVector3 normal = contactTangent2;
		btScalar denomA = bodyA->computeImpulseDenominator(contactPointA, normal);
		btScalar denomB = bodyB->computeImpulseDenominator(contactPointB, normal);
		btScalar jacDiagInv =  1 / (denomA + denomB);
		btScalar velocityError = vAB.dot(normal);

		contact.v2 = velocityError;
		contact.friction2.jacDiagInv = jacDiagInv;
		contact.friction2.rhs = -velocityError * jacDiagInv;
		contact.friction2.cfm = 0;
		contact.friction2.lowerLimit = 0;
		contact.friction2.upperLimit = 0;
		contact.friction2.accumImpulse = 0;
		contact.friction2.normal = normal;
		contact.friction2.angularCompA = bodyA->getInvInertiaTensorWorld() * rA.cross(normal);
		contact.friction2.angularCompB = bodyB->getInvInertiaTensorWorld() * rB.cross(normal);
	}

	// ABS
	abs_active = false;
	btScalar brake_torque = brake.getTorque();
	btScalar slide = tire.getSlide();
	btScalar ideal_slide = tire.getIdealSlide();
	if (abs_enabled && (brake_torque > 1E-3) && (btFabs(contact.v1) > 3) && (slide < -ideal_slide))
	{
		// predict new angvel
		btScalar angvel_delta = shaft.getAngularVelocity() - angvel;
		btScalar angvel_new = shaft.getAngularVelocity() + angvel_delta;

		// calculate brake torque correction to reach 95% ideal_slide
		btScalar angvel_target = (0.95 * ideal_slide + 1) * contact.v1 / radius;
		angvel_delta = angvel_new - angvel_target;
		if (angvel_delta < 0)
		{
			// set brake torque to reach angvel_target
			brake_torque += angvel_delta / dt * shaft.getInertia();
			btScalar factor = brake_torque / brake.getMaxTorque();
			btClamp(factor, btScalar(0), btScalar(1));
			brake.setBrakeFactor(factor);
			abs_active = true;
		}
	}

	// TCS
	tcs_active = false;
	if (tcs_enabled && (slide > ideal_slide))
	{
		// predict new angvel
		btScalar angvel_delta = shaft.getAngularVelocity() - angvel;
		btScalar angvel_new = shaft.getAngularVelocity() + angvel_delta;

		// calculate brake torque correction to reach 95% ideal_slide
		btScalar angvel_target = (0.95 * ideal_slide + 1) * contact.v1 / radius;
		angvel_delta = angvel_new - angvel_target;
		if (angvel_delta > 0)
		{
			// set brake torque to reach angvel_target
			btScalar brake_torque = angvel_delta / dt * shaft.getInertia();
			btScalar factor = brake_torque / brake.getMaxTorque();
			btClamp(factor, brake.getBrakeFactor(), btScalar(1));
			brake.setBrakeFactor(factor);
			tcs_active = true;
		}
	}

	// store old angular velocity
	angvel = shaft.getAngularVelocity();

	return true;
}

}
