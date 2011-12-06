#ifndef EDT_H_
#define EDT_H_

#include "gpath.h"

typedef Gpath<PassableButMyHills, UnitCost> Edt;
typedef Gpath<Unidirectional<PassableButMyHills>, UnitCost> UniEdt;

#endif /* EDT_H_ */
