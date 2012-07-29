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

#ifndef _DELEGATE_H
#define _DELEGATE_H

#include <cassert>

template <typename R>
class Delegate0
{
	typedef void* InstancePtr;
	typedef R (*FunctionPtr)(InstancePtr);

public:
	Delegate0(void) : m_inst(0), m_func(0)
	{
		// ctor
	}

	Delegate0(const Delegate0 & other)
	{
		*this = other;
	}

	Delegate0 & operator=(const Delegate0 & other)
	{
		 m_inst = other.m_inst;
		 m_func = other.m_func;
		 return *this;
	}

	// Bind a function
	template <R (*Function)()>
	void bind(void)
	{
		m_inst = 0;
		m_func = &callFunction<Function>;
	}

	// Bind a class method
	template <class C, R (C::*Function)()>
	void bind(C* inst)
	{
		m_inst = inst;
		m_func = &callClassMethod<C, Function>;
	}

	// Invoke delegate
	R operator()() const
	{
		assert(m_func != 0);
		return m_func(m_inst);
	}

private:
	InstancePtr m_inst;
	FunctionPtr m_func;

	// Free function call wrapper
	template <R (*Function)()>
	static R callFunction(InstancePtr)
	{
		return (Function)();
	}

	// Member function call wrapper
	template <class C, R (C::*Function)()>
	static R callClassMethod(InstancePtr inst)
	{
		return (static_cast<C*>(inst)->*Function)();
	}
};

template <typename R, typename P>
class Delegate1
{
	typedef void* InstancePtr;
	typedef R (*FunctionPtr)(InstancePtr, P);

public:
	Delegate1(void) : m_inst(0), m_func(0)
	{
		// ctor
	}

	Delegate1(const Delegate1 & other)
	{
		*this = other;
	}

	Delegate1 & operator=(const Delegate1 & other)
	{
		 m_inst = other.m_inst;
		 m_func = other.m_func;
		return *this;
	}

	template <R (*Function)(P)>
	void bind(void)
	{
		m_inst = 0;
		m_func = &callFunction<Function>;
	}

	template <class C, R (C::*Function)(P)>
	void bind(C* inst)
	{
		m_inst = inst;
		m_func = &callClassMethod<C, Function>;
	}

	R operator()(P p) const
	{
		assert(m_func != 0);
		return m_func(m_inst, p);
	}

private:
	InstancePtr m_inst;
	FunctionPtr m_func;

	template <R (*Function)(P)>
	static R callFunction(InstancePtr, P p)
	{
		return (Function)(p);
	}

	template <class C, R (C::*Function)(P)>
	static R callClassMethod(InstancePtr inst, P p)
	{
		return (static_cast<C*>(inst)->*Function)(p);
	}
};

template <typename R, typename P1, typename P2>
class Delegate2
{
	typedef void* InstancePtr;
	typedef R (*FunctionPtr)(InstancePtr, P1, P2);

public:
	Delegate2(void) : m_inst(0), m_func(0)
	{
		// ctor
	}

	Delegate2(const Delegate2 & other)
	{
		*this = other;
	}

	Delegate2 & operator=(const Delegate2 & other)
	{
		 m_inst = other.m_inst;
		 m_func = other.m_func;
		 return *this;
	}

	template <R (*Function)(P1, P2)>
	void bind(void)
	{
		m_inst = 0;
		m_func = &callFunction<Function>;
	}

	template <class C, R (C::*Function)(P1, P2)>
	void bind(C* inst)
	{
		m_inst = inst;
		m_func = &callClassMethod<C, Function>;
	}

	R operator()(P1 p1, P2 p2) const
	{
		assert(m_func != 0);
		return m_func(m_inst, p1, p2);
	}

private:
	InstancePtr m_inst;
	FunctionPtr m_func;

	template <R (*Function)(P1, P2)>
	static R callFunction(InstancePtr, P1 p1, P2 p2)
	{
		return (Function)(p1, p2);
	}

	template <class C, R (C::*Function)(P1, P2)>
	static R callClassMethod(InstancePtr inst, P1 p1, P2 p2)
	{
		return (static_cast<C*>(inst)->*Function)(p1, p2);
	}
};

#endif // _DELEGATE_H
