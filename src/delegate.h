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

template <typename R, typename... P>
class Delegate
{
	typedef void* InstancePtr;
	typedef R (*FunctionPtr)(InstancePtr, P...);

public:
	// Bind a function
	template <R (*Function)(P...)>
	constexpr void bind(void)
	{
		m_inst = 0;
		m_func = &callFunction<Function>;
	}

	// Bind a class method
	template <class C, R (C::*Function)(P...)>
	constexpr void bind(C* inst)
	{
		m_inst = inst;
		m_func = &callClassMethod<C, Function>;
	}

	constexpr bool operator<(const Delegate<R, P...> & d) const
	{
		return m_func < d.m_func;
	}

	// Invoke delegate
	R operator()(P... p) const
	{
		assert(m_func != 0);
		return m_func(m_inst, p...);
	}

private:
	InstancePtr m_inst = 0;
	FunctionPtr m_func = 0;

	// Free function call wrapper
	template <R (*Function)(P...)>
	static R callFunction(InstancePtr, P... p)
	{
		return (Function)(p...);
	}

	// Member function call wrapper
	template <class C, R (C::*Function)(P...)>
	static R callClassMethod(InstancePtr inst, P... p)
	{
		return (static_cast<C*>(inst)->*Function)(p...);
	}
};

template <typename... P>
using Delegated = Delegate<void, P...>;

#endif // _DELEGATE_H
