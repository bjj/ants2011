#ifndef __ENABLE_IF_
#define __ENABLE_IF_

template<bool _Cond, typename _Tp>
struct enable_if 
{ };

template<typename _Tp>
struct enable_if<true, _Tp>
{ typedef _Tp type; };

#endif /* __ENABLE_IF_ */
