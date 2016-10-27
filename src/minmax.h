#ifndef _MINMAX_H
#define _MINMAX_H

template <typename T>
T Min(T u, T v)
{
	return u < v ? u : v;
}

template <typename T>
T Max(T u, T v)
{
	return u > v ? u : v;
}

template <typename T>
T Clamp(T v, T vmin, T vmax)
{
	return Min(Max(v, vmin), vmax);
}

#endif // _MINMAX_H
